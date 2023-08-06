#include "thread_pool.h"

#include <pthread.h>

// 静态成员初始化
std::deque<Task*> ThreadPool::task_list;
bool ThreadPool::exit = false;
pthread_mutex_t ThreadPool::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPool::cond = PTHREAD_COND_INITIALIZER;

void Task::setData(void* data) { this->data = data; }

ThreadPool::ThreadPool(int thread_num) {
    this->thread_num = thread_num;
    printf("create %d threads\n", thread_num);
    create();
}

void ThreadPool::create() {
    pthread_id = new pthread_t[thread_num];

    for (int i = 0; i < thread_num; i++) {
        // 不处理返回值
        pthread_create(&pthread_id[i], nullptr, threadFunc, nullptr);
    }
}

void* ThreadPool::threadFunc(void* thread_data) {
    while (true) {
        // 互斥锁 同一时刻只能有一个线程进入临界区
        // 获取互斥锁 mutex, 进入临界区
        pthread_mutex_lock(&mutex);

        // 等待新任务
        while (task_list.size() == 0 && !exit) {
            // 自动释放互斥锁，使调用线程进入阻塞状态，等待条件变量的通知，等待结束后自动加锁
            pthread_cond_wait(&cond, &mutex);
            // 这里加循环是唤醒后进行再一次判断是否有任务，防止误唤醒
        }

        // 关闭线程
        if (exit) {
            // 释放互斥锁 mutex, 退出临界区
            pthread_mutex_unlock(&mutex);
            printf("tid : %lu exit\n", pthread_self());
            // 线程主动结束 终止当前线程 线程资源会被自动回收
            pthread_exit(nullptr);
        }

        printf("tid : %lu run: ", pthread_self());

        Task* task = nullptr;
        // 取出一个任务 照理说到这里 task_list不会是空的
        // while (task_list.size == 0 ...) 阻塞住了
        if (!task_list.empty()) {
            task = task_list.front();
            task_list.pop_front();
        }

        // 释放互斥锁 mutex, 退出临界区
        pthread_mutex_unlock(&mutex);

        task->run();

        printf("tid : %lu idle\n", pthread_self());
    }

    return nullptr;
}

int ThreadPool::addTask(Task* task) {
    // 获取互斥锁 mutex, 进入临界区
    pthread_mutex_lock(&mutex);
    task_list.push_back(task);
    // 释放互斥锁 mutex, 退出临界区
    pthread_mutex_unlock(&mutex);

    // 唤醒等待在指定条件变量cond上的一个线程, 使其从阻塞状态变为可运行状态
    pthread_cond_signal(&cond);

    return 0;
}

int ThreadPool::stopAll() {
    if (exit) {
        return -1;
    }

    printf("stop all threads\n");

    // 更新退出标记
    exit = true;
    // 唤醒所有等待在指定条件变量上的线程
    pthread_cond_broadcast(&cond);
    // 没有任务才会调用stopAll， 没有任务线程会进入阻塞状态
    // threadFunc中会根据exit标志退出线程

    for (int i = 0; i < thread_num; i++) {
        // pthread_join 阻塞当前线程, 等待指定的线程运行结束后再继续执行
        // 让指定线程所占的资源得到释放
        pthread_join(pthread_id[i], nullptr);
    }

    delete[] pthread_id;
    pthread_id = nullptr;

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}

size_t ThreadPool::getTaskSize() { return task_list.size(); }