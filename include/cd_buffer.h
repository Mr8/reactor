/*
    __Author__ : zhangbo1@ijinshan.com
    __Time__   : 2014/05/26
    __License__: MIT

    Buffer headerfile
*/

#ifndef _CD_BUFFER_H_
#define _CD_BUFFER_H_

#include <all.h>

#define MAXBUFFER 1048576
#define MAXSPLITC 128

#define CHARSPLIT 0X00
#define NUMSPLIT  0X01


typedef int32_t readCb(char *buf, int32_t n, void *args);
typedef int32_t endCb(char *buf, int32_t flag);

struct buffer{
    char              sockBuffer[MAXBUFFER];
    char              sendLeaveBuf[MAXBUFFER];

    char              splitC[MAXSPLITC];    /*split flag*/

    struct event      ev;
    struct reactor    *reactor;
    readCb            *rcb ;

    void              *args;                /*write callback args*/

    int32_t           nExist ;
    int32_t           readSplitN ;
    int32_t           aioFlag ;             /*CHARSPLIT or NUMSPLIT*/

    int32_t           nWBufLeave ;
    int32_t           nWBufExist ;
    int32_t           nExpWBufLen;
    char              *expWBuf  ;

};

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

#endif
