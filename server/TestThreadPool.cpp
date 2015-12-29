#include "Global.h"
#include "ThreadPool.h"

void func1(void* arg)
{
    cout << "func1 : " << *((int *)arg) << endl;
}

void func2(void* arg)
{
    cout << "func2 : " << *((int *)arg) << endl;
}

int main(int argc, char* argv[])
{
    ThreadPool tp;

    for (int i = 0; i < 10; i++) {
        int arg1 = 10;
        tp.add(func1, &arg1);

        int arg2 = 20;
        tp.add(func2, &arg2);
    }

    while (1);

    return 0;
}
