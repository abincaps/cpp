#pragma once

// 希望表达式能返回真或假, 以决定采取什么策略
// 但不应该返回 bool值, 而应该返回一个具有真/假性质的对象
// 利用两种不同的类型选择不同的重载函数， 而bool 属于一种类型

// C++模板元编程是一种利用模板和编译时的实例化展开机制实现编译期计算和参数推导的技术
// 通过在编译期通过模板展开产生代码，可以实现编译期参数推导

struct __true_type {};
struct __false_type {};

template <typename type>
struct __type_traits {
    using this_dummy_member_must_be_first = __true_type;
    // 该类型是否有平凡默认构造函数
    using has_trivial_default_constructor = __false_type;
    // 该类型是否有平凡复制（拷贝）构造函数
    using has_trivial_copy_constructor = __false_type;
    // 该类型是否有平凡赋值运算符
    using has_trivial_assignment_operator = __false_type;
    // 该类型是否有平凡析构运算符
    using has_trivial_destructor = __false_type;
    // 只有聚合类型(像struct和array)才可能是POD。
    // 只包含内置类型和其他POD作为成员。
    // 没有自定义构造函数、析构函数、拷贝函数等。
    // 没有基类且不能派生子类
    using is_POD_type = __false_type;
};

// 一个默认构造函数被认为是平凡的
// 不接受任何参数
// 函数体里没有内容 也就是没有任何初始化工作

// 其余平凡函数类似

// 全特化 对模板的全部参数进行明确指定的特化
// 下面都是基本类型
template <>
struct __type_traits<char> {
    using has_trivial_default_constructor = __true_type;
    using has_trivial_copy_constructor = __true_type;
    using has_trivial_assignment_operator = __true_type;
    using has_trivial_destructor = __true_type;
    using is_POD_type = __true_type;
};

template <>
struct __type_traits<signed char> {
    using has_trivial_default_constructor = __true_type;
    using has_trivial_copy_constructor = __true_type;
    using has_trivial_assignment_operator = __true_type;
    using has_trivial_destructor = __true_type;
    using is_POD_type = __true_type;
};

template <>
struct __type_traits<unsigned char> {
    using has_trivial_default_constructor = __true_type;
    using has_trivial_copy_constructor = __true_type;
    using has_trivial_assignment_operator = __true_type;
    using has_trivial_destructor = __true_type;
    using is_POD_type = __true_type;
};

template <>
struct __type_traits<short> {
    using has_trivial_default_constructor = __true_type;
    using has_trivial_copy_constructor = __true_type;
    using has_trivial_assignment_operator = __true_type;
    using has_trivial_destructor = __true_type;
    using is_POD_type = __true_type;
};

template <>
struct __type_traits<unsigned short> {
    using has_trivial_default_constructor = __true_type;
    using has_trivial_copy_constructor = __true_type;
    using has_trivial_assignment_operator = __true_type;
    using has_trivial_destructor = __true_type;
    using is_POD_type = __true_type;
};

template <>
struct __type_traits<int> {
    using has_trivial_default_constructor = __true_type;
    using has_trivial_copy_constructor = __true_type;
    using has_trivial_assignment_operator = __true_type;
    using has_trivial_destructor = __true_type;
    using is_POD_type = __true_type;
};

template <>
struct __type_traits<unsigned int> {
    using has_trivial_default_constructor = __true_type;
    using has_trivial_copy_constructor = __true_type;
    using has_trivial_assignment_operator = __true_type;
    using has_trivial_destructor = __true_type;
    using is_POD_type = __true_type;
};

template <>
struct __type_traits<long> {
    using has_trivial_default_constructor = __true_type;
    using has_trivial_copy_constructor = __true_type;
    using has_trivial_assignment_operator = __true_type;
    using has_trivial_destructor = __true_type;
    using is_POD_type = __true_type;
};

template <>
struct __type_traits<unsigned long> {
    using has_trivial_default_constructor = __true_type;
    using has_trivial_copy_constructor = __true_type;
    using has_trivial_assignment_operator = __true_type;
    using has_trivial_destructor = __true_type;
    using is_POD_type = __true_type;
};

template <>
struct __type_traits<float> {
    using has_trivial_default_constructor = __true_type;
    using has_trivial_copy_constructor = __true_type;
    using has_trivial_assignment_operator = __true_type;
    using has_trivial_destructor = __true_type;
    using is_POD_type = __true_type;
};

template <>
struct __type_traits<double> {
    using has_trivial_default_constructor = __true_type;
    using has_trivial_copy_constructor = __true_type;
    using has_trivial_assignment_operator = __true_type;
    using has_trivial_destructor = __true_type;
    using is_POD_type = __true_type;
};

template <>
struct __type_traits<long double> {
    using has_trivial_default_constructor = __true_type;
    using has_trivial_copy_constructor = __true_type;
    using has_trivial_assignment_operator = __true_type;
    using has_trivial_destructor = __true_type;
    using is_POD_type = __true_type;
};

// 原生指针偏特化 允许对模板进行部分参数特化, 而不是完全特化
// 必须放在原模板声明后才能被识别
template <typename T>
struct __type_traits<T*> {
    using has_trivial_default_constructor = __true_type;
    using has_trivial_copy_constructor = __true_type;
    using has_trivial_assignment_operator = __true_type;
    using has_trivial_destructor = __true_type;
    using is_POD_type = __true_type;
};

// 基本(内置）类型都是__true_type, 而对象都是__false_type