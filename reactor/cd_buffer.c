/*
    __Author__ : zhangbo1@ijinshan.com
    __Time__   : 2014/05/26
    __License__: MIT

    Buffer base on reactor
*/

#include <all.h>
static void _errCallback(void *args);


static int splitFind(char *source, int nSrc, char *find, int nFind){
    if( nFind > nSrc ){
        return -1;
    }
    int i = 0 ;
    int j = 0 ;

    for(; i < nFind; j++){
        if( !(source[j] == find[i]) ){
            i = 0;
        }else{
            i++ ;
        }
        if( j == nSrc -1 ){
            break;
        }

    }
    return i == nFind ? j - nFind + 1 : -1 ;
}

static void memMove(char *buf, int nBreak, int nExit, int nMax){
    memcpy(buf, (char *)buf + nBreak, nMax - nBreak);
    memset(buf + (nExit - nBreak), 0, nMax - (nExit - nBreak));
}

static int aioNRead(char *buf, int nExit, int nMax, readCb *rcb, void *args){
    if( rcb == (readCb *)NULL || buf == (char *)NULL ){
        return ;
    }
    uint32_t nUsed = (* rcb)(buf, nExit, args);
    assert(nUsed <= nExit);
    memMove(buf, nUsed, nExit, nMax);
    return nUsed;
}


static void _readFromSocket(struct event * _ev, int signal, void *_args){
    if( _ev == (struct event *)NULL \
        || _args == (void *)NULL ){
        #ifdef DEBUG
            x_error("Read from socket error\n");
        #endif
        return ;
    }
    if( signal & EV_ERR ){
        goto err;
    }
    struct buffer *_bs = (struct buffer *)_args;
    char *buf = _bs->sockBuffer;

    int nRecv = 0;
    while(1){
        if( MAXBUFFER - _bs->nExist <= 0){
            goto err;
        }
        nRecv = recv(_ev->fd, buf + _bs->nExist, MAXBUFFER - _bs->nExist, MSG_DONTWAIT);
        #ifdef DEBUG
            x_error("readFromSocket be called and nRecv = %d\n", nRecv);
        #endif
        if( nRecv < 0 ){
            if(errno == EAGAIN || errno == EINTR){
                break;
            }else{
                #ifndef DEBUG
                    x_error("Error in readFromSocket\n");
                #endif
                goto err;
            }
        }else if(nRecv == 0){
            _ev->status = INCLOSEQUEUE;
            break;
        }else{
            _bs->nExist += nRecv ;
        }

        break;
    }

    int iPos  = -1;
    int nUsed = 0;
    switch(_bs->aioFlag){

        case NUMSPLIT:
            /*Deal with number of n in buffer*/
            if( _bs->nExist >= _bs->readSplitN ){
                #ifdef DEBUG
                    x_error("aioNRead be called\n");
                #endif
                nUsed = aioNRead(_bs->sockBuffer, _bs->nExist, MAXBUFFER, _bs->rcb, _bs->args);
                _bs->nExist -= nUsed;
            }
            break;

        case CHARSPLIT:
            /*Find split*/
            iPos = splitFind(_bs->sockBuffer, _bs->nExist, _bs->splitC, _bs->readSplitN);
            if( iPos != -1 ){
                #ifdef DEBUG
                    x_error("aioNRead be called\n");
                #endif
                nUsed = aioNRead(_bs->sockBuffer , _bs->nExist, MAXBUFFER, _bs->rcb, _bs->args);
                _bs->nExist -= nUsed;
            }
            break;

        default :
            /*Error*/
            break;
    }

    if( _ev->evCb == (evCallback *)NULL ){
        #ifdef DEBUG
            x_error("Read Callback not define\n");
        #endif
        goto err;
    }

    return ;
err:
    setEventError(_ev);
    return ;
}


static void _writeToSocket(struct event * _ev, int mode, void *args){
    if( _ev == (struct event *)NULL \
        || args == (void *)NULL ){
        return ;
    }
    struct buffer * _B = (struct buffer *)args;
    int nSend = 0;
    while(1){
        nSend = send(_B->ev.fd, _B->sendLeaveBuf, _B->nWBufExist, MSG_NOSIGNAL | MSG_DONTWAIT);

        if( nSend < 0 ){
            if(errno == EAGAIN || errno == EINTR){
                continue;
            }else{
                goto err;
            }
        }
        break;
    }

    if(nSend < _B->nWBufExist ){
        if( nSend > 0 ){
            _B->nWBufExist -= nSend;
        }
    }else{
        _B->nWBufExist = 0;
        if( eventModListen(&_B->ev, -1, EV_READ, (evCallback *)&_readFromSocket) == -1){
            goto err;
        }
    }

    memMove(_B->sendLeaveBuf, nSend, _B->nWBufExist, MAXBUFFER);
    return ;

err:
    setEventError(&_B->ev);
    return ;
}


int bufferWrite(struct buffer * _B, char *buf, int n){
    if( _B == (struct buffer *)NULL ){
        goto err;
    }
    if( _B->nWBufLeave < n ){
        #ifdef DEBUG
            x_error("Buffer Error, No enough memory\n");
        #endif
        goto err;
    }
    
    int nSend = send(_B->ev.fd, buf, n, MSG_NOSIGNAL | MSG_DONTWAIT);
    #ifdef DEBUG
        x_error("[DEBUG]Send to Client %d Byte\n", nSend);
    #endif
    if( nSend < 0 ){
        if(errno == EAGAIN || errno == EINTR){
            memcpy(_B->sendLeaveBuf + _B->nWBufExist, buf, n);
            _B->nWBufExist += n ;
            _B->ev.args = (struct buffer *)_B;
            eventModListen(&_B->ev, -1, EV_WRITE, (evCallback *)&_writeToSocket);
        }else{
            goto err;
        }
    }

    return 0 ;
err:
    setEventError(&_B->ev);
    return -1;
}

void bufferReadContinue(struct buffer * _B, int nNeed){
    assert(nNeed < MAXBUFFER);
    _B->readSplitN = nNeed;
}

int bufferReadUntil(struct buffer * _B, int n, readCb *rcb, void *args){  
    if( _B == (struct buffer *)NULL ){
        goto err;
    }
    if(n > MAXBUFFER){
        goto err;
    }
    _B->aioFlag    = NUMSPLIT;
    _B->rcb        = rcb ;
    _B->nExist     = 0;
    _B->args       = args;
    _B->readSplitN = n ;

    memset(_B->sockBuffer,   0, MAXBUFFER);
    memset(_B->sendLeaveBuf, 0, MAXBUFFER);
    return 0;
err:
    return -1;
}

int bufferReadSplit(struct buffer *_B, char * split, int nSplit, readCb *rcb, void *args){
    if( _B == (struct buffer *)NULL ){
        goto err;
    }
    if( nSplit >= MAXSPLITC ){
        goto err;
    }
    _B->aioFlag    = CHARSPLIT ;
    _B->rcb        = rcb ;
    _B->nExist     = 0;
    _B->args       = args;
    _B->readSplitN = nSplit ;

    if(nSplit > MAXSPLITC){
        return -1;
    }

    strncpy(_B->splitC, split, nSplit);

    memset(_B->sockBuffer,   0, MAXBUFFER);
    memset(_B->sendLeaveBuf, 0, MAXBUFFER);
    return 0;
err:
    return -1;
}


struct buffer * createBuffer(struct reactor * _R, int fd){

    if( _R == (struct reactor *)NULL ){
            goto err;
    }

    struct buffer * _b = (struct buffer *)malloc(sizeof(struct buffer));
    if( _b == (struct buffer *)NULL){
        goto err;
    }

    _b->ev.args     = (struct buffer *)_b;
    _b->ev.errArgs  = (struct buffer *)_b;
    _b->nExist      = 0;
    _b->nExpWBufLen = 0;
    _b->nWBufLeave  = (int)MAXBUFFER;
    _b->expWBuf     = (char *)NULL ;
    _b->reactor     = _R ;

    if( eventSelfInit(&_b->ev, fd, EV_READ, (evCallback *)&_readFromSocket, \
                     (evErrorCb *)&_errCallback, _b) == -1)
    {
        #ifdef DEBUG
            x_error("In Buffer create, error of eventSelfInit\n");
        #endif
        goto err;
    }

    if( eventAddListen(_R, &_b->ev) == -1 ){
        goto err;
    }


    return (struct buffer *)_b;

err:
    return (struct buffer *)NULL;
}


void disBuffer(struct buffer * _B){
    eventRmListen(_B->reactor, &_B->ev);
    free(_B);
}

static void _errCallback(void *args){
    struct buffer *_B = (struct buffer *)args;
    close(_B->ev.fd);
    disBuffer(_B);
}
