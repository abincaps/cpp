// class Alloc = alloc 默认参数 自定义的分配器类型
template <typename T, typename Alloc = alloc>
class vector {
public:
    // vector<T> 类型
    using value_type = T;
    using pointer = value_type*;
    
    using iterator = value_type*; // 定义迭代器
    using reference = value_type&;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

protected:
    using data_allocator = simple_alloc<value_type, Alloc>;  // 空间配置器接口，传入元素类型value_type 和选择的配置器Alloc
    iterator start;                                          // 表示目前使用空间的头
    iterator finish;                                         // 表示目前使用空间的尾 
    iterator end_of_storage;                                 // 表示目前备用空间的尾 （也就是实际空间的尾）

    void insert_aux(iterator position, const T& x);

    void deallocate() {
        // end_of_storage - start 从实际分配的空间尾部 - 实际分配的空间头部
        if (start) {
            // 把内存归还给free list 或者 free掉
            data_allocator::deallocate(start, end_of_storage - start);
        }
    }

    void fill_initialize(size_type n, const T& value) {
        start = allocate_and_fill(n, value);
        finish = start + n;
        end_of_storage = finish;
    }

public:
    // 迭代器首尾  [first, end) 半闭半开
    iterator begin() { return start; }
    iterator end() { return finish; }

    // 返回容器中元素个数 end() - begin() 指针相减返回的是个数 O(1)
    size_type size() const { return size_type(end() - begin()); }

    // 返回 容器实际大小（容量）O(1)
    size_type capacity() const { return size_type(end_of_storage - begin()); }

    // 判空 O(1)
    bool empty() const { return begin() == end(); }

    // 重载operator[]运算符 根据给定的索引 n, 返回容器中对应位置的元素引用
    reference operator[](size_type n) { return *(begin() + n); }

    // 构造函数   初始化列表
    vector() : start(nullptr), finish(nullptr), end_of_storage(nullptr) {}

    // 初始化n个值为value
    vector(size_type n, const T& value) { fill_initialize(n, value); }
    vector(int n, const T& value) { fill_initialize(n, value); }
    vector(long n, const T& value) { fill_initialize(n, value); }

    // 初始化n个默认值
    // explicit 禁用隐式转换
    explicit vector(size_type n) { fill_initialize(n, T()); }

    ~vector() {
        // 先析构掉内存上的对象
        destroy(start, finish);
        // 然后把内存归还给free list 或者 free掉
        // 这里调用的是vector的成员函数
        deallocate();
    }

    reference front() { return *begin(); }     // 返回第一个元素 是引用类型 可以修改
    reference back() { return *(end() - 1); }  // 返回最后一个元素的引用

    // 将元素x插入尾部 O(1)
    void push_back(const T& x) {
        // 还有可以用的备用空间
        if (finish != end_of_storage) {
            construct(finish, x);  // 调用stl_construct.h中的construct全域函数
            finish++;
        } else
            insert_aux(end(), x);
    }

    // 将末端元素弹出(取出) O(1)
    void pop_back() {
        // 析构对象 但不释放内存
        destroy(finish);  // stl_construct.h中的destroy全域函数
    }

    // 删除迭代器所指位置上的元素 O(n)
    iterator erase(iterator position) {
        if (position + 1 != end()) copy(position + 1, finish, position);  // 後續元素往前搬移
        --finish;
        destroy(finish);
        return position;
    }

    // 	改变容器中可存储元素的个数
    void resize(size_type new_size, const T& x) {
        // 如果新大小比之前小 就删除末尾多余的元素
        if (new_size < size()) {
            erase(begin() + new_size, end());
        } else {
            // 如果新大小比之前大 就在末尾插入 元素类型为T的 x
            insert(end(), new_size - size(), x);
        }
    }

    // 	改变容器中可存储元素的个数  T() 默认值
    void resize(size_type new_size) { resize(new_size, T()); }

    // 清空容器内所有元素
    void clear() { erase(begin(), end()); }

    void insert(iterator position, size_type n, const T& x);

protected:
    iterator allocate_and_fill(size_type n, const T& x) {
        // 这里默认调用第二级配置器 分配空间
        iterator res = data_allocator::allocate(n);
        // 调用 simple_alloc<value_type, Alloc> 中的 allocate
        // 然后 n * sizeof(value_type);

        uninitialized_fill_n(res, n, x);
        // 返回的是配置空间的起始位置
        return res;
    }
}

// 在position位置上插入一个x
template <typename T, typename Alloc>
void vector<T, Alloc>::insert_aux(iterator position, const T& x) {
    // 还有备用空间
    if (finish != end_of_storage) {
        // TODO
        // construct(finish, *(finish - 1));
        // ++finish;
        // T x_copy = x;
        // copy_backward(position, finish - 2, finish - 1);
        // *position = x_copy;

    } else {
        // 没有可以用的备用空间
        const size_type old_size = size();  // 记录原来的大小

        // 如果原大小等于0，就分配一个元素大小
        // 如果原大小不为0，则分配原大小的两倍
        const size_type len = old_size != 0 ? 2 * old_size : 1;

        // 前半段用来放原来的， 后半段放新插入的
        // 调用配置器分配新的内存
        iterator new_start = data_allocator::allocate(len);

        iterator new_finish = new_start;

        try {
            // 要在position位置插入数据， 则position前面的数据是原封不动搬到新内存区域
            // 将原来的 [start, position) 区域 拷贝到新的内存区域
            // 这里position是原来的 end()
            new_finish = uninitialized_copy(start, position, new_start);
            // 在new_finish位置上构造新元素x
            construct(new_finish, x);
            // 尾巴后移一个
            new_finish++;

            // 将原空间的备用空间内容也拷贝过来
            // new_finish = uninitialized_copy(position, finish, new_finish);

            // 三个点表示可以捕获任意类型的异常
        } catch (...) {
            // 析构掉
            destroy(new_start, new_finish);
            // 释放空间
            data_allocator::deallocate(new_start, len);
            throw;
        }

        // 析构原来的空间
        destroy(begin(), end());
        // 释放原来的空间
        deallocate();

        // 调整迭代器指针
        start = new_start;
        finish = new_finish;
        end_of_storage = new_start + len;
    }
}

// 在position的位置插入n个x
template <class T, class Alloc>
void vector<T, Alloc>::insert(iterator position, size_type n, const T& x) {
    
    if (0 != n) {
        // 备用空间大于新增的个数 TODO
        if (size_type(end_of_storage - finish) >= n) {

            T x_copy = x;

            
            const size_type elems_after = finish - position; // 插入位置到结尾的距离

            
            iterator old_finish = finish;

            // 大于要插入的个数 n
            if (elems_after > n) {
  
                // uninitialized_copy(finish - n, finish, finish);
                // finish += n;
                // copy_backward(position, old_finish - n, old_finish);
                // fill(position, position + n, x_copy);

            } else {

                // 先填充finish
                uninitialized_fill_n(finish, n - elems_after, x_copy);
                finish += n - elems_after;
                uninitialized_copy(position, old_finish, finish);
                finish += elems_after;
                std::fill(position, old_finish, x_copy);
            }

        } else {

            // 重新申请的空间 = max（当前两倍的空间，当前的空间 + 插入所需的空间）
            // 也就等于 当前的空间 + max(当前的空间， 插入所需的空间)

            const size_type old_size = size();
            // 这里的max TODO
            const size_type len = old_size + std::max(old_size, n);

            iterator new_start = data_allocator::allocate(len);
            iterator new_finish = new_start;

            try {
                // 将原来的 [start, position) 区域 拷贝到新的内存区域
                new_finish = uninitialized_copy(start, position, new_start);
                // 从new_finish开始填充n个值为x的元素
                new_finish = uninitialized_fill_n(new_finish, n, x);
                // 将[positon, finish) 移动到n个被插入元素的后面
                new_finish = uninitialized_copy(position, finish, new_finish);
            }
            catch (...) {

                destroy(new_start, new_finish);
                data_allocator::deallocate(new_start, len);
                throw;
            }

            destroy(start, finish);
            deallocate();

            start = new_start;
            finish = new_finish;
            end_of_storage = new_start + len;
        }
    }
}
