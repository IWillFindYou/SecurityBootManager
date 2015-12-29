#ifndef THREADPOOL_H_
#define THREADPOOL_H_

// Thread 최대 생성 수
const int MAX_THREAD_POOL = 256;
// Thread 실행할 함수 포인터
typedef void (*RunFunc)(void *arg);
typedef struct _tpi
{
    int       idx;
    pthread_t pth;
} pthread_idx_t;

typedef struct _tp
{
    pthread_idx_t  pthIdx;
    pthread_cond_t cond;
    RunFunc        func;
    void           *arg;
} pthread_info_t;

class ThreadPool
{
private:
    multimap<int, pthread_idx_t> mPthActMap;
public:
    ThreadPool();
    ~ThreadPool();

    int add(RunFunc func, void *arg);
private:
    static void* __sandbox(void *obj);
};

#endif

