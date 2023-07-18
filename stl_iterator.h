#include <cstddef>
// 输入迭代器 只能读入
struct input_iterator_tag {};
// 输出迭代器 只能写入
struct output_iterator_tag {};
// 前向迭代器 继承自输入迭代器 支持读写操作
struct forward_iterator_tag : public input_iterator_tag {};
// 双向迭代器 继承自前向迭代器 增加了反向遍历
struct bidirectional_iterator_tag : public forward_iterator_tag {};
// 随机访问迭代器 继承自双向迭代器 增加了随机访问
struct random_access_iterator_tag : public bidirectional_iterator_tag {};

// traits萃取器 用来为同一类数据提供统一的操作函数
template <typename Iterator>
struct iterator_traits {
    // 本质上通过指定Iterator 来获取 Iterator：：作用域中的类型名， 整体看作
    // typename Iterator::iterator_category是嵌套依赖名 如果不使用typename,
    // 编译器会认为Iterator::iterator是一个静态成员, 从而导致编译错误。
    using iterator_category = typename Iterator::iterator_category;  // 迭代器类型
    using value_type = typename Iterator::value_type;                // 迭代器指向元素的类型
    using difference_type = typename Iterator::difference_type;      // 迭代器距离的类型
    using pointer = typename Iterator::pointer;                      // 迭代器指向元素的类型指针
    using reference = typename Iterator::reference;                  // 迭代器指向元素的类型引用
};

// 偏特化允许为模板的部分参数组合提供定制的实现,而不必要完全特化每一种情况
// 仅针对部分参数进行定制 提供比原模板更优化的实现
// 对原生指针设计的偏特化版
template <typename T>
struct iterator_traits<T*> {
    using iterator_category = random_access_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
};

// const T*的偏特化版
template <typename T>
struct iterator_traits<const T*> {
    using iterator_category = random_access_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
};

// 确定该迭代器的 category
template <typename Iterator>
// 返回值是迭代器类型 参数const Iterator& itr决定了Iterator的实际类型
inline typename iterator_traits<Iterator>::iterator_category iterator_category(const Iterator&) {
    using category = typename iterator_traits<Iterator>::iterator_category;
    // 调用category() category是 struct .. tag 类型
    // 对于一个struct 如果里面都没有声明构造函数 那么该struct就具有一个默认生成的无参构造函数
    return category();
}

// 确定该迭代器的 difference_type
template <typename Iterator>
inline typename iterator_traits<Iterator>::difference_type* difference_type(const Iterator&) {
    // 通过萃取器获取类型 静态转换空指针 通过指针来确定类型
    return static_cast<typename iterator_traits<Iterator>::difference_type*>(nullptr);
}

// 确定该迭代器的 value type
template <typename Iterator>
inline typename iterator_traits<Iterator>::value_type* value_type(const Iterator&) {
    // 同 difference_type
    return static_cast<typename iterator_traits<Iterator>::value_type*>(nullptr);
}

// 根据迭代器类型执行其对应的__distance()
template <typename InputIterator>
inline typename iterator_traits<InputIterator>::difference_type __distance(InputIterator first, InputIterator last,
                                                                           input_iterator_tag) {
    // iterator_traits<InputIterator>::difference_type 是一个依赖类型名
    // 如果不使用 typename,C++编译器会将其视为一个静态值而不是一个类型
    typename iterator_traits<InputIterator>::difference_type dis = 0;

    // 计算两个迭代器之间的距离
    while (first != last) {
        first++, dis++;
    }

    return dis;
}

template <typename RandomAccessIterator>
inline typename iterator_traits<RandomAccessIterator>::difference_type __distance(RandomAccessIterator first,
                                                                                  RandomAccessIterator last,
                                                                                  random_access_iterator_tag) {
    // 随机访问内存空间是连续的 直接 last - first
    return last - first;
}

// 通过distance调用对应迭代器类型的版本
template <class InputIterator>
inline typename iterator_traits<InputIterator>::difference_type distance(InputIterator first, InputIterator last) {
    // 通过category来调用对应的__distance重载版本
    return __distance(first, last, iterator_category(first));
}

// 输入迭代器 i 移动 n个位置
template <typename InputIterator, typename Distance>
inline void __advance(InputIterator& i, Distance n, input_iterator_tag) {
    while (n--) i++;
}

template <typename BidirectionalIterator, typename Distance>
inline void __advance(BidirectionalIterator& i, Distance n, bidirectional_iterator_tag) {
    // 判断正向移动还是反向移动
    if (n >= 0)
        while (n--) i++;
    else
        while (n++) i--;
}

template <typename RandomAccessIterator, typename Distance>
inline void __advance(RandomAccessIterator& i, Distance n, random_access_iterator_tag) {
    // 随机访问内存空间是连续的 直接 i + n n正负都可以
    i += n;
}

// 通过advance调用对应迭代器类型的版本
// 输入迭代器 i 移动 n 个位置
template <typename InputIterator, typename Distance>
inline void advance(InputIterator& i, Distance n) {
    __advance(i, n, iterator_category(i));
}
