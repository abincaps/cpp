#include <unistd.h>

#include "thread_pool.h"

class MyTask : public Task {
public:
    MyTask() = default;

    void run() {
        printf("%s\n", static_cast<char*>(data));
        // sleep 强制暂停（休眠，阻塞）当前线程
        // 给其他线程执行的机会
        sleep(rand() % 3 + 1);
    }

    ~MyTask() = default;
};

int main() {

    MyTask t;

    char str[] = "hello word";

    t.setData(static_cast<void*>(str));

    ThreadPool pool(5);

    for (int i = 0; i < 10; i++) {
        pool.addTask(&t);
    }

    while (true) {
        printf("there are still %lu tasks need to handle\n", pool.getTaskSize());
        if (pool.getTaskSize() == 0) {
            if (pool.stopAll() == -1) {
                printf("thread pool destroy\n");
                exit(0);
            }
        }

        sleep(2);

        printf("2 seconds later..\n");
    }
    return 0;
}