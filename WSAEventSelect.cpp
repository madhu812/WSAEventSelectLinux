///////////////////////////////////////////////////////////////////////////////////////
//
//
//
//  Asynchronous Socket Functions for Incoming connections and Read,Write,Close events 
//
//
///////////////////////////////////////////////////////////////////////////////////////

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <string.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "WSAEventSelect.h"


typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR -1
#define SOCKET int
int FD_ACCEPT = 2;
int FD_READ_WRITE_CLOSE = 1;

bool WSA::WSAEventAccept(SOCKET sockFd, void * objPtr)
{
    SOCKET listener = sockFd;
    struct epoll_event event, events[512];
    int epollfd = epoll_create1(0);
    if (epollfd == -1) 
    {
        perror("Failed to create epoll instance");
        return 1;
    }
    event.data.fd = listener;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listener, &event) == -1) 
    {
        perror("Failed to add listening socket to epoll.");
        return 1;
    }
    // Have this in infinite loop so that the function waits for any incoming connections
    // this loop is a blocking one on wait() but since it's launched as async() thread the main thread will continue
    for(;;) 
    {  
        int nfds = epoll_wait(epollfd, events, 512, -1);
        if (nfds == -1) 
        {
            perror("Failed to wait for events on epoll.");
            return 1;
        }

        if(event.events & EPOLLIN) 
        {
            // You can call accept() call as we got notified about incoming connections
            Accept_Incoming();
        }
    }
    // END for(;;)--and you thought it would never end!
    return true;
}
/*
 * register events of fd to epfd
 */
static void epoll_ctl_add(int epfd, int fd, uint32_t events)
{
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		perror("epoll_ctl()\n");
		exit(1);
	}
}

bool WSA::WSAEventRWX(SOCKET sockFd)
{
    struct epoll_event events[512];
    int epfd = epoll_create(1);
    epoll_ctl_add(epfd, sockFd, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP);

    // main loop
    for(;;)
    {
        int nfds = epoll_wait(epfd, events, 512, -1);
        if (nfds > 0)
        {
            
            if(events->events & EPOLLHUP || events->events & EPOLLERR)
            {
                if(epoll_ctl(epfd, EPOLL_CTL_DEL, sockFd, NULL) == -1) 
                {
                    perror("Epoll Del Failed");
                }
                Close();// Close if the connection is terminated/closed
                return false;
            }
            Read();// since we have registered for EPOLLIN and EPOLLOUT event, based on the received event u can call Read() & Write() functions
           
        }

    } // END for(;;)--and you thought it would never end!
    return true;
}


bool WSA::WSAEventSelectLinux(SOCKET &sockFd, int &sockEvntFlag)
{
    if (sockEvntFlag == FD_ACCEPT) // pre-accept event to be captured
    {
        m_resultFromWSAEventAccept.push_back(std::async(std::launch::async, WSAEventAccept, sockFd, (void*)this ));
    }
    if (sockEvntFlag == FD_READ_WRITE_CLOSE) // three kind of events to be captured..
    {
        m_resultFromWSAEventRWX.push_back(std::async(std::launch::async, WSAEventRWX, sockFd ));
    }
    return true;
}
// Dummy functions for Read(),Write(),Close()
//Have your own definitions as per requirement
void WSA::Read(){}
void WSA::Accept_Incoming(){}
void WSA::Close(){}

int main()
{
    ///usage:Refer Readme to completely understand the usage
    WSA Wsa_Obj;
    int Socket;// dummy socket variable
    // invoking this function will make the main thread continue without blocking
    // the new thread takes the responsibility of notifying incoming connections
    Wsa_Obj.WSAEventSelectLinux( Socket, FD_ACCEPT );
    // invoking this function will make the main thread continue without blocking
    // the new thread takes the responsibility of notifying RWX operations
    Wsa_Obj.WSAEventSelectLinux( Socket, FD_READ_WRITE_CLOSE );
}

