#pragma once

// 定义符合STL规格的配置器接口
#include <cstddef>
#include <cstdlib>
#include <new>

static void (*__malloc_alloc_oom_handler)() = nullptr;  // 内存分配超限处理函数， 默认为0

// 调用Alloc作用域的
// STL 容器全都使用这个 simple_alloc 介面
template <typename T, typename Alloc>
class simple_alloc {
public:
    // 静态函数是天然线程安全的
    // 静态函数避免开销
    static T* allocate(size_t n) { return 0 == n ? 0 : (T*)Alloc::allocate(n * sizeof(T)); }
    static T* allocate(void) { return (T*)Alloc::allocate(sizeof(T)); }
    static void deallocate(T* p, size_t n) {
        if (0 != n) {
            Alloc::allocate(p, n * sizeof(T));
        }
    }
    static void deallocate(T* p) { Alloc::dellocate(p, sizeof(T)); }
};

// 第一级配置器是直接调用malloc分配空间, 调用free释放空间
// 第二级配置器 当配置区块小于等于128bytes 就是建立一个内存池, 小于128字节的申请都直接在内存池申请

// 第一级配置器
// <int inst> 模板非类型参数
template <int inst>
class __malloc_alloc_template {
private:
    // oom out of memory
    // 处理内存不足的情况
    static void* oom_malloc(size_t);          // 分配不足
    static void* oom_realloc(void*, size_t);  // 重新分配不足
    // static void (*__malloc_alloc_oom_handler)();  // 内存不足设置的处理例程
public:
    static void* allocate(size_t n) {
        void* res = malloc(n);  // 第一级配置器是直接调用malloc分配空间
        if (0 == res) {
            res = oom_malloc(n);
        }
        return res;
    }

    static void deallocate(void* p, size_t) { free(p); }

    static void* reallocate(void* p, size_t, size_t new_size) {
        // realloc()
        // 如果size == 0, 效果等同于free(ptr), 释放内存块
        // 如果ptr是 NULL, 实际效果等同于malloc(size), 分配内存块
        // 如果size大于原内存块大小, 会找到一块足够大的新内存区域, 内容拷贝后释放旧的内存块。
        // 如果size小于原内存块大小, 直接缩小内存块大小, 返回原指针。
        // 如果size和原大小相同, 直接返回原指针, 什么也不做。

        void* res = realloc(p, new_size);  // 第一级配置器是直接调用malloc分配空间
        if (0 == res) {
            res = oom_realloc(p, new_size);
        }
        return res;
    }
};

// 第一级配置器并没有 ::operator new 配置内存
// set_malloc_handle仿照set_new_handler通过将参数设置为0（nullptr)
// 告诉系统当内存分配操作失败时, 不再执行任何特定的操作
// 而是使用默认的处理方式（通常是抛出std::bad_alloc异常）

// void (*set_malloc_handler(void (*f)()))() 返回函数指针
// void (*f)() 函数指针 为set_malloc_handler函数指针中的参数
static void (*set_malloc_handler(void (*f)()))() {
    void (*old)() = __malloc_alloc_oom_handler;
    __malloc_alloc_oom_handler = f;  // 设置新的内存分配超限的处理函数
    return (old);                    // 返回旧的内存分配超限的处理函数
}

// template <int inst>
// void(*__malloc_alloc_template<inst>::__malloc_alloc_oom_handler)() = nullptr;

template <int inst>
void* __malloc_alloc_template<inst>::oom_malloc(size_t n) {
    for (;;) {
        void (*my_malloc_handler)() = __malloc_alloc_oom_handler;
        if (nullptr == my_malloc_handler) {
            // throw std::bad_alloc()
            // 在C++ 中用于抛出一个bad_alloc类型的异常。
            // std::bad_alloc是C++ 标准库定义的异常类, 继承自std::exception基类。
            // 当发生内存分配错误时, 会抛出这个异常
            throw std::bad_alloc();
        }

        my_malloc_handler();  // 尝试释放内存

        void* res = malloc(n);  // 重复分配

        // 分配成功就返回
        if (res) {
            return res;
        }
    }
}

template <int inst>
void* __malloc_alloc_template<inst>::oom_realloc(void* p, size_t n) {
    for (;;) {
        void (*my_malloc_handler)() = __malloc_alloc_oom_handler;
        // 如果没有设置__malloc_alloc_oom_handler 抛出异常
        if (nullptr == my_malloc_handler) {
            throw std::bad_alloc();
        }
        my_malloc_handler();  // 尝试释放内存

        void* res = realloc(p, n);  // 重新分配

        // 分配成功就返回
        if (res) {
            return res;
        }
    }
}

// 第二级配置器 小于等于 128 bytes 使用 memory pool (内存池)
// 如果大于 128 bytes 交给第一级配置器处理

enum {
    __ALIGN = 8,        // 设置对齐要求
    __MAX_BYTES = 128,  // 第二级配置器 最大一次性申请大小
    __NFREELISTS = __MAX_BYTES / __ALIGN  // 128 / 8 = 16 个自由链表(free-list), 节点大小分别是8的倍数, 从8字节到128字节
};

template <bool threads, int inst>
class __default_alloc_template {
private:
    // 参数bytes上调至8的倍数
    static size_t ROUND_UP(size_t bytes) {
        return ((bytes + __ALIGN - 1) & ~(__ALIGN - 1));

        // 8 - 1 = 7 (0...0000000111)
        // ~7 (1...1111111000)
    }

    // free-lists 结点
    // union的这种用法可以极大节省内存, 避免每个obj对象都需要额外的链表指针空间
    //
    union obj {
        union obj* free_list_link;  // 链接空闲内存块
        char client_data[1];        // 存储数据
    };
    // 16 个 free-lists
    static obj* volatile free_list[__NFREELISTS];

    // 根据内存块大小找到对应free list索引
    static size_t FREELIST_INDEX(size_t bytes) { return ((bytes + __ALIGN - 1) / __ALIGN - 1); };

    static void* refill(size_t n);

    static char* chunk_alloc(size_t size, int& nobjs);

    static char* start_free;  // 内存池起始位置
    static char* end_free;    // 内存池结束位置
    static size_t heap_size;

public:
    static void* allocate(size_t n) {
        // 大于128就调用第一级配置器
        if (n > size_t(__MAX_BYTES)) {
            return __malloc_alloc_template<0>::allocate(n);
        }

        // 使用volatile指针访问共享的free_list,保证不同线程都能访问到主存版本
        obj* volatile* my_free_list = free_list + FREELIST_INDEX(n);

        // 尝试直接从my_free_list所指的空闲链表头部获取一个可用内存块
        obj* res = *my_free_list;

        // 如果链表为空, 表示free list中没有可用区块
        // 调用refill()函数将chain拆分生成链表填充给该free list
        if (nullptr == res) {
            void* tmp = refill(ROUND_UP(n));
            return tmp;
        }

        // 调整链表指针,将分配的对象从链表中移除
        *my_free_list = res->free_list_link;
        return res;
    }

    static void deallocate(void* p, size_t n) {
        // 大于128就调用第一级配置器
        if (n > size_t(__MAX_BYTES)) {
            __malloc_alloc_template<0>::deallocate(p, n);
            return;
        }

        // 寻找对应的free list 二级指针 相当于 *&
        // free_list + FREELIST_INDEX(n) 相当于 free_list[FREELIST_INDEX(n)]
        obj* volatile* my_free_list = free_list + FREELIST_INDEX(n);

        // 调整free list 回收区块
        obj* q = (obj*)p;
        q->free_list_link = *my_free_list;
        *my_free_list = q;
    }

    static void* reallocate(void* p, size_t old_sz, size_t new_sz);
};

// 初值设定
template <bool threads, int inst>
char* __default_alloc_template<threads, inst>::start_free = nullptr;
template <bool threads, int inst>
char* __default_alloc_template<threads, inst>::end_free = nullptr;
template <bool threads, int inst>
size_t __default_alloc_template<threads, inst>::heap_size = 0;
template <bool threads, int inst>
typename __default_alloc_template<threads, inst>::obj* volatile __default_alloc_template<
    threads, inst>::free_list[__NFREELISTS] = {0};

template <bool threads, int inst>
// 只能在类的内部定义中使用 static 关键字,在类的外部是不允许的
void* __default_alloc_template<threads, inst>::refill(size_t n) {
    int nobjs = 20;  // 20个新结点
    char* chunk = chunk_alloc(n, nobjs);

    // 如果只申请到一个对象大小，就直接返回一个内存大小
    if (1 == nobjs) {
        return chunk;
    }

    obj* volatile* my_free_list = free_list + FREELIST_INDEX(n);

    obj* res = (obj*)chunk;

    obj* next_obj = (obj*)(chunk + n);

    // i从0开始， 这里i = 1代表从第2个开始
    // 第1个要回传给调用者
    for (int i = 1;; i++) {
        obj* cur_obj = next_obj;
        next_obj = (obj*)((char*)next_obj + n);

        // 第20个时退出
        if (i + 1 == nobjs) {
            cur_obj->free_list_link = nullptr;
            break;
        }

        cur_obj->free_list_link = next_obj;
    }

    return res;
}

template <bool threads, int inst>
char* __default_alloc_template<threads, inst>::chunk_alloc(size_t size, int& nobjs) {
    size_t total_bytes = size * nobjs;          // 链表需要申请的内存大小
    size_t bytes_left = end_free - start_free;  // 内存池剩余空间

    // 内存池剩余空间满足需求
    if (bytes_left >= total_bytes) {
        char* res = start_free;
        start_free += total_bytes;
        return res;
    }

    // size * 1 内存池剩余空间满足一个区块及以上
    if (bytes_left >= size) {
        nobjs = bytes_left / size;
        total_bytes = size * nobjs;

        char* res = start_free;
        start_free += total_bytes;
        return res;
    }

    // 内存池剩余空间连一个区块大小也无法提供
    // 申请两倍大小 + 额外大小的内存   heap_size >> 4 (乘16倍)
    size_t bytes_to_get = 2 * total_bytes + ROUND_UP(heap_size >> 4);

    // 处理小于一个区块的剩余空间
    if (bytes_left > 0) {
        obj* volatile* my_free_list = free_list + FREELIST_INDEX(bytes_left);
        ((obj*)start_free)->free_list_link = *my_free_list;
        *my_free_list = (obj*)start_free;
    }

    start_free = (char*)malloc(bytes_to_get);

    //  内存不足的情况
    if (nullptr == start_free) {
        // TODO
    }

    // 内存池成功扩容后修改内存池大小， 结束位置， 
    heap_size += bytes_to_get;
    end_free = start_free + bytes_to_get;

    // 重新调用chunk_alloc分配区块
    return chunk_alloc(size, nobjs);
}
