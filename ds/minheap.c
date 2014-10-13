/*
    __Author__ : zhangbo1@ijinshan.com
    __Time__   : 2014/05/23
    __License__: MIT

    Minheap headfile
*/

#include <all.h>

struct minheap * minHeapInit(int32_t maxNum){
    if(maxNum <= 0){
        return (struct minheap *)NULL;
    }

    struct minheap * this = (struct minheap *)malloc(sizeof(struct minheap));
    if(this == (struct minheap *)NULL){
        return (struct minheap *)NULL;
    }
    
    this->nodeList = \
        (struct minHeapNode *)calloc(maxNum, sizeof(struct minHeapNode));
    if(this->nodeList == NULL){
        free(this);
        return (struct minheap *)NULL;
    }
    
    this->headIndex = 1;
    this->lastIndex = 1;
    this->maxNum    = maxNum;

    this->insert = &_minHeapInsert;
    this->pop    = &_minHeapPop;
    this->des    = &_minHeapDes;
    return (struct minheap *)this;
}
