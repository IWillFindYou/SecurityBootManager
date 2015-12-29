#include "Global.h"
#include "ThreadPool.h"

#include "Protocol.h"
#include "Packet.h"
#include "PacketExecuteQueue.h"
#include "PacketParser.h"

#include "Device.h"
#include "Server.h"

#include "PingThread.h"
#include "ExecuteThread.h"

// 서버 실행
// 사용시 : 루프의 조건으로 실행할 것
bool Server::run(PacketExecuteQueue& q)
{
    EventHandler handler[] = {
        { 0,          &Server::__connect    },
        { EPOLLERR,   &Server::__disconnect },
        { EPOLLHUP,   &Server::__disconnect },
        { EPOLLRDHUP, &Server::__disconnect },
        { EPOLLIN,    &Server::__receive    },
    };
    int handlerSize = sizeof(handler) / sizeof(handler[0]);

    int retval;
    epoll_event event, currEvent;

    int n = epoll_wait(this->mEpollFd, this->mEvents, MAX_EVENTS, -1);
    for (int i = 0; i < n; i++) {
        currEvent = this->mEvents[i];
        // 클라이언트 접속이 감지된 경우가 아니고서는 핸들러 구조체 체크
        if (currEvent.data.fd != this->mSock) {
            // 이벤트 발생여부 체크를 위한 변수
            bool isEvents = false;
            // 0 번째 인덱스는 클라이언트 접속 허용을 위해 사용되므로
            // 1 번째 인덱스 부터 시작된다.
            for (int j = 1; j < handlerSize; j++) {
                if (currEvent.events & handler[j].events) {
                    if ((this->*handler[j].func)(currEvent, q) == 0)
                        isEvents = true;
                    break;
                }
            }

            // 핸들러에 등록되지 않은 이벤트의 경우 클라이언트 종료를 호출
            if (isEvents == false)
                this->__disconnect(currEvent, q);
        } else {
            (this->*handler[0].func)(currEvent, q);
        }
    }
    return true;
}

// Heartbeat & Execute Threads
int Server::createExecuteThread(PacketExecuteQueue& q)
{
    PingThread pt;
    this->mThreadPool.add(pt.run, &this->mDevices);

    // create ExecuteThread => (number of cpu core) * 2
    long cpuCounts = sysconf(_SC_NPROCESSORS_ONLN);
    long threadCounts = cpuCounts * 2;

    for (long i = 0; i < threadCounts; i++) {
        ExecuteThread et;
        this->mThreadPool.add(et.run, &q);
    }

    return 0;
}

// 서버 생성
int Server::createServer(const int port)
{
    this->mPort = port;

    this->__init();
    this->__setNonBlock(this->mSock);
    this->__initEpoll();

    return 0;
}

// 서버 종료
int Server::shutdownServer()
{
    if (this->mSock > 0)
        close(this->mSock);

    return 0;
}

// 서버 Socket 초기화
int Server::__init()
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int retval;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;

    char portStr[6] = { 0, };
    sprintf(portStr, "%d", this->mPort);
    retval = getaddrinfo(NULL, portStr, &hints, &result);

    this->mSock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (this->mSock == -1)
        return -1;

    int flag = 1;
    retval = setsockopt(this->mSock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
    if (retval == -1)
        return -1;

    retval = bind(this->mSock, result->ai_addr, result->ai_addrlen);
    if (retval == -1)
        return -1;

    listen(this->mSock, SOMAXCONN);

    freeaddrinfo(result);
    return 0;
}

// EPOLL 초기화
int Server::__initEpoll()
{
    const int EPOLL_SIZE = 100;

    this->mEpollFd = epoll_create(EPOLL_SIZE);
    
    epoll_event event;
    event.data.fd = this->mSock;
    event.events = EPOLLIN | EPOLLET;
    int retval = epoll_ctl(this->mEpollFd, EPOLL_CTL_ADD, this->mSock, &event);
    if (retval == -1)
        return -1;

    return 0;
}

// Non-Block Socket을 위한 옵션 셋팅
int Server::__setNonBlock(int sock)
{
    int flags, s;
    flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1)
        return -1;

    flags |= O_NONBLOCK;
    s = fcntl(sock, F_SETFL, flags);
    if (s == -1)
        return -1;

    return 0;
}

// 사용자 접속시 실행되는 핸들러 함수
int HANDLER Server::__connect(epoll_event currEvent, PacketExecuteQueue& q)
{
    for (;;) {
        sockaddr_in inAddr;
        sockaddr    sa;
        socklen_t   inLen = sizeof(inAddr);
        int infd;
        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

        infd = accept4(this->mSock, (sockaddr *)&inAddr, &inLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (infd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else
                return -1;
        }

        this->__setNonBlock(infd);

        //int retval = getnameinfo(&sa, inLen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
        //if (retval == 0) {
        //    // ** host = hbuf, serv = sbuf
        //}

        Device dev;
        dev.setSock(infd);
        dev.setIpAddr(inet_ntoa(inAddr.sin_addr));
        dev.setPort(inAddr.sin_port);
        dev.setDeviceType(UNKNOWN);
        this->mDevices.push_back(dev);

        cout << "Server::__connect()" << endl;

        epoll_event event;
        event.data.fd = infd;
        event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
        int retval = epoll_ctl(this->mEpollFd, EPOLL_CTL_ADD, infd, &event);
    }
    return 0;
}

// 클라이언트로 부터 데이터 수신시 실행되는 핸들러 함수
int HANDLER Server::__receive(epoll_event currEvent, PacketExecuteQueue& q)
{
    char buf[MAX_BUFFER_SIZE];
    int nread = read(currEvent.data.fd, buf, MAX_BUFFER_SIZE);
    if (nread < 1) {
        // closed connection on descriptor
        this->__disconnect(currEvent, q);
    } else {
        // NULL value setting
        //if (nread < MAX_BUFFER_SIZE) buf[nread] = 0;

        // ** recv event!!
        cout << "Server::__receive()" << endl;
        //cout << "Receive(" << nread << ") : " << buf << endl;

        PacketParser pp;
        Packet* packet = pp.decode(buf, nread);
        if (packet != NULL) {
            // get send to server <- client
            for (vector<Device>::iterator dev = this->mDevices.begin(); dev != this->mDevices.end(); dev++) {
                if (dev->getSock() == currEvent.data.fd) {
                    packet->setDevice(&(*dev));
                    break;
                }
            }
            packet->setGroups(&this->mGroups);
            q.push(packet);
        }
    }
    return 0;
}

// 클라이언트와 연결 종료시 실행되는 핸들러 함수
int HANDLER Server::__disconnect(epoll_event currEvent, PacketExecuteQueue& q)
{
    // search to disconnect socket of client
    for (vector<Device>::iterator devIter = this->mDevices.begin(); devIter != this->mDevices.end(); devIter++) {
        if (devIter->getSock() == currEvent.data.fd) {
            //multimap< string, vector<Device> >::iterator dev = this->mGroups->find(devIter->getMacAddr());
            // unregister device to group
            //if (dev != this->mGroups->end()) {
            //    this->mGroups->erase(dev);
            //}

            // setting delete iterator
            vector<Device>::iterator delDev = devIter;
            this->mDevices.erase(delDev);

            // client socket close
            int retval = close(currEvent.data.fd);
            if (retval == -1)
                return -1;

            break;
        }
    }
    cout << "Server::__disconnect()" << endl;

    return 0;
}

