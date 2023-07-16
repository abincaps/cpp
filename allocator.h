#pragma once

#include <iostream> // for cerr
#include <cstddef>  // for ptrdiff_t, size_t
#include <climits>  // for UINT_MAX
#include <new>      // for placement new
namespace cpp { 

template <typename T1, typename T2>
inline void construct(T1* pointer, const T2& arguments) {
    // placement new操作符, 用于在已分配的内存上构造一个对象, 只负责调用对象的构造函数，不进行内存分配
    // 将 T1 类型的对象构造在 pointer 所指向的已分配内存位置上，并向构造函数传递了 arguments
    new(pointer) T1(arguments);
}

template <typename T>
inline T* allocate(ptrdiff_t size, T*) {
    // ptrdiff_t 表示两个指针的距离
    // set_new_handler通过将参数设置为0（nullptr)
    // 告诉系统当内存分配操作失败时, 不再执行任何特定的操作
    // 而是使用默认的处理方式（通常是抛出std::bad_alloc异常）
    std::set_new_handler(nullptr);

    // 与 new 运算符不同，::operator new 不会调用对象的构造函数，仅仅是分配内存空间
    // 它返回的是一个指向已分配内存的 void* 指针，而不是指向对象的指针
    T* tmp = (T*)(::operator new(size_t(size * sizeof(T))));


    if (tmp == nullptr) {
        std::cerr << "out of memory" << std::endl;
        exit(1);
    }

    return tmp;
}

template <typename T>
inline void deallocate(T* buffer) {
    // ::operator delete 不会自动调用对象的析构函数。它仅用于释放内存，不会执行任何对象特定的清理操作
    // delete 运算符在释放内存之前会自动调用对象的析构函数，以确保对象所占用的资源被正确释放
    ::operator delete(buffer);
}

template <typename T>
class allocator {
public: 
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    
    pointer allocate(size_type n) {
        return cpp::allocate(difference_type(n), pointer(nullptr));
    }

    void deallocate(pointer p) {
        cpp::deallocate(p);
    }

    pointer address(reference x) {
        return pointer(&x);
    }

    const_pointer address(const_reference x) {
        return const_pointer(&x);
    }

    size_type init_page_size() {
        return std::max(size_type(1), size_type(4096 / sizeof(T)));
    }

    size_type max_size() const {
        return std::max(size_type(1), size_type(UINT_MAX / sizeof(T)));
    }
}; // end of class allocator

} // end of namespace cpp
