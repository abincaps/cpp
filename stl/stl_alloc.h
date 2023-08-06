#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <new>

// 定义符合STL规格的配置器接口
// 调用Alloc作用域的
// STL 容器全都使用这个 simple_alloc 介面
template <typename T, typename Alloc>
class simple_alloc {
public:
    // 静态函数是天然线程安全的
    // 静态函数避免开销
    static T* allocate(size_t n) {
        // 分配 n个T对象的大小
        // 如果要分配的大小等于0 直接优化掉
        // 调用Alloc的分配器。 这里指默认调用第一级配置器还是第二级配置器，
        // 由Alloc指定
        return 0 == n ? 0 : (T*)Alloc::allocate(n * sizeof(T));
    }

    static T* allocate(void) {
        // 分配一个T对象的大小
        return (T*)Alloc::allocate(sizeof(T));
    }
    // T* p 传入迭代器first n是个数
    static void deallocate(T* p, size_t n) {
        // 如果要释放的大小等于0 直接不做 优化
        if (0 != n) {
            // n * sizeof(T) 计算要释放空间的大小
            // 根据Alloc::作用域调用第一级或者第二级配置器
            Alloc::deallocate(p, n * sizeof(T));
        }
    }
    static void deallocate(T* p) { Alloc::dellocate(p, sizeof(T)); }
};

// 第一级配置器是直接调用malloc分配空间, 调用free释放空间
// <int inst> 模板非类型参数
template <int inst>
class __malloc_alloc_template {
private:
    // oom out of memory
    // 处理内存不足的情况
    static void* oom_malloc(size_t);              // 分配不足
    static void* oom_realloc(void*, size_t);      // 重新分配不足
    static void (*__malloc_alloc_oom_handler)();  // 内存不足处理函数
public:
    static void* allocate(size_t n) {
        void* res = malloc(n);  // 第一级配置器是直接调用malloc分配空间
        // 分配失败
        if (nullptr == res) {
            // 调用处理内存不足的函数
            res = oom_malloc(n);
        }
        return res;
    }

    static void deallocate(void* p, size_t) { free(p); }

    static void* reallocate(void* p, size_t, size_t new_size) {
        // realloc()
        // 如果size == 0, 效果等同于free(ptr), 释放内存块
        // 如果ptr是 NULL, 实际效果等同于malloc(size), 分配内存块
        // 如果size大于原内存块大小, 会找到一块足够大的新内存区域,
        // 内容拷贝后释放旧的内存块。 如果size小于原内存块大小,
        // 直接缩小内存块大小, 返回原指针。 如果size和原大小相同,
        // 直接返回原指针, 什么也不做。

        void* res = realloc(p, new_size);
        if (nullptr == res) {
            res = oom_realloc(p, new_size);
        }
        return res;
    }

    // 第一级配置器并没有 ::operator new 配置内存 不能使用set_new_handler
    // set_malloc_handle仿照set_new_handler通过将参数设置为0（nullptr)
    // 告诉系统当内存分配操作失败时, 不再执行任何特定的操作
    // 而是使用默认的处理方式（通常是抛出std::bad_alloc异常）

    // void (*set_malloc_handler(void (*f)()))() 返回函数指针
    // void (*f)() 函数指针 为set_malloc_handler函数指针中的参数
    static void (*set_malloc_handler(void (*f)()))() {
        void (*old)() = __malloc_alloc_oom_handler;
        __malloc_alloc_oom_handler = f;  // 设置新的分配内存不足的处理函数
        return (old);                    // 返回旧的分配内存不足的处理函数
    }
};

// 设置 分配内存不足的处理函数为空 默认不处理
template <int inst>
void (*__malloc_alloc_template<inst>::__malloc_alloc_oom_handler)() = nullptr;

template <int inst>
void* __malloc_alloc_template<inst>::oom_malloc(size_t n) {
    // 不断尝试释放内存，重新分配内存，直到分配成功
    for (;;) {
        void (*my_malloc_handler)() = __malloc_alloc_oom_handler;  // 获取分配内存不足处理函数
        if (nullptr == my_malloc_handler) {
            // throw std::bad_alloc() 抛出一个bad_alloc类型的异常
            // std::bad_alloc是C++ 标准库定义的异常类, 继承自std::exception基类
            // 当发生内存分配错误时, 会抛出这个异常
            throw std::bad_alloc();
        }

        my_malloc_handler();  // 尝试释放内存

        void* res = malloc(n);  // 尝试分配内存

        // 分配成功就返回
        if (res) {
            return res;
        }
    }
}

template <int inst>
void* __malloc_alloc_template<inst>::oom_realloc(void* p, size_t n) {
    // 类似oom_malloc
    for (;;) {
        void (*my_malloc_handler)() = __malloc_alloc_oom_handler;

        // 如果没有设置__malloc_alloc_oom_handler 抛出异常
        if (nullptr == my_malloc_handler) {
            throw std::bad_alloc();
        }

        my_malloc_handler();  // 尝试释放内存

        void* res = realloc(p, n);  // 尝试重新分配内存

        // 分配成功就返回
        if (res) {
            return res;
        }
    }
}

// 第二级配置器 当配置区块 小于等于 128 bytes 使用建立的 memory pool (内存池)
// 小于128字节的申请都直接在内存池申请
// 如果大于 128 bytes 交给第一级配置器处理

enum {
    __ALIGN = 8,                          // 设置对齐要求
    __MAX_BYTES = 128,                    // 第二级配置器的一次性申请的最大大小
    __NFREELISTS = __MAX_BYTES / __ALIGN  // 128 / 8 = 16 个空闲链表(free list), 节点大小分别是8的倍数,
                                          // 从8字节到128字节 number of freelists
};

template <bool threads, int inst>
class __default_alloc_template {
private:
    // 参数 bytes 上调至 8的倍数
    static size_t round_up(size_t bytes) {
        return ((bytes + __ALIGN - 1) & ~(__ALIGN - 1));

        // 8 - 1 = 7 (0...0000000111)
        // ~7 (1...1111111000)
    }

    // free-lists 结点
    // union的这种用法可以极大节省内存, 避免每个obj对象都需要额外的链表指针空间
    union obj {
        union obj* free_list_link;  // 链接空闲内存块
        char data[1];               // 存储数据
    };

    // 16 个 free lists
    static obj* volatile free_list[__NFREELISTS];

    // 根据内存块大小找到对应free list索引 索引下标从0开始
    // 通过上调对齐, 可以映射不同大小的内存块到固定数量的空闲链表中,
    // 以实现大小分类和重复利用
    // (bytes + __ALIGN - 1) / __ALIGN 向上取整 然后 - 1
    static size_t freelist_index(size_t bytes) { return ((bytes + __ALIGN - 1) / __ALIGN - 1); };

    // 分配长度为n字节的内存块
    static void* refill(size_t n);

    // 从内存池中分配一块大内存块
    // 提供给refill切分成nobjs个大小为size的小块插入free list中串连成链表
    static char* chunk_alloc(size_t size, int& nobjs);

    static char* start_free;  // 内存池起始位置
    static char* end_free;    // 内存池结束位置
    static size_t heap_size;  // 分配在堆上的内存池大小
                              // 注意这里不是内存池剩余大小，是一次分配的内存池大小
    // 内存池剩余大小由 end_free - start_free 求出

public:
    static void* allocate(size_t n) {
        // 大于128 就调用第一级配置器
        if (n > size_t(__MAX_BYTES)) {
            return __malloc_alloc_template<0>::allocate(n);
        }

        // 二级指针 相当于 *&
        // free_list + freelist_index(n) 相当于 free_list[freelist_index(n)]
        // 使用volatile指针访问共享的free_list, 保证不同线程都能访问到主存版本
        obj* volatile* cur_free_list = free_list + freelist_index(n);

        // 尝试直接从cur_free_list所指的空闲链表头部获取一个可用内存块
        obj* res = *cur_free_list;

        // 如果链表为空, 表示free list中没有可用区块
        // 调用refill()函数重新分配内存块给调用者使用
        // round_up（）将申请的内存块大小上调至8的倍数
        if (nullptr == res) {
            void* tmp = refill(round_up(n));
            return tmp;
        }

        // 将分配的对象从链表中移除
        // 修改*cur_free_list也就是free_list所指的空闲链表头部 指向
        // 将要分配的内存块指向的下一个可用的内存块 头删
        *cur_free_list = res->free_list_link;
        return res;
    }

    static void deallocate(void* p, size_t n) {
        // 大于128就调用第一级配置器
        if (n > size_t(__MAX_BYTES)) {
            __malloc_alloc_template<0>::deallocate(p, n);
            return;
        }

        // 寻找对应的free list
        obj* volatile* cur_free_list = free_list + freelist_index(n);

        // 调整free list 回收区块
        // 头插形成链
        obj* q = (obj*)p;
        // 当前将要闲置的空闲区块 指向的下一个空闲区块是
        // 当前 *cur_free_list也就是free_list 指向的空闲区块
        q->free_list_link = *cur_free_list;
        // *cur_free_list也就是free_list 指向 当前将要闲置的空闲区块
        *cur_free_list = q;
    }

    static void* reallocate(void* p, size_t old_sz, size_t new_sz);
};

// 初值设定 非const静态数据成员必须在类的外部进行初始化
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
// 只能在类的内部定义中使用 static 关键字, 在类的外部是不允许的
void* __default_alloc_template<threads, inst>::refill(size_t n) {
    int nobjs = 20;  // 向内存池申请20个新内存块 nobj 相当于 number of objs

    // chunk_alloc中的参数nobjs是引用，会修改实现分配的内存块数量
    char* chunk = chunk_alloc(n, nobjs);

    // 如果只申请到一个内存块，就直接返回一个给调用者使用
    if (1 == nobjs) {
        return chunk;
    }

    obj* volatile* cur_free_list = free_list + freelist_index(n);

    // 将首地址赋给res, 用于返回调用者使用
    obj* res = (obj*)chunk;

    // 移到第二个
    // 这里n的大小 之前经过 round_up(n)调整过 为8的倍数
    // 每个内存块的大小为n
    obj* next_obj = (obj*)(chunk + n);

    // *cur_free_list也就是free_list 为nullptr
    // 因为为nullptr才会调用refill
    // 链表头部直接指向next_obj
    *cur_free_list = next_obj;

    // i从0开始， 这里i = 1代表从第2个开始
    // 首地址也就是 第1个要回传给调用者
    for (int i = 1;; i++) {
        // 记录上一个内存块的地址
        obj* pre_obj = next_obj;
        // 移动至下一个内存块
        // 申请的大内存为连续的块， 所以可以直接 + n
        next_obj = (obj*)((char*)next_obj + n);

        // 第20个时退出， i + 1 是修正， 因为i从0开始
        if (i + 1 == nobjs) {
            // 最后链表尾部要置空
            pre_obj->free_list_link = nullptr;
            break;
        }

        // 上一个内存块的指向的下一个空闲（可用）内存块 为 当前的内存块
        pre_obj->free_list_link = next_obj;
    }
    // 也就是将其余的 nobjs - 1 个内存块结点串成链表

    // 返回的内存块不在free list 中
    // 以后可以回收进free list中
    return res;
}

template <bool threads, int inst>
char* __default_alloc_template<threads, inst>::chunk_alloc(size_t size, int& nobjs) {
    size_t total_bytes = size * nobjs;          // 链表需要申请的内存大小（字节数）
    size_t left_bytes = end_free - start_free;  // 内存池剩余字节数（剩余空间）

    // 内存池剩余空间满足需求
    if (left_bytes >= total_bytes) {
        // 先记录要返回的内存块首地址
        char* res = start_free;
        // start_free +  total_bytes
        // total_bytes这部分以及被分配出内存池，不可以再用
        // 移动内存池开始的位置
        start_free += total_bytes;
        return res;
    }

    // size * 1 内存池剩余空间满足一个区块及以上
    if (left_bytes >= size) {
        // 修改 nobjs 其余 nobjs - 1个结点会被refiil()插入free list中串连成链表
        nobjs = left_bytes / size;
        // 对应total_bytes 也要随 nobjs 变动
        total_bytes = size * nobjs;

        char* res = start_free;
        start_free += total_bytes;
        return res;
    }

    // 内存池剩余空间连一个区块大小也无法提供
    // 申请扩容两倍大小 + 额外大小的内存   heap_size >> 4 (乘16倍)
    // to_get 将要获取 的 bytes 字节数
    size_t to_get_bytes = 2 * total_bytes + round_up(heap_size >> 4);

    // 处理小于一个区块的剩余空间
    if (left_bytes > 0) {
        // 分配给对应的free list  8 16 32 ... 128 理论上不会是最大值128
        // 128肯定满足一个区块大小
        obj* volatile* cur_free_list = free_list + freelist_index(left_bytes);
        // 头插
        ((obj*)start_free)->free_list_link = *cur_free_list;
        // 修改头指针指向刚插入的内存块
        *cur_free_list = (obj*)start_free;
    }

    // 分配新的内存池
    start_free = (char*)malloc(to_get_bytes);

    //  内存不足的情况
    if (nullptr == start_free) {
        // 从要分配的区块大小开始 ， 不可能从小于size的内存块中分配
        for (int i = size; i <= __MAX_BYTES; i += __ALIGN) {
            obj* volatile* cur_free_list = free_list + freelist_index(i);

            obj* p = *cur_free_list;

            // p不等于nullptr 说明当前对应的free list中有可用的内存块
            if (p) {
                // 从当前对应的 free_list 中释放出一个内存块
                *cur_free_list = p->free_list_link;
                start_free = (char*)p;
                end_free = start_free + i;
                // 调用自己修正nobjs
                return (chunk_alloc(size, nobjs));
            }
        }

        // 分配不了一点
        end_free = nullptr;

        // 调用第一级配置器 处理分配内存不足的情况，并抛出异常
        start_free = (char*)__malloc_alloc_template<0>::allocate(to_get_bytes);
    }

    // 内存池成功扩容后修改内存池大小， 结束位置，
    heap_size += to_get_bytes;
    end_free = start_free + to_get_bytes;

    // 重新调用chunk_alloc分配区块
    return chunk_alloc(size, nobjs);
}
