/**
 * @file hash_table.h
 * @brief 哈希表 —— 用于日记精确查找（O(1)平均查找）
 *
 * 核心数据结构：链地址法（Separate Chaining）哈希表
 *
 * 实现要点：
 *   - 使用链地址法处理冲突（每个桶挂一个链表）
 *   - 动态扩容：当装载因子超过阈值时自动扩容
 *   - 支持字符串键值对
 *
 * 时间复杂度：
 *   - 查找: O(1) 平均，O(n) 最坏（所有元素哈希冲突）
 *   - 插入: O(1) 平均
 *   - 删除: O(1) 平均
 *
 * 空间复杂度: O(n)
 *
 * 课程设计要求：自行实现，不用 std::unordered_map
 */

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <cstring>
#include <stdexcept>
#include <string>
#include <functional>

namespace algorithm {

// ============================================================
// 链表节点
// ============================================================
template <typename K, typename V>
struct HashNode {
    K key;
    V value;
    HashNode<K, V>* next;

    HashNode() : next(nullptr) {}
    HashNode(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
    HashNode(const K& k, V&& v) : key(k), value(static_cast<V&&>(v)), next(nullptr) {}
};

// ============================================================
// 哈希表
// ============================================================
template <typename K, typename V>
class HashMap {
private:
    // 链表桶
    struct Bucket {
        HashNode<K, V>* head;
        int count;

        Bucket() : head(nullptr), count(0) {}

        ~Bucket() {
            HashNode<K, V>* cur = head;
            while (cur) {
                HashNode<K, V>* next = cur->next;
                delete cur;
                cur = next;
            }
        }

        void clear() {
            HashNode<K, V>* cur = head;
            while (cur) {
                HashNode<K, V>* next = cur->next;
                delete cur;
                cur = next;
            }
            head = nullptr;
            count = 0;
        }
    };

    Bucket* buckets_;       // 桶数组
    int bucket_count_;      // 桶数量
    int size_;              // 元素数量
    double load_factor_;    // 装载因子阈值
    std::hash<K> hash_fn_;  // 哈希函数

    /// 哈希值取模
    int hash_to_index(const K& key) const {
        return static_cast<int>(hash_fn_(key) % static_cast<size_t>(bucket_count_));
    }

    /// 扩容
    void rehash(int new_bucket_count) {
        Bucket* old_buckets = buckets_;
        int old_count = bucket_count_;

        buckets_ = new Bucket[new_bucket_count];
        bucket_count_ = new_bucket_count;
        size_ = 0;

        // 重新插入所有元素（使用 move 避免 copy），并释放旧节点
        for (int i = 0; i < old_count; i++) {
            HashNode<K, V>* cur = old_buckets[i].head;
            old_buckets[i].head = nullptr;  // 防止 ~Bucket 重复释放已迁移的节点
            while (cur) {
                HashNode<K, V>* next = cur->next;
                insert(cur->key, static_cast<V&&>(cur->value));
                delete cur;
                cur = next;
            }
        }

        delete[] old_buckets;
    }

public:
    /// 构造函数
    explicit HashMap(int init_buckets = 16, double load_factor = 0.75)
        : bucket_count_(init_buckets), size_(0), load_factor_(load_factor) {
        buckets_ = new Bucket[bucket_count_];
    }

    ~HashMap() {
        delete[] buckets_;
    }

    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;
    HashMap& operator=(HashMap&& other) noexcept {
        if (this != &other) {
            delete[] buckets_;
            buckets_ = other.buckets_;
            bucket_count_ = other.bucket_count_;
            size_ = other.size_;
            load_factor_ = other.load_factor_;
            other.buckets_ = nullptr;
            other.bucket_count_ = 0;
            other.size_ = 0;
        }
        return *this;
    }

    /// 获取元素数量
    int size() const { return size_; }

    /// 是否为空
    bool empty() const { return size_ == 0; }

    /// 插入或更新键值对
    void insert(const K& key, const V& value) {
        // 检查是否需要扩容
        if (static_cast<double>(size_ + 1) / bucket_count_ > load_factor_) {
            rehash(bucket_count_ * 2);
        }

        int idx = hash_to_index(key);

        // 先查找是否已存在
        HashNode<K, V>* cur = buckets_[idx].head;
        while (cur) {
            if (cur->key == key) {
                cur->value = value;  // 更新（需要 V 支持拷贝赋值）
                return;
            }
            cur = cur->next;
        }

        // 不存在，插入到链表头部
        HashNode<K, V>* node = new HashNode<K, V>(key, value);
        node->next = buckets_[idx].head;
        buckets_[idx].head = node;
        buckets_[idx].count++;
        size_++;
    }

    /// 插入或更新键值对（move 版本，用于不可拷贝的值类型）
    void insert(const K& key, V&& value) {
        if (static_cast<double>(size_ + 1) / bucket_count_ > load_factor_) {
            rehash(bucket_count_ * 2);
        }

        int idx = hash_to_index(key);

        HashNode<K, V>* cur = buckets_[idx].head;
        while (cur) {
            if (cur->key == key) {
                cur->value = static_cast<V&&>(value);  // 移动更新
                return;
            }
            cur = cur->next;
        }

        HashNode<K, V>* node = new HashNode<K, V>(key, static_cast<V&&>(value));
        node->next = buckets_[idx].head;
        buckets_[idx].head = node;
        buckets_[idx].count++;
        size_++;
    }

    /// 查找
    /// @return 指向value的指针，未找到返回nullptr
    const V* find(const K& key) const {
        int idx = hash_to_index(key);
        HashNode<K, V>* cur = buckets_[idx].head;
        while (cur) {
            if (cur->key == key) return &cur->value;
            cur = cur->next;
        }
        return nullptr;
    }

    /// 可修改查找
    V* find(const K& key) {
        int idx = hash_to_index(key);
        HashNode<K, V>* cur = buckets_[idx].head;
        while (cur) {
            if (cur->key == key) return &cur->value;
            cur = cur->next;
        }
        return nullptr;
    }

    /// 是否包含键
    bool contains(const K& key) const {
        return find(key) != nullptr;
    }

    /// 删除
    bool erase(const K& key) {
        int idx = hash_to_index(key);
        HashNode<K, V>* cur = buckets_[idx].head;
        HashNode<K, V>* prev = nullptr;

        while (cur) {
            if (cur->key == key) {
                if (prev) {
                    prev->next = cur->next;
                } else {
                    buckets_[idx].head = cur->next;
                }
                delete cur;
                buckets_[idx].count--;
                size_--;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
        return false;
    }

    /// 清空
    void clear() {
        for (int i = 0; i < bucket_count_; i++) {
            buckets_[i].clear();
        }
        size_ = 0;
    }

    /// 获取所有键（用于遍历）
    /// @param keys 输出数组（调用者分配）
    /// @return 键的数量
    int get_all_keys(K* keys) const {
        int count = 0;
        for (int i = 0; i < bucket_count_; i++) {
            HashNode<K, V>* cur = buckets_[i].head;
            while (cur) {
                keys[count++] = cur->key;
                cur = cur->next;
            }
        }
        return count;
    }

    /// 装载因子
    double load_factor_value() const {
        return static_cast<double>(size_) / bucket_count_;
    }
};

// ============================================================
// 特化：整数键哈希表（常用场景）
// ============================================================
class IntHashMap {
private:
    struct Node {
        int key;
        int value;
        Node* next;
        Node(int k, int v) : key(k), value(v), next(nullptr) {}
    };

    struct Bucket {
        Node* head;
        Bucket() : head(nullptr) {}
        ~Bucket() {
            Node* c = head;
            while (c) { Node* n = c->next; delete c; c = n; }
        }
    };

    Bucket* buckets_;
    int bucket_count_;
    int size_;

    static unsigned int hash_int(int key, int bc) {
        unsigned int h = static_cast<unsigned int>(key);
        h = ((h >> 16) ^ h) * 0x45d9f3b;
        h = ((h >> 16) ^ h) * 0x45d9f3b;
        h = (h >> 16) ^ h;
        return h % static_cast<unsigned int>(bc);
    }

public:
    explicit IntHashMap(int bc = 16) : bucket_count_(bc), size_(0) {
        buckets_ = new Bucket[bucket_count_];
    }
    ~IntHashMap() { delete[] buckets_; }

    IntHashMap(const IntHashMap&) = delete;
    IntHashMap& operator=(const IntHashMap&) = delete;

    IntHashMap(IntHashMap&& other) noexcept
        : buckets_(other.buckets_),
          bucket_count_(other.bucket_count_),
          size_(other.size_) {
        other.buckets_ = nullptr;
        other.bucket_count_ = 0;
        other.size_ = 0;
    }

    IntHashMap& operator=(IntHashMap&& other) noexcept {
        if (this != &other) {
            delete[] buckets_;
            buckets_ = other.buckets_;
            bucket_count_ = other.bucket_count_;
            size_ = other.size_;
            other.buckets_ = nullptr;
            other.bucket_count_ = 0;
            other.size_ = 0;
        }
        return *this;
    }

    void put(int key, int value) {
        unsigned int idx = hash_int(key, bucket_count_);
        Node* c = buckets_[idx].head;
        while (c) {
            if (c->key == key) { c->value = value; return; }
            c = c->next;
        }
        Node* n = new Node(key, value);
        n->next = buckets_[idx].head;
        buckets_[idx].head = n;
        size_++;
    }

    int* get(int key) {
        unsigned int idx = hash_int(key, bucket_count_);
        Node* c = buckets_[idx].head;
        while (c) {
            if (c->key == key) return &c->value;
            c = c->next;
        }
        return nullptr;
    }

    bool contains(int key) const {
        unsigned int idx = hash_int(key, bucket_count_);
        Node* c = buckets_[idx].head;
        while (c) {
            if (c->key == key) return true;
            c = c->next;
        }
        return false;
    }

    int size() const { return size_; }

    // STL 风格迭代器支持（简化版）
    struct Iterator {
        int key;
        int value;
        bool valid;
    };

    Iterator find(int key) {
        unsigned int idx = hash_int(key, bucket_count_);
        Node* c = buckets_[idx].head;
        while (c) {
            if (c->key == key) return {c->key, c->value, true};
            c = c->next;
        }
        return {0, 0, false};
    }

    static constexpr Iterator end() { return {0, 0, false}; }
};

} // namespace algorithm

#endif // HASH_TABLE_H
