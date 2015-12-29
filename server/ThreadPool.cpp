#include "Global.h"
#include "ThreadPool.h"

// thread aync vars
pthread_cond_t  gPthCond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t gPthMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_info_t  gPthInfoVec[MAX_THREAD_POOL];
pthread_mutex_t gPthreadMutex = PTHREAD_MUTEX_INITIALIZER;

ThreadPool::ThreadPool()
{
    pthread_mutex_init(&gPthMutex, NULL);
    pthread_cond_init(&gPthCond, NULL);

    for (int i = 0; i < MAX_THREAD_POOL; i++) {
        pthread_mutex_lock(&gPthMutex);

        pthread_idx_t pthIdx;
        pthIdx.idx = i;
        if (pthread_create(&pthIdx.pth, NULL, this->__sandbox, &i) < 0) {
            // shutdown all threads
            break;
        }
        this->mPthActMap.insert(pair<int, pthread_idx_t>(0, pthIdx));

        pthread_info_t pthInfo;
        pthInfo.pthIdx = pthIdx;
        pthread_cond_init(&pthInfo.cond, NULL);
        gPthInfoVec[i] = pthInfo;

        pthread_cond_wait(&gPthCond, &gPthMutex);
        pthread_mutex_unlock(&gPthMutex);
    }
}

ThreadPool::~ThreadPool()
{
}

int ThreadPool::add(RunFunc func, void *arg)
{
    multimap<int, pthread_idx_t>::iterator mi = this->mPthActMap.begin();
    pthread_idx_t pthIdx = mi->second;

    pthread_info_t &pthInfo = gPthInfoVec[pthIdx.idx];
    pthInfo.func = func;
    pthInfo.arg  = arg;
    pthread_cond_signal(&pthInfo.cond);

    this->mPthActMap.erase(mi);
    this->mPthActMap.insert(pair<int, pthread_idx_t>(1, pthIdx));

    return 0;
}

void* ThreadPool::__sandbox(void *obj)
{
    int idx = *((int *) obj);

    pthread_mutex_lock(&gPthMutex);
    pthread_cond_signal(&gPthCond);
    pthread_mutex_unlock(&gPthMutex);

    for (;;) {
        pthread_mutex_lock(&gPthreadMutex);
        pthread_info_t &pthInfo = gPthInfoVec[idx];
        pthread_cond_wait(&pthInfo.cond, &gPthreadMutex);
        pthread_mutex_unlock(&gPthreadMutex);

        // worker start
        pthInfo.func(pthInfo.arg);
    }

    pthread_exit(NULL);

    return NULL;
}

