/*
    __Author__ : zhangbo1@ijinshan.com
    __Time__   : 2014/05/22
    __License__: MIT

    Reactor with epoll and event driver
*/
#include <all.h>

extern struct eventOps epoll ;

static inline int32_t eventListInit(struct eventList * _L){
    if (_L == (struct eventList*)NULL){
        return -1;
    }
    _L->head    = NULL;
    _L->tail    = _L->head;
    _L->nodeNum = 0;
    return 0;
}

static inline int32_t eventListAdd(struct eventList * _L, struct event *_ev){
    if (_L == (struct eventList*)NULL || _ev == (struct event *)NULL ){
        return -1;
    }

    if (_L->head == (struct event *)NULL){
        _L->head = _ev;
        _ev->prev = NULL ;
        _ev->next = NULL ;
    }else{
        _L->tail->next = _ev;
        _ev->prev = _L->tail;
        _ev->next = NULL ;
    }
    _L->tail = _ev;
    _L->nodeNum += 1;
    return 0;
}

static inline int32_t eventListRm(struct eventList * _L, struct event *_ev){
    if (_L == (struct eventList*)NULL || _ev == (struct event *)NULL ){
        return -1;
    }
    if(_ev == _L->head){
        if(_ev->next == (struct event *)NULL){
            return eventListInit(_L);
        }else{
            _L->head = _ev->next;
        }
    }else if(_ev == _L->tail){
        if(_ev->prev == (struct event *)NULL){
            return eventListInit(_L);
        }else{
            _L->tail = _ev->prev;
            _L->tail->next = NULL ;
        }
    }else{
        _ev->prev->next = _ev->next;
        _ev->next->prev = _ev->prev;
    }
    _L->nodeNum -= 1;
    return 0;
}

static void _defaultErrCallback(void *args){
    return ;
}


void setEventError(struct event *_ev){
    _ev->status = INERRORQUEUE;
}

void setEventClose(struct event *_ev){
    _ev->status = INCLOSEQUEUE;
}

int32_t eventSelfInit(struct event *_ev,    int32_t    fd, \
                      int32_t      mode,    evCallback *_evCb, \
                      evErrorCb    *_errCb, void       *_errArgs)
{
    if( _ev == (struct event *)NULL ){
        return -1;
    }
    _ev->fd           = fd;
    _ev->mode         = mode;
    _ev->evCb         = _evCb;
    _ev->inited       = EVENT_INITED;
    _ev->status       = ININITQUEUE;
    _ev->signal       = 0 ;
    _ev->minHeapFlag  = OUT_MINHEAP;

    if( _errCb == (evErrorCb *)NULL ){
        _ev->errCb = _defaultErrCallback;
        _ev->errArgs = NULL ;
    }else{
        _ev->errCb   = _errCb;
        _ev->errArgs = _errArgs;
    }

    setnonblock(_ev->fd);
    //setnodelay(_ev->fd);
    return 0;
}

int32_t eventAddTimeout(struct event *_ev, int32_t timeDelay, evErrorCb *otCb){
    if( !_ev || _ev->inited != EVENT_INITED ){
        goto err;
    }

    _ev->timeDelay = timeDelay;

    if(otCb == (evErrorCb *)NULL){
        _ev->otCb = _defaultErrCallback;
    }
    _ev->otCb = otCb;


    struct minheap *pm = _ev->reactor->minheap;
    if(pm->insert(pm, timeDelay, _ev)){
        _ev->timeDelay   = timeDelay;
        _ev->minHeapFlag = IN_MINHEAP;
    }

    return  0;
err:
    return -1;
}

int32_t eventAddListen(struct reactor *_R, struct event *_ev){
    if(  _R  == (struct reactor *)NULL \
      || _ev == (struct event *)NULL ){
        goto err;
    }

    if(  _ev->inited != EVENT_INITED \
      || _R->inited  != EVENT_INITED){
        goto err;
    }

    if(_ev->status != ININITQUEUE){
        x_error("[ERROR]Readd listen of event with fd %d", _ev->fd);
        return -1;
    }
    _ev->reactor = _R;
    _ev->status  = INWAITQUEUE ;
    eventListAdd(&_R->waitQueue, _ev);

    _R->sysOps->add(_ev);

    return 0;
err:
    return -1;
}

int32_t eventRmListen(struct reactor *_R, struct event *_ev){
    if(  _R  == (struct reactor *)NULL \
      || _ev == (struct event *)NULL ){
        goto err;
    }

    if(  _ev->inited != EVENT_INITED \
      || _R->inited  != EVENT_INITED){
        goto err;
    }

    switch(_ev->status){
        case ININITQUEUE:
            break;

        case INWAITQUEUE:
            _R->sysOps->del(_ev);
            eventListRm(&_R->waitQueue, _ev);
            break;

        case INSIGNALQUEUE:
            _R->sysOps->del(_ev);
            eventListRm(&_R->signalQueue, _ev);
            break;

        case INERRORQUEUE:
            eventListRm(&_R->signalQueue, _ev);
            break;

        default:
            /*Error*/
            break;
    }
    _ev->status = ININITQUEUE;
    return 0;
err:
    return -1;
}


int32_t eventModListen(struct event *_ev, \
                       int32_t fd, int32_t mode, evCallback *_evCb){
    if(_ev == (struct event*)NULL){
        goto err;
    }
    if(_ev->inited != EVENT_INITED){
        goto err;
    }

    if(eventRmListen(_ev->reactor, _ev) == -1){
        goto err;
    }

    if( eventSelfInit( _ev,                         \
                       fd > -1 ? fd : _ev->fd,      \
                       mode,                        \
                       _evCb,                       \
                       _ev->errCb,                  \
                       _ev->errArgs) == -1)
    {
        goto err;
    }

    if( eventAddListen(_ev->reactor, _ev) == -1){
        goto err;
    }

    return 0;
err:
    return -1;
}

static inline int32_t _getTimeOut(struct reactor *this){
    if(this->minheap->headIndex != this->minheap->lastIndex){
        return this->minheap->nodeList[this->minheap->headIndex].key;
    }
    return -1;
}

int _reactorRun(struct reactor *this){
    if(this == (struct reactor *)NULL){
        return -1;
    }
    while(1){
        struct eventList *readySignalQueue = &this->signalQueue;
        /*There's no minheap
         * In run() will move ready event from waitQueue to signalQueue
         * So when called callBack, should move to waitQueue
         * ...
         */

        #ifdef DEBUG
            printf("Epoll run\n");
        #endif
        this->sysOps->run(&this->epOper, _getTimeOut(this));

        struct event * ev = readySignalQueue->head;
        #ifdef DEBUG
            printf("%p\n", ev);
        #endif
        for(; ev; ev = ev->next){
            /*If remove from epoll powered by evCb*/
            ev->evCb(ev, ev->signal, ev->args);
        }

        ev = readySignalQueue->head;
        struct event * ev_next = ev;
        while(ev_next){
            switch(ev->status){
                case ININITQUEUE:
                    break;

                case INSIGNALQUEUE:
                    eventListRm(&this->signalQueue, ev);
                    ev->status = INWAITQUEUE ;
                    eventListAdd(&this->waitQueue, ev);
                    ev_next = ev->next;
                    break;

                case INCLOSEQUEUE:
                    eventListRm(&this->signalQueue, ev);
                    ev_next = ev->next;
                    if(ev->errCb){
                        /*In errCb, event may be destroied*/
                        ev->errCb(ev->errArgs);
                    }
                    break;

                case INERRORQUEUE:
                    eventListRm(&this->signalQueue, ev);
                    ev_next = ev->next;
                    if(ev->errCb){
                        ev->errCb(ev->errArgs);
                    }
                    break;

                default:
                    /*Unknown status*/
                    break;
            }
        }
    }
}


struct reactor * reactorInit(int32_t maxConNum){

    struct reactor * retR = (struct reactor *)malloc(sizeof(struct reactor));
    if(retR == (struct reactor *)NULL){
        goto err;
    }

    memset(retR, 0, sizeof(struct reactor));

    eventListInit(&retR->waitQueue);
    eventListInit(&retR->signalQueue);

    /*Sysops epoll in file epoll.c which will be redirected on link*/
    retR->sysOps = &epoll;
    retR->maxConNum = maxConNum;

    /*Init epoll operator*/
    (retR->epOper).waitQueue   = &retR->waitQueue;
    (retR->epOper).signalQueue = &retR->signalQueue;
    (retR->epOper).evAdd       = &eventListAdd;
    (retR->epOper).evRm        = &eventListRm;

    /*Init epoll*/
    if( retR->sysOps->init() != 0){
        goto err;
    }

    /*Init Minheap*/
    retR->minheap = minHeapInit(maxConNum);
    if(retR->minheap == NULL){
        goto err;
    }

    int32_t cpuNum = sysconf(_SC_NPROCESSORS_CONF);
    retR->cpuNum = cpuNum > 1 ? cpuNum : 1;
    retR->inited = EVENT_INITED;

    retR->run = (reactorRun *)&_reactorRun;

    retR->stop = REACTORRUN;

    // a beautiful way to exit
    // pipe(stoppipe);
    // struct event * stopEv = (struct event *)malloc(sizeof(struct event *));
    // eventSelfInit(stopEv, stoppipe[0], )

    return retR;
err:
    return (struct reactor *)NULL;
}

void destroyReactor(struct reactor * _R){
    if(_R == (struct reactor *)NULL){
        return ;
    }
    if(_R->minheap != NULL){
        _R->minheap->des(_R->minheap);
    }
}
