/*
    __Author__ : zhangbo1@ijinshan.com
    __Time__   : 2014/05/23
    __License__: MIT

*/
#ifndef _MINHEAP_H_
#define _MINHEAP_H_

struct minheap;
struct minHeapNode{
    int32_t key   ;
    void    *data ;
};

static inline int32_t _minHeapInsert(struct minheap *this, \
                                     int32_t key, void *data);

static inline int32_t _minHeapPop(struct minheap *this, \
                                  struct minHeapNode *node_);

static inline int32_t _minHeapDes(struct minheap *this);

struct minheap * minHeapInit(int32_t maxNum);


struct minheap{
    struct minHeapNode  *nodeList;
    int32_t             headIndex;
    int32_t             lastIndex;
    int32_t             maxNum;

    int32_t (* insert)(struct minheap *this, int32_t key, void *data);
    int32_t (* pop)(struct minheap *this,  struct minHeapNode *node_);
    int32_t (* des)(struct minheap *this);

};
/*
    minheap class with like C++ object model,
    set insert, pop, des callback to minheap
*/

static inline int32_t _getFatherIndex(int32_t index){
    if(index == 1 || index == 0){
        return 0;
    }

    return (int)(index / 2);
}

static inline int32_t _getLeftChildIndex(int32_t index){
    if(index == 0){
        return 0;
    }
    return index*2;
}

static inline int32_t _getRightChildIndex(int32_t index){
    if(index == 0){
        return 0;
    }
    return index*2 + 1;
}

static inline void _exchange(int32_t *i1, int32_t *i2){
    int32_t temp = *i1;
    *i1 = *i2;
    *i2 = temp;
}

static inline void _exchangePoint(void **app, void **bpp)
{
    void *temp = *app;
    *app = *bpp;
    *bpp = temp;
}

static inline int32_t _minHeapInsert(struct minheap *this, 
                                     int32_t         key, 
                                     void           *data){

    if(this->lastIndex >= this->maxNum-1){
        return -1;
    }
    int32_t index = this->lastIndex;

    this->nodeList[this->lastIndex].data = data;
    this->nodeList[this->lastIndex].key  = key;

    this->lastIndex += 1;

    int32_t fatherIndex = _getFatherIndex(index);
    int32_t temp = -1;
    while(fatherIndex){
        if(key < this->nodeList[fatherIndex].key){
            _exchange(&this->nodeList[fatherIndex].key, \
                      &this->nodeList[index].key);
            _exchangePoint(                             \
                &this->nodeList[fatherIndex].data,      \
                &this->nodeList[index].data);

            index       = fatherIndex;
            fatherIndex = _getFatherIndex(fatherIndex);
        }
        else
            break;
    }
    
    return 0;
}

static inline int32_t _minChildIndex(struct minheap *this, int32_t index){
    if( index >= this->lastIndex ){
        return 0;
    }
    int32_t _l = _getLeftChildIndex(index);
    int32_t _r = _getRightChildIndex(index);

    if( _l >= this->lastIndex){
        return 0;
    }else if( _r >= this->lastIndex ){
        return _l;
    }

    return this->nodeList[_l].key < this->nodeList[_r].key ? _l : _r ;
}   

static inline int32_t _minHeapPop(struct minheap *this, struct minHeapNode *node){
    if(this->lastIndex == 1){
        return -1;
    }

    node->key  = this->nodeList[this->headIndex].key;
    node->data = this->nodeList[this->headIndex].data;

    _exchange(&this->nodeList[this->lastIndex-1].key, \
              &this->nodeList[this->headIndex].key);

    _exchangePoint(&this->nodeList[this->lastIndex-1].data,  \
                   &this->nodeList[this->headIndex].data);

    this->lastIndex -= 1;

    int32_t index      = this->headIndex;
    int32_t childIndex = _minChildIndex(this, index);

    while(childIndex){  
        if(this->nodeList[index].key >= this->nodeList[childIndex].key){
            _exchange(&this->nodeList[index].key, \
                      &this->nodeList[childIndex].key);
            _exchangePoint(&this->nodeList[index].data, \
                          &this->nodeList[childIndex].data);

            index      = childIndex;
            childIndex = _minChildIndex(this, childIndex);
        }else{
            break;
        }
    }

    return 0;
}

static inline int32_t _minHeapDes(struct minheap *this){
    if(this == (struct minheap *)NULL){
        return -1;
    }
    if(this->nodeList != NULL){
        free(this->nodeList);
        this->nodeList = NULL;
    }
    free(this);
    return 0;
}


#endif
