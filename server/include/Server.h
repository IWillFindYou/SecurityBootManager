#ifndef SERVER_H_
#define SERVER_H_

#define HANDLER

class Server;

// 이벤트에 따른 핸들러 함수
typedef int (Server::*EventFunc)(epoll_event currEvent, PacketExecuteQueue& q);
// 이벤트 핸들러 등록을 위한 구조체
typedef struct _events
{
    int          events;
    EventFunc    func;
} EventHandler;

class Server
{
private:
    enum Config {
        MAX_EVENTS = 64,
        MAX_BUFFER_SIZE = 512
    };
protected:
    int                                mEpollFd;
    int                                mSock;
    int                                mPort;
    multimap< string, vector<Device> > mGroups;
    vector<Device>                     mDevices;
    epoll_event                        mEvents[MAX_EVENTS];

    ThreadPool                         mThreadPool;
public:
    Server() : mSock(0), mPort(0) {}
    ~Server() { this->shutdownServer(); }

    int  createServer(const int port);
    int  shutdownServer();
    int  createExecuteThread(PacketExecuteQueue& q);
    bool run(PacketExecuteQueue& q);
private:
    // 객체 복사 방지
    Server(const Server& obj);
    Server& operator=(const Server& obj);

    int  __init();
    int  __initEpoll();
    int  __setNonBlock(int sock);

    int  HANDLER __connect(epoll_event currEvent, PacketExecuteQueue& q);
    int  HANDLER __receive(epoll_event currEvent, PacketExecuteQueue& q);
    int  HANDLER __disconnect(epoll_event currEvent, PacketExecuteQueue& q);
};

#endif

