/*
    __Author__ : zhangbo1@ijinshan.com
    __Time__   : 2014/05/21
    __License__: MIT
*/
#ifndef _EVENT_H_
#define _EVENT_H_

#include <all.h>

#define EVENT_INITED  0X19891115

#define INWAITQUEUE   0X0001
#define INSIGNALQUEUE 0X0002
#define ININITQUEUE   0X0003
#define INCLOSEQUEUE  0x0004
#define INERRORQUEUE  0x0005

#define SOCKETOPEN    0X00
#define SOCKETCLOSE   0X01
#define SOCKETERROR   0X02

#define REACTORSTOP   0X00
#define REACTORRUN    0X01

#define EV_READ_INT      0
#define EV_WRITE_INT     1
#define EV_SIGNAL_INT    2
#define EV_CONNECT_INT   3
#define EV_ACCEPT_INT    4
#define EV_BREAK_INT     28
#define EV_ERR_INT       29
#define EV_FORCE_OFF_INT 30
#define EV_SLEEP_INT     31

#define EV_READ         (1 << EV_READ_INT)
#define EV_WRITE        (1 << EV_WRITE_INT)
#define EV_SIGNAL       (1 << EV_SIGNAL_INT)
#define EV_CONNECT      (1 << EV_CONNECT_INT)
#define EV_ACCEPT       (1 << EV_ACCEPT_INT)
#define EV_BREAK        (1 << EV_BREAK_INT)
#define EV_ERR          (1 << EV_ERR_INT)
#define EV_FORCE_OFF    (1 << EV_FORCE_OFF_INT)
#define EV_SLEEP        (1 << EV_SLEEP_INT)


#define IN_MINHEAP      0X01
#define OUT_MINHEAP     0X02

struct event;
struct reactor;
struct eventList;

typedef int32_t (* eventAdd)(struct eventList * _L, struct event *_ev);
typedef int32_t (* eventRm)(struct eventList * _L, struct event *_ev);

typedef void evCallback(struct event *, int, void *);
typedef void evErrorCb(void *args);
typedef int  reactorRun(struct reactor *this);


struct epollOpe {
    void     *waitQueue;
    void     *signalQueue;
    eventAdd evAdd;
    eventRm  evRm;
};



struct event {
    int32_t        fd;
    int32_t        mode;
    int32_t        signal;

    evCallback     *evCb;     /*Event callback when occur*/
    evErrorCb      *errCb;    /*Callback when error*/
    evErrorCb      *otCb;     /*Overtime callback when timeout*/
    struct event   *prev;     /*Event list point*/
    struct event   *next;

    void           *args;     /*Argument point*/
    void           *errArgs;

    struct reactor *reactor;
    int32_t        inited;      /*Flag of inited with magic code*/
    int32_t        status;      /*Flag of which queue event in*/
    int8_t         minHeapFlag; /*Minheap key*/
    int32_t        timeDelay;   /*Timeout time delay*/
};

struct eventList{
    struct event  *head;
    struct event  *tail;
    int32_t       nodeNum;
};

struct eventOps {
    char *name;
    int32_t (*init)(void);
    int32_t (*add)(struct event *);
    int32_t (*mod)(struct event *);
    int32_t (*del)(struct event *);
    int32_t (*run)(struct epollOpe *, int32_t timeout);
};

struct reactor{
    struct eventOps   *sysOps;        /*method of event operations*/
    struct eventList  waitQueue;      /*event list of in waiting queue*/
    struct eventList  signalQueue;    /*event list of in signal queue*/
    struct minheap    *minheap;       /*minheap point*/
    struct epollOpe   epOper;         /*epoll reverse dependience module*/
    int32_t           cpuNum;
    int32_t           inited;
    int32_t           maxConNum;

    int32_t           stop;
    int32_t           stoppipe[2];    /*Pipe to stop self*/

    reactorRun        *run;           /*Function point of self run*/
};


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

#endif
