#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <all.h>

static int setnonblock(int s){
    int ret = fcntl(s, F_GETFL, 0);
    return fcntl(s, F_SETFL, ret | O_NONBLOCK);
}

static int setreuseaddr(int s){
    int optval = 1; 

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
    { 
        perror("");
        return -1;
    }
    return 0;
}

static int setnodelay(int s){
	int optval = 1;
	if(setsockopt(s, IPPROTO_TCP, TCP_NODELAY, \
                 (char *)&optval, sizeof(optval) ) == -1)
    {
		perror("");
		return -1;
	}
	return 0;
}
#endif
