/*
    __Author__ : zhangbo1@ijinshan.com
    __Time__   : 2014/05/20
    __License__: MIT
*/

#ifndef _ALL_H_
#define _ALL_H_
#define DEBUG

/*Sys Head files*/
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <malloc.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

/*Self define headfiles*/

/*reactor*/
#include <cd_event.h>
#include <cd_buffer.h>

/*ds datastrue*/
#include <minheap.h>

/*Net interface operation*/
#include <socket.h>

#define x_error(fmt, arg...) printf(fmt, ##arg)

#endif
