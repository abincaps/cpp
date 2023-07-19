#include <cstring>

// 全域函数 作用于未初始化的空间上

// 从first开始n个元素填充成x
template <typename ForwardIterator, typename Size, typename T>
inline ForwardIterator uninitialized_fill_n(ForwardIterator first, Size n, const T& x) {
    // value_type() 判断迭代器所指向元素的类型
    return __uninitialized_fill_n(first, n, x, value_type(first));
}

// POD 指 Plain Old Data，也就是基本类型（scalar types）或传统的 C struct 类型
template <typename ForwardIterator, typename Size, typename T, typename T1>
inline ForwardIterator __uninitialized_fill_n(ForwardIterator first, Size n, const T& x, T1*) {
    // 通过迭代器所指向元素的类型 来 判断是否是pod
    // 例如 T1模板参数推断出来是int
    // 全特化
    // __type_traits<int>::is_is_POD_type 也就是 __false_type
    using is_POD = typename __type_traits<T1>::is_POD_type;
    // is_POD() 相当于 __true_type() 或这 __false_type()
    return __uninitialized_fill_n_aux(first, n, x, is_POD());
}

template <typename ForwardIterator, typename Size, typename T>
inline ForwardIterator __uninitialized_fill_n_aux(ForwardIterator first, Size n, const T& x, __true_type) {
    return fill_n(first, n, x);
}

template <typename ForwardIterator, typename Size, typename T>
ForwardIterator __uninitialized_fill_n_aux(ForwardIterator first, Size n, const T& x, __false_type) {
    ForwardIterator cur = first;
    //*cur 先获取迭代器所指向的对象  & 获取迭代器所指向的对象的地址
    for (; n > 0; n--, cur++) construct(&*cur, x);
    return cur;
}

// 接收源范围的输入迭代器[first, last)
// 在result指向的空间拷贝构造出对象
template <typename InputIterator, typename ForwardIterator>
inline ForwardIterator uninitialized_copy(InputIterator first, InputIterator last, ForwardIterator result) {
    return __uninitialized_copy(first, last, result, value_type(result));
}

// 判断是否是 pod
template <typename InputIterator, typename ForwardIterator, typename T>
inline ForwardIterator __uninitialized_copy(InputIterator first, InputIterator last, ForwardIterator result, T*) {
    using is_POD = typename __type_traits<T>::is_POD_type;
    // is_POD() 相当于 __true_type() 或这 __false_type()
    return __uninitialized_copy_aux(first, last, result, is_POD());
}

// 是pod
template <typename InputIterator, typename ForwardIterator>
inline ForwardIterator __uninitialized_copy_aux(InputIterator first, InputIterator last, ForwardIterator result,
                                                __true_type) {
    return copy(first, last, result);
}

// 不是pod
template <typename InputIterator, typename ForwardIterator>
ForwardIterator __uninitialized_copy_aux(InputIterator first, InputIterator last, ForwardIterator result,
                                         __false_type) {
    ForwardIterator cur = result;
    // construct(指针位置，构造参数)
    for (; first != last; ++first, ++cur) construct(&*cur, *first);  // 一个一个构造
    // 返回拷贝结束的迭代器位置
    return cur;
}

// 针对 char* 和 wchar_t* 这两种类型最具有效率的做法
// memmove（直接搬移内存内容）来执行复制行为

// 以下针对 const char* 的特化版本
inline char* uninitialized_copy(const char* first, const char* last, char* result) {
    memmove(result, first, last - first);
    // 返回拷贝结束的迭代器位置
    return result + (last - first);
}
// 以下针对 const wchar_t* 的特化版本
inline wchar_t* uninitialized_copy(const wchar_t* first, const wchar_t* last, wchar_t* result) {
    // char 是 1个字节所以不用sizeof  wchar_t一般是4个字节
    memmove(result, first, sizeof(wchar_t) * (last - first));
    return result + (last - first);
}

// 从 [first, last) 填充为x
// 类似 uninitialized_fill_n
template <typename ForwardIterator, typename T>
inline void uninitialized_fill(ForwardIterator first, ForwardIterator last, const T& x) {
    // value_type() 判断迭代器所指向元素的类型
    __uninitialized_fill(first, last, x, value_type(first));
}

template <class ForwardIterator, class T, class T1>
inline void __uninitialized_fill(ForwardIterator first, ForwardIterator last, const T& x, T1*) {
    using is_POD = typename __type_traits<T1>::is_POD_type;
    __uninitialized_fill_aux(first, last, x, is_POD());
}

template <class ForwardIterator, class T>
inline void __uninitialized_fill_aux(ForwardIterator first, ForwardIterator last, const T& x, __true_type) {
    fill(first, last, x);
}

template <class ForwardIterator, class T>
void __uninitialized_fill_aux(ForwardIterator first, ForwardIterator last, const T& x, __false_type) {
    ForwardIterator cur = first;
    for (; cur != last; cur++) construct(&*cur, x);
}
