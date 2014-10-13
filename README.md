# Reactor 模块文档
##
- 本代码用于演示一个Reactor模型的raw工作模型

## 整体结构
- 多路复用 (epoll 模块)
- 事件驱动 (事件封装)
- 缓冲管理 (上层buffer管理)

---

## 设计思想

> 层次化的设计，每一个模块只调用上一个模块的接口，并将耦合聚拢于接口处，同时每个模块也只暴露固定的操作接口给上层调用。
如果下层需要依赖上层的一些操作，将这部分操作提取出接口，由上层调用的时候将接口实现传递给下层。
最终整个网络模块只暴露了对于缓冲区的几个操作，不需要考虑数据收发时候的套接字状态等。

---

## 详细介绍
### **1. 多路复用**
```
    struct eventOps {
        char *name;
        int32_t (*init)(void);
        int32_t (*add)(struct event *);
        int32_t (*mod)(struct event *);
        int32_t (*del)(struct event *);
        int32_t (*run)(struct epollOpe *, int32_t timeout);
    };
    struct eventOps epoll = {
        "GNU/Linux/EPOLL",
        &epoll_init,
        &epoll_add,
        &epoll_mod,
        &epoll_del,
        &epoll_run
    };
```

>  通过此结构(interface),将epoll操作与系统实际的模块隔离,底层可以使用SELECT或者KQUEUE
其他模块通过以下方式导入IO多路复用模块，将耦合点聚拢在接口处。接口供上层event调用

    extern struct eventOps epoll ;

### **2. 事件驱动**
事件驱动主要围绕Reactor来设计，其中Reactor维护三个队列
- **ININITQUEUE** 初始化队列，事件最初状态
- **ININITQUEUE** 信号队列，其中保存了被触发后的事件
- **INCLOSEQUEUE** 关闭队列，处于此队列中的事件将被移除反应堆并关闭连接，调用close callback回收资源
- **INERRORQUEUE** 错误队列，处于此队列中的事件将调用error callback函数

> Reactor是一个全局的结构，是所有event的归属。
事件驱动模块提供了以下几个接口供buffer层调用

```
    int32_t eventSelfInit(struct event *_ev,   \
                      int32_t      fd,     \
                      int32_t      mode,   \
                      evCallback *_evCb,   \
                      evErrorCb  *_errCb,  \
                      void       *_errArgs);
    /*
        Self initilize
        Before an event add to reactor, we should init
        event with listen fd and set callback of when
        event has been occured.
    */
    
    
    int32_t eventAddListen(struct reactor *_R, struct event *_ev);
    /*
        Add an event to reactor
    */
    
    int32_t eventRmListen(struct reactor *_R, struct event *_ev);
    /*
        Remove from reactor
    */
    
    int32_t eventModListen(struct event *_ev, int fd, int32_t mode, evCallback *_evCb);
    /*
        Modify listen mode
    */
    
    void setEventError(struct event *_ev);
    /*
        Set event error when error raise, when event has been set error flag
        error callback will be called
    */
    
    void setEventClose(struct event *_ev);
    /*
        Set error close when server close or been closed
        when event has been set close flag,
        close callback will be called, close callback may
        as same as error callback.
    */
    
    
    struct reactor * reactorInit(int32_t maxConNum);
    /*
        init reactor with max connection number
    */
    
    void destroyReactor(struct reactor * _R);
    /*
        Destroy a reactor
    */
```
### **3.缓冲管理**
> 通过层次化的设计，将上述三个模块隔离开，最终在buffer层导出以下几个接口

```
    struct buffer * createBuffer(struct reactor * _R, int32_t socket);
    /*
        create a buffer object, with a socket you which you want to 
        set to reactor.
    */
    
    int32_t bufferWrite(struct buffer * _B, char *buf, int32_t n);
    /*
        NOTICE: In bufferWrite, datas would be send to socket directly.
        when net device not ready, send will return a value of 
        EAGAIN or EINTR.
        In this case, we should copy data to write buffer of our self,
        and add write event to reactor, write event will trigger 
        when net device ready.
    */
    
    int32_t bufferReadUntil(struct buffer * _B, int32_t n, readCb *rcb, void *args);
    /*
        Set callback to buffer when then buffer size >= n.
        when read_buffer >= n, the read callback will be trigger.
        NOTICE, the read callback should return the actual number of used.
    */
    
    void bufferReadContinue(struct buffer * _B, int nNeed);
    /*
        when you read from socket with number of n, but there are not a whole
        package you want, you should set the number of n you want the next event
        trigger.
    
    */
    int32_t bufferReadSplit(struct buffer * _B, char * split, int32_t nSplit, readCb *rcb, void *arg);
    /*
        Set callback to buffer when there are split char in buffer.
        ex:
            GET / HTTP/1.1\r\n\r\n
            Host: www.baidu.com\r\n\r\n
        the \r\n\r\n is char split.
    */
    
    
    void disBuffer(struct buffer * _B);
```

>使用场景：每次有连接请求到来的时候，Accept连接后创建一个buffer结构，用于管理此连接的数据收发操作。
其中buffer提供了write 和 两种读的方式
- **bufferWrite** 直接将数据写入到缓冲区，缓冲区会写到socket，如果socket写满，将此socket注册到 epoll中，事件类型为 WRITEN
- **bufferReadUntil**， 当缓冲区中的数据满足参数n的时候，调用注册到缓冲区的callback，此callback一般为协议处理函数，其中返回的个数为实际使用到的buffer中的字节数M，底层buffer管理会将此buffer向前挪动M个字节
- **bufferReadContinue**, 当读取完数据后，如果还需要继续读取N个字节的数据，调用此函数
- **bufferReadSplit**，和bufferReadUntil类似，触发条件为buffer 中存在split的分隔符时候，一般用于字符流的处理
