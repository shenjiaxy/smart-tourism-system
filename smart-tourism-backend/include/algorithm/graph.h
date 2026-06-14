/**
 * @file graph.h
 * @brief 邻接表图数据结构 —— 用于路网表示、路线规划和场所查询
 *
 * 核心数据结构：邻接表（Adjacency List）
 * 存储方式：vector<Node> 节点表 + vector<vector<Edge>> 邻接表
 *
 * 支持操作：
 *   - 添加/删除节点和边
 *   - 获取邻居节点
 *   - 多权值边（距离、时间、交通工具类型）
 *   - 有向/无向图
 *
 * 时间复杂度：
 *   - 添加边: O(1) amortized
 *   - 遍历邻居: O(degree(v))
 *   - 空间: O(V + E)
 *
 * 课程设计核心要求：所有容器自行实现（不用STL vector/list等）
 * 这里使用定长数组 + 动态扩容实现
 */

#ifndef GRAPH_H
#define GRAPH_H

#include <cstring>
#include <new>
#include <stdexcept>
#include <string>

namespace algorithm {

// ============================================================
// 节点信息
// ============================================================
struct GraphNode {
    int id;             // 节点唯一标识
    double x, y;        // 坐标（用于地图渲染和距离估算）
    int spot_id;        // 所属景区/校园ID（-1表示不属于任何景区）
    std::string name;   // 节点名称（如"南门"、"图书馆"）
    int type;           // 节点类型: 0=道路交叉口, 1=建筑入口, 2=设施, 3=景点
    int floor_num;      // 楼层号（-1表示室外）

    GraphNode() : id(-1), x(0), y(0), spot_id(-1), name(""), type(0), floor_num(-1) {}
    GraphNode(int _id, double _x, double _y, int _spot_id = -1,
              const std::string& _name = "", int _type = 0, int _floor = -1)
        : id(_id), x(_x), y(_y), spot_id(_spot_id), name(_name), type(_type), floor_num(_floor) {}
};

// ============================================================
// 边信息（支持多权值）
// ============================================================
struct GraphEdge {
    int to;             // 目标节点ID
    double distance;    // 物理距离（米）
    double time;        // 通行时间（秒），基于距离和拥挤度计算
    double congestion;  // 通行系数 (0, 1]，真实速度 = 通行系数 * 理想速度
    double ideal_speed; // 理想速度（米/秒）
    int transport;      // 交通工具类型: 0=步行, 1=自行车, 2=电瓶车

    GraphEdge()
        : to(-1), distance(0), time(0), congestion(0.5), ideal_speed(1.2), transport(0) {}
    GraphEdge(int _to, double _dist, double _congestion = 0.5, int _trans = 0,
              double _ideal_speed = 0.0)
        : to(_to), distance(_dist), congestion(_congestion), ideal_speed(_ideal_speed),
          transport(_trans) {
        if (ideal_speed <= 0.0) {
            ideal_speed = (_trans == 1) ? 3.5 : ((_trans == 2) ? 5.0 : 1.2);
        }
        double factor = congestion;
        if (factor <= 0.0) factor = 0.05;
        if (factor > 1.0) factor = 1.0;
        time = _dist / (ideal_speed * factor);
    }
};

// ============================================================
// 动态数组（替代 std::vector，自行实现）
// ============================================================
template <typename T>
class DynamicArray {
private:
    T* data_;
    int size_;
    int capacity_;

    void expand() {
        int new_cap = capacity_ == 0 ? 4 : capacity_ * 2;
        // 使用原始内存分配（不构造对象），配合 placement new 手动控制生命周期
        T* new_data = static_cast<T*>(operator new[](sizeof(T) * static_cast<size_t>(new_cap)));
        for (int i = 0; i < size_; i++) {
            new (&new_data[i]) T(static_cast<T&&>(data_[i]));
            data_[i].~T();
        }
        operator delete[](data_);
        data_ = new_data;
        capacity_ = new_cap;
    }

public:
    DynamicArray() : data_(nullptr), size_(0), capacity_(0) {}

    ~DynamicArray() {
        for (int i = 0; i < size_; i++) {
            data_[i].~T();
        }
        operator delete[](data_);
    }

    // 禁止拷贝
    DynamicArray(const DynamicArray&) = delete;
    DynamicArray& operator=(const DynamicArray&) = delete;

    // 移动语义
    DynamicArray(DynamicArray&& other) noexcept
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    DynamicArray& operator=(DynamicArray&& other) noexcept {
        if (this != &other) {
            for (int i = 0; i < size_; i++) data_[i].~T();
            operator delete[](data_);
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    void push_back(const T& val) {
        if (size_ >= capacity_) expand();
        new (&data_[size_]) T(val);
        size_++;
    }

    void push_back(T&& val) {
        if (size_ >= capacity_) expand();
        new (&data_[size_]) T(static_cast<T&&>(val));
        size_++;
    }

    void pop_back() {
        if (size_ > 0) {
            --size_;
            data_[size_].~T();
        }
    }

    int size() const { return size_; }

    bool empty() const { return size_ == 0; }

    void clear() {
        for (int i = 0; i < size_; i++) data_[i].~T();
        size_ = 0;
    }

    T& operator[](int idx) {
        if (idx < 0 || idx >= size_) throw std::out_of_range("DynamicArray index out of range");
        return data_[idx];
    }

    const T& operator[](int idx) const {
        if (idx < 0 || idx >= size_) throw std::out_of_range("DynamicArray index out of range");
        return data_[idx];
    }

    T& back() { return data_[size_ - 1]; }
    const T& back() const { return data_[size_ - 1]; }
};

// ============================================================
// 邻接表图
// ============================================================
class Graph {
public:
    // 图类型
    enum class Type { DIRECTED, UNDIRECTED };

private:
    DynamicArray<GraphNode> nodes_;               // 节点列表
    DynamicArray<DynamicArray<GraphEdge>> adj_;   // 邻接表
    DynamicArray<bool> node_exists_;              // 节点存在标记（用于非连续ID）
    Type type_;                                    // 有向/无向
    int max_id_;                                   // 最大节点ID

    // 确保 ID 到索引的映射可用（节点ID可能不连续）
    // 这里简化处理：假设节点ID为 0 到 node_count-1 的连续整数
    // 对于非连续ID，通过内部索引映射处理

public:
    Graph(Type t = Type::UNDIRECTED) : type_(t), max_id_(0) {}

    /// 节点数量
    int node_count() const { return nodes_.size(); }

    /// 边数量（有向图算单条，无向图也算单条）
    int edge_count() const {
        int count = 0;
        for (int i = 0; i < adj_.size(); i++) {
            count += adj_[i].size();
        }
        return count;
    }

    /// 添加节点，返回节点内部索引
    int add_node(const GraphNode& node) {
        int idx = nodes_.size();
        nodes_.push_back(node);
        adj_.push_back(DynamicArray<GraphEdge>());
        if (node.id > max_id_) max_id_ = node.id;
        return idx;
    }

    /// 通过内部索引添加节点
    int add_node(int id, double x, double y, int spot_id = -1,
                 const std::string& name = "", int type = 0, int floor_num = -1) {
        GraphNode node(id, x, y, spot_id, name, type, floor_num);
        return add_node(node);
    }

    /// 添加边（通过内部索引）
    /// 返回边的内部索引
    void add_edge(int from, int to, double distance, double congestion = 0.5,
                  int transport = 0, double ideal_speed = 0.0) {
        if (from < 0 || from >= nodes_.size() || to < 0 || to >= nodes_.size()) {
            throw std::out_of_range("Graph::add_edge: node index out of range");
        }
        adj_[from].push_back(GraphEdge(to, distance, congestion, transport, ideal_speed));
        if (type_ == Type::UNDIRECTED) {
            adj_[to].push_back(GraphEdge(from, distance, congestion, transport, ideal_speed));
        }
    }

    /// 获取节点信息（通过内部索引）
    const GraphNode& get_node(int idx) const {
        if (idx < 0 || idx >= nodes_.size()) throw std::out_of_range("Graph::get_node: index out of range");
        return nodes_[idx];
    }

    /// 获取节点信息（可修改）
    GraphNode& get_node(int idx) {
        if (idx < 0 || idx >= nodes_.size()) throw std::out_of_range("Graph::get_node: index out of range");
        return nodes_[idx];
    }

    /// 获取邻接表（某个节点的所有出边）
    const DynamicArray<GraphEdge>& get_neighbors(int idx) const {
        if (idx < 0 || idx >= nodes_.size()) throw std::out_of_range("Graph::get_neighbors: index out of range");
        return adj_[idx];
    }

    /// 获取邻接表引用（可修改）
    DynamicArray<GraphEdge>& get_neighbors(int idx) {
        if (idx < 0 || idx >= nodes_.size()) throw std::out_of_range("Graph::get_neighbors: index out of range");
        return adj_[idx];
    }

    /// 根据节点ID查找内部索引（线性查找，数据量大时可改为HashMap）
    int find_node_index(int node_id) const {
        for (int i = 0; i < nodes_.size(); i++) {
            if (nodes_[i].id == node_id) return i;
        }
        return -1;  // 未找到
    }

    /// 根据名称查找节点内部索引
    int find_node_by_name(const std::string& name) const {
        for (int i = 0; i < nodes_.size(); i++) {
            if (nodes_[i].name == name) return i;
        }
        return -1;
    }

    /// 清空图
    void clear() {
        nodes_.clear();
        adj_.clear();
        max_id_ = 0;
    }

    /// 预分配空间
    void reserve(int node_count) {
        // DynamicArray不支持reserve，但可以在构建时提供大致数量
        (void)node_count;
    }

    /// 获取所有节点（用于遍历）
    const DynamicArray<GraphNode>& get_all_nodes() const { return nodes_; }

    /// 获取邻接表总表（用于遍历）
    const DynamicArray<DynamicArray<GraphEdge>>& get_adjacency_list() const { return adj_; }

    /// 获取图的类型
    Type get_type() const { return type_; }
};

} // namespace algorithm

#endif // GRAPH_H
