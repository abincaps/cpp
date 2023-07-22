#pragma once

#include <new>  // for placement new

// <stl_construct>：定义了全域函数 construct() 和 estroy()，负责对象的构造和析构

template <typename T1, typename T2>
inline void construct(T1* pointer, const T2& arguments) {
    // placement new操作符, 用于在已分配的内存上构造一个对象,
    // 只负责调用对象的构造函数，不进行内存分配 将 T1 类型的对象构造在 pointer
    // 所指向的已分配内存位置上，并向构造函数传递了 arguments
    new (pointer) T1(arguments);
}

// 接受指针
template <typename T>
inline void destroy(T* pointer) {
    // 调用析构函数释放
    pointer->~T();
}

// ForwardIterator 单向序列中进行顺序遍历的迭代器
// 接受两个迭代器, 调用 value_type() 获得迭代器所指对象的类别
template <typename ForwardIterator>
inline void destroy(ForwardIterator first, ForwardIterator last) {
    __destroy(first, last, value_type(first));
}

// trivial destructor（平凡析构函数）
// 没有显式定义析构函数
// 或者析构函数不接受任何参数
// 函数体里没有内容

template <typename ForwardIterator, typename T>
inline void __destroy(ForwardIterator first, ForwardIterator last, T*) {
    // 判断T类型是否是平凡析构函数
    using trivial_destructor = typename __type_traits<T>::has_trivial_destructor;
    __destroy_aux(first, last, trivial_destructor());
}

// non-trivial destructor
// __false_type 不是平凡析构函数
template <typename ForwardIterator>
inline void __destroy(ForwardIterator first, ForwardIterator last, __false_type) {
    // 将 [first,last) 范围内的对象一个一个析构掉
    for (; first < last; first++) {
        // *first对迭代器first进行解引用, 得到它当前指向的对象
        // &取这个对象的地址
        // destory(first) 直接将迭代器first本身传递给destory函数
        // 这意味着, 传递给destory的参数是一个迭代器, 不是一个对象指针。
        destory(&*first);
    }
}

// trivial destructor
// __true_type 是平凡析构函数 也就是基本类型
// 基本类型只需要把内存归还给free list 或者 free掉
// 优化，什么也不做
template <typename ForwardIterator>
inline void __destroy(ForwardIterator first, ForwardIterator last, __true_type) {}

// 泛型特化
// inline void destroy(char*, char*) {}
// inline void destroy(wchar_t*, wchar_t*) {}