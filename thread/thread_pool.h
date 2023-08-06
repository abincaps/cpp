#pragma once

#include <pthread.h>

#include <cstddef>
#include <deque>
#include <iostream>
#include <string>

class Task {
public:
    Task() = default;
    Task(std::string& task_name) : task_name(task_name), data(nullptr) {}
    void setData(void* data);
    // 包含纯虚函数的类(也就是抽象类）不能直接实例化
    // 子类继承抽象类后，必须实现抽象类中的所有纯虚函数，否则子类也属于抽象类
    virtual void run() = 0;
    ~Task() = default;

protected:
    std::string task_name;  // 任务的名称
    void* data;             // 执行任务的具体数据
};

class ThreadPool {
public:
    ThreadPool(int thread_num);  // 设置线程池大小
    int addTask(Task* task);
    int stopAll();
    size_t getTaskSize();

protected:
    static void* threadFunc(void* thread_data); // 在新线程中执行的函数
    void create();  // 创建线程
private:
    static std::deque<Task*> task_list;  // 任务列表
    static bool exit;                    // 线程退出的标志
    int thread_num;                      // 线程池中启动的线程数
    pthread_t* pthread_id;

    static pthread_mutex_t mutex;
    static pthread_cond_t cond;
};
