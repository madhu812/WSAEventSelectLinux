///////////////////////////////////////////////////////////////////////////////////////
//
//
//
//  Asynchronous Socket Functions for Incoming connections and Read,Write,Close events On Linux
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
#include <iostream>
#include <thread>
#include "WSAEventSelect.h"


typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR -1
#define SOCKET int
int FD_ACCEPT = 2;
int FD_READ_WRITE_CLOSE = 1;
static struct sockaddr_in serv_addr;
static int m_sockfd;
static socklen_t addrlen;

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
            //Accept_Incoming();// if this function has accept() then the thread blocks until accepted
            printf("New incoming connection detected......\n");
            auto future = std::async(std::launch::async,Accept_Incoming);// this makes the thread non-blocking and accept() can be launched in new thread
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
            auto future = std::async(std::launch::async,Read);// This launches Read in seperate thread
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

bool WSA::Listen()
{
    
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8888);
    addrlen = sizeof(serv_addr);
    SOCKET sock_fd = socket( AF_INET, SOCK_STREAM, 0 );
    m_sockfd=sock_fd;
    if( sock_fd == INVALID_SOCKET )
    {
        perror("Failed to create TCP listening socket");
        return false;
    }
    const int enable = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("setsockpot failed");
    }
        if( SOCKET_ERROR == bind( sock_fd, (SOCKADDR*)&serv_addr, sizeof( serv_addr ) ) )
        {
            if( errno != EADDRINUSE )
            {
                perror("Failed to bind TCP listening socket");
                close( sock_fd );
                return false;
            }
            else
            {
                perror("Failed to bind TCP listening socket,Check if Address is already in use");
                close( sock_fd );
                return false;
            }
        }
    std::cout << " Server is Listening on port [" << 8888 << "] ThreadID:: [" << gettid() << "]" << std::endl;
    if( SOCKET_ERROR == listen( sock_fd, 5 ) )
    {
        perror("Failed to start listening on TCP server socket");
        close( sock_fd );
        return false;
    }
    // invoking this function will make the main thread continue without blocking
    // the new thread takes the responsibility of notifying incoming connections
    if( this->WSAEventSelectLinux( sock_fd, FD_ACCEPT )  == false)
    {
        close( sock_fd );
        return false;
    }
    return true;
}

void WSA::Accept_Incoming()
{
    int new_socket;
    if ((new_socket
        = accept(m_sockfd, (struct sockaddr*)&serv_addr, &addrlen))
    < 0) {
    perror("accept");
    exit(EXIT_FAILURE);
    }
    std::cout << " New connection accepted......ThreadID:: [" << gettid() << "]" << std::endl;
}

bool WSA::Connect()
{
    SOCKET      sock_fd;
    SOCKADDR_IN sAddr;
    sAddr.sin_family = AF_INET;
    sAddr.sin_port = htons(8888);
    if (inet_pton(AF_INET, "0.0.0.0", &sAddr.sin_addr)<= 0) 
    {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
    }
    sock_fd = socket( PF_INET, SOCK_STREAM, 0 );
    if( sock_fd == INVALID_SOCKET )
    {
        perror("Failed to create new socket.");
        return false; 
    }
    std::cout << " Connecting to Remote host......ThreadID:: [" << gettid() << "]" << std::endl;
    int retVal = connect( sock_fd, (struct sockaddr *)&sAddr, sizeof(sAddr) );

    if( retVal == -1)
    {
        perror("Failed to connect to remote host.");
        sock_fd = INVALID_SOCKET;
        close( sock_fd );
        return false;
    }
    else
    {
        // invoking this function will make the main thread continue without blocking
        // the new thread takes the responsibility of notifying RWX operations
        std::cout << " Successfully Connected to Remote Host.....ThreadID:: [" << gettid() << "]" << std::endl;
        this->WSAEventSelectLinux(sock_fd,FD_READ_WRITE_CLOSE);
    }
    return true;
}

// Dummy functions for Read(),Close()
//Have your own definitions as per requirement
void WSA::Read(){}
void WSA::Close(){}

int main()
{
    //Refer Readme to completely understand the usage
    //of this library
    WSA obj;
    //Both server and client side comms are carried out here
    // Create a thread for Listen()
    std::thread listenThread([&obj]() {
        obj.Listen();
    });

    // Create a thread for Connect()
    std::thread connectThread([&obj]() {
                obj.Connect();
    });

    // Wait for both threads to finish
    listenThread.join();
    connectThread.join();

    
}