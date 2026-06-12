/**
 * @file heap.h
 * @brief 最大堆/最小堆 —— 用于Top-K推荐（不完全排序获取前K个元素）
 *
 * 核心算法思想：
 *   - 维护一个大小为K的堆，遍历所有元素
 *   - 最大堆用于"最小的K个"（如最近距离），最小堆用于"最大的K个"（如最高热度）
 *   - 复杂度 O(n + k*log n)，远优于完全排序 O(n*log n)
 *
 * 课程设计要求：
 *   - 堆结构自行实现（数组存储，不用STL priority_queue）
 *   - 支持模板泛型，可对任意可比较类型进行堆操作
 *
 * 时间复杂度：
 *   - 建堆: O(n)（自底向上heapify）
 *   - 插入: O(log n)
 *   - 取顶: O(1)
 *   - 弹出顶: O(log n)
 *   - Top-K: O(n*log k)
 */

#ifndef HEAP_H
#define HEAP_H

#include <functional>
#include <stdexcept>

namespace algorithm {

/**
 * @brief 泛型堆实现
 * @tparam T 元素类型
 * @tparam Compare 比较器类型（默认 std::less<T> 构建最大堆）
 *
 * 内部使用数组存储，从索引1开始（索引0不使用），
 * 父节点 = i/2, 左子 = 2*i, 右子 = 2*i+1
 */
template <typename T, typename Compare = std::less<T>>
class Heap {
private:
    T* data_;          // 数据数组
    int size_;         // 当前元素个数
    int capacity_;     // 数组容量
    Compare comp_;     // 比较函数

    void expand() {
        int new_cap = capacity_ == 0 ? 16 : capacity_ * 2;
        T* new_data = new T[new_cap];
        // 堆使用1-based索引，有效元素在 data_[1..size_]，必须复制到 size_（含）
        for (int i = 0; i <= size_; i++) {
            new_data[i] = data_[i];
        }
        delete[] data_;
        data_ = new_data;
        capacity_ = new_cap;
    }

    /// 上浮操作（插入后维护堆性质）
    void sift_up(int idx) {
        while (idx > 1) {
            int parent = idx / 2;
            if (comp_(data_[parent], data_[idx])) {
                // 父节点比当前节点"小"（对于less，即父<子），需要交换
                T tmp = data_[parent];
                data_[parent] = data_[idx];
                data_[idx] = tmp;
                idx = parent;
            } else {
                break;
            }
        }
    }

    /// 下沉操作（弹出后维护堆性质）
    void sift_down(int idx) {
        while (2 * idx <= size_) {
            int child = 2 * idx;
            // 选较大的子节点
            if (child + 1 <= size_ && comp_(data_[child], data_[child + 1])) {
                child++;
            }
            if (comp_(data_[idx], data_[child])) {
                T tmp = data_[idx];
                data_[idx] = data_[child];
                data_[child] = tmp;
                idx = child;
            } else {
                break;
            }
        }
    }

public:
    Heap() : data_(nullptr), size_(0), capacity_(0), comp_() {}

    ~Heap() { delete[] data_; }

    Heap(const Heap&) = delete;
    Heap& operator=(const Heap&) = delete;

    /// 用初始容量构造
    explicit Heap(int init_capacity) : data_(nullptr), size_(0), capacity_(0), comp_() {
        if (init_capacity > 0) {
            capacity_ = init_capacity + 1;  // 索引0不使用
            data_ = new T[capacity_];
        }
    }

    /// 用自定义比较器构造
    Heap(const Compare& comp) : data_(nullptr), size_(0), capacity_(0), comp_(comp) {}

    /// 向上转型：显式指定堆类型
    /// MaxHeap = Heap<T, std::less<T>>
    /// MinHeap = Heap<T, std::greater<T>>

    int size() const { return size_; }
    bool empty() const { return size_ == 0; }

    /// 获取堆顶元素
    const T& top() const {
        if (size_ == 0) throw std::out_of_range("Heap::top: heap is empty");
        return data_[1];
    }

    /// 插入元素
    void push(const T& val) {
        if (size_ + 1 >= capacity_) expand();
        data_[++size_] = val;
        sift_up(size_);
    }

    /// 弹出堆顶
    T pop() {
        if (size_ == 0) throw std::out_of_range("Heap::pop: heap is empty");
        T top_val = data_[1];
        data_[1] = data_[size_--];
        sift_down(1);
        return top_val;
    }

    /// 用数组建堆（O(n)时间）
    void build_heap(const T* arr, int n) {
        if (n + 1 >= capacity_) {
            delete[] data_;
            capacity_ = n + 1;
            data_ = new T[capacity_];
        }
        for (int i = 0; i < n; i++) {
            data_[i + 1] = arr[i];
        }
        size_ = n;
        // 自底向上heapify
        for (int i = size_ / 2; i >= 1; i--) {
            sift_down(i);
        }
    }

    /// 清空堆
    void clear() { size_ = 0; }
};

// ============================================================
// Top-K 算法
// ============================================================

/**
 * @brief 从数组中找出最大的K个元素
 *
 * 算法思想：
 *   1. 建立一个大小为K的**最小堆**
 *   2. 遍历所有元素，若当前元素 > 堆顶，则替换堆顶并调整
 *   3. 最终堆中即为最大的K个元素
 *
 * 时间复杂度: O(n * log k)
 * 空间复杂度: O(k)
 *
 * @tparam T 元素类型
 * @param data 数据数组
 * @param n 数据个数
 * @param k 要取的前K个
 * @param result 输出结果数组（调用者分配，至少k个元素）
 * @return 实际返回的元素个数（min(k, n)）
 */
template <typename T>
int top_k_largest(const T* data, int n, int k, T* result) {
    if (k <= 0 || n <= 0) return 0;
    k = (k < n) ? k : n;

    // 最小堆（std::greater -> 堆顶是最小值）
    Heap<T, std::greater<T>> min_heap(k + 1);

    // 前 k 个元素入堆
    for (int i = 0; i < k; i++) {
        min_heap.push(data[i]);
    }

    // 遍历剩余元素
    for (int i = k; i < n; i++) {
        if (data[i] > min_heap.top()) {
            min_heap.pop();
            min_heap.push(data[i]);
        }
    }

    // 倒序弹出即为从大到小
    for (int i = k - 1; i >= 0; i--) {
        result[i] = min_heap.pop();
    }

    return k;
}

/**
 * @brief 从数组中找出最小的K个元素
 *
 * 算法思想：
 *   1. 建立一个大小为K的**最大堆**
 *   2. 遍历所有元素，若当前元素 < 堆顶，则替换堆顶并调整
 *
 * 时间复杂度: O(n * log k)
 *
 * @tparam T 元素类型
 */
template <typename T>
int top_k_smallest(const T* data, int n, int k, T* result) {
    if (k <= 0 || n <= 0) return 0;
    k = (k < n) ? k : n;

    // 最大堆（std::less -> 堆顶是最大值）
    Heap<T, std::less<T>> max_heap(k + 1);

    for (int i = 0; i < k; i++) {
        max_heap.push(data[i]);
    }

    for (int i = k; i < n; i++) {
        if (data[i] < max_heap.top()) {
            max_heap.pop();
            max_heap.push(data[i]);
        }
    }

    // 倒序弹出即为从小到大
    for (int i = k - 1; i >= 0; i--) {
        result[i] = max_heap.pop();
    }

    return k;
}

/**
 * @brief 带权重的 Top-K（用于推荐系统，元素带分数）
 *
 * @tparam T 元素类型（需要有 double score() 方法或通过比较器访问分数）
 */
template <typename T, typename ScoreFunc>
int top_k_by_score(const T* data, int n, int k, T* result, ScoreFunc get_score, bool descending = true) {
    if (k <= 0 || n <= 0) return 0;
    k = (k < n) ? k : n;

    if (descending) {
        // 取分数最高的K个 -> 用最小堆
        struct ScoredItem {
            T item;
            double score;
            bool operator>(const ScoredItem& other) const { return score > other.score; }
            bool operator<(const ScoredItem& other) const { return score < other.score; }
        };

        Heap<ScoredItem, std::greater<ScoredItem>> min_heap(k + 1);

        for (int i = 0; i < k; i++) {
            ScoredItem si;
            si.item = data[i];
            si.score = get_score(data[i]);
            min_heap.push(si);
        }

        for (int i = k; i < n; i++) {
            double s = get_score(data[i]);
            if (s > min_heap.top().score) {
                min_heap.pop();
                ScoredItem si;
                si.item = data[i];
                si.score = s;
                min_heap.push(si);
            }
        }

        for (int i = k - 1; i >= 0; i--) {
            result[i] = min_heap.pop().item;
        }
    } else {
        // 取分数最低的K个 -> 用最大堆
        struct ScoredItem {
            T item;
            double score;
            bool operator>(const ScoredItem& other) const { return score > other.score; }
            bool operator<(const ScoredItem& other) const { return score < other.score; }
        };

        Heap<ScoredItem, std::less<ScoredItem>> max_heap(k + 1);

        for (int i = 0; i < k; i++) {
            ScoredItem si;
            si.item = data[i];
            si.score = get_score(data[i]);
            max_heap.push(si);
        }

        for (int i = k; i < n; i++) {
            double s = get_score(data[i]);
            if (s < max_heap.top().score) {
                max_heap.pop();
                ScoredItem si;
                si.item = data[i];
                si.score = s;
                max_heap.push(si);
            }
        }

        for (int i = k - 1; i >= 0; i--) {
            result[i] = max_heap.pop().item;
        }
    }

    return k;
}

} // namespace algorithm

#endif // HEAP_H
