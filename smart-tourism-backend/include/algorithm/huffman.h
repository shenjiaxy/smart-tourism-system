/**
 * @file huffman.h
 * @brief Huffman 编码 —— 用于日记文本的无损压缩存储
 *
 * 核心算法：贪心构造最优前缀码
 *
 * 算法步骤：
 *   1. 统计字符频率
 *   2. 将每个字符作为一个叶子节点，放入最小堆
 *   3. 反复取出堆顶两个最小频率节点，合并为新节点放回堆
 *   4. 直到堆中只剩一个节点（根节点）
 *   5. 从根到叶子的路径即为 Huffman 编码
 *
 * 时间复杂度：
 *   - 频率统计: O(n)
 *   - 建堆: O(k)，k为不同字符数
 *   - 构建Huffman树: O(k * log k)
 *   - 编码: O(n * L)，L为平均编码长度
 *   - 解码: O(n * L)
 *
 * 压缩率：接近信源熵，平均节省 20-50% 空间
 */

#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "heap.h"
#include <cstring>
#include <string>
#include <cmath>

namespace algorithm {

// ============================================================
// Huffman 树节点
// ============================================================
struct HuffmanNode {
    char ch;            // 字符（叶子节点有效，内部节点为 '\0'）
    int freq;           // 频率
    HuffmanNode* left;  // 左子树（编码0）
    HuffmanNode* right; // 右子树（编码1）

    HuffmanNode() : ch('\0'), freq(0), left(nullptr), right(nullptr) {}
    HuffmanNode(char c, int f) : ch(c), freq(f), left(nullptr), right(nullptr) {}

    bool operator>(const HuffmanNode& other) const { return freq > other.freq; }
    bool operator<(const HuffmanNode& other) const { return freq < other.freq; }
};

// ============================================================
// Huffman 编码表项
// ============================================================
struct HuffmanCode {
    char ch;            // 字符
    std::string code;   // 编码串（如 "0101"）
};

// ============================================================
// Huffman 编码器/解码器
// ============================================================
class Huffman {
public:
    /**
     * @brief 统计字符频率
     * @param data 输入数据
     * @param len 数据长度
     * @param freq_table 输出频率表（ASCII 256个字符）
     */
    static void count_frequency(const unsigned char* data, int len, int* freq_table) {
        memset(freq_table, 0, sizeof(int) * 256);
        for (int i = 0; i < len; i++) {
            freq_table[data[i]]++;
        }
    }

    /**
     * @brief 构建 Huffman 树
     * @param freq_table 频率表（256项）
     * @return Huffman 树根节点
     */
    static HuffmanNode* build_tree(const int* freq_table) {
        // 统计有效字符数
        int valid_chars = 0;
        for (int i = 0; i < 256; i++) {
            if (freq_table[i] > 0) valid_chars++;
        }

        if (valid_chars == 0) return nullptr;
        if (valid_chars == 1) {
            // 只有一种字符
            HuffmanNode* root = new HuffmanNode('\0', 0);
            for (int i = 0; i < 256; i++) {
                if (freq_table[i] > 0) {
                    root->left = new HuffmanNode(static_cast<char>(i), freq_table[i]);
                    break;
                }
            }
            return root;
        }

        // 使用最小堆
        Heap<HuffmanNode*, FreqCompare> min_heap(valid_chars + 1);

        // 每个字符创建叶子节点
        for (int i = 0; i < 256; i++) {
            if (freq_table[i] > 0) {
                min_heap.push(new HuffmanNode(static_cast<char>(i), freq_table[i]));
            }
        }

        // 反复合并最小两个节点
        while (min_heap.size() > 1) {
            HuffmanNode* left = min_heap.pop();
            HuffmanNode* right = min_heap.pop();
            HuffmanNode* parent = new HuffmanNode('\0', left->freq + right->freq);
            parent->left = left;
            parent->right = right;
            min_heap.push(parent);
        }

        return min_heap.pop();
    }

    /**
     * @brief 生成编码表（从 Huffman 树遍历得到）
     * @param root Huffman 树根
     * @param codes 输出编码表（最多256项）
     * @return 有效编码数
     */
    static int build_code_table(HuffmanNode* root, HuffmanCode* codes) {
        if (!root) return 0;
        std::string current;
        int count = 0;
        generate_codes(root, current, codes, count);
        return count;
    }

    /**
     * @brief 编码：将原始数据编码为二进制串
     * @param data 原始数据
     * @param len 数据长度
     * @param codes 编码表
     * @param code_count 编码表项数
     * @return 编码后的二进制串（'0'和'1'字符）
     */
    static std::string encode(const unsigned char* data, int len,
                              const HuffmanCode* codes, int code_count) {
        // 先构建快速查找表
        std::string lookup[256];
        for (int i = 0; i < code_count; i++) {
            lookup[static_cast<unsigned char>(codes[i].ch)] = codes[i].code;
        }

        std::string result;
        result.reserve(len * 8);  // 预分配

        for (int i = 0; i < len; i++) {
            result += lookup[data[i]];
        }

        return result;
    }

    /**
     * @brief 解码：将二进制串解码为原始数据
     * @param bits 二进制编码串
     * @param root Huffman 树根
     * @param expected_len 预期的原始数据长度（用于停止解码）
     * @return 解码后的数据
     */
    static std::string decode(const std::string& bits, HuffmanNode* root, int expected_len) {
        std::string result;
        result.reserve(expected_len);

        HuffmanNode* cur = root;
        for (size_t i = 0; i < bits.size() && static_cast<int>(result.size()) < expected_len; i++) {
            if (bits[i] == '0') {
                cur = cur->left;
            } else {
                cur = cur->right;
            }

            if (!cur) break;  // 错误的编码

            if (!cur->left && !cur->right) {
                // 叶子节点
                result += cur->ch;
                cur = root;
            }
        }

        return result;
    }

    /**
     * @brief 压缩数据（一步完成：频率统计 -> 建树 -> 编码）
     * @param data 原始数据
     * @param len 数据长度
     * @param compressed 输出压缩后的字节
     * @return 压缩后字节数
     */
    static int compress(const unsigned char* data, int len, unsigned char** compressed) {
        // 1. 统计频率
        int freq[256];
        count_frequency(data, len, freq);

        // 2. 建树
        HuffmanNode* root = build_tree(freq);

        // 3. 生成编码表
        HuffmanCode codes[256];
        int code_count = build_code_table(root, codes);

        // 4. 编码
        std::string bits = encode(data, len, codes, code_count);

        // 5. 打包：头部(频率表256*4字节) + 编码数据(每8位打包为1字节)
        int header_size = 256 * 4;  // 频率表
        int data_bits = static_cast<int>(bits.size());
        int data_bytes = (data_bits + 7) / 8;
        int total_size = header_size + data_bytes + 4;  // +4 for original length

        *compressed = new unsigned char[total_size];

        // 写入原始长度
        (*compressed)[0] = static_cast<unsigned char>(len & 0xFF);
        (*compressed)[1] = static_cast<unsigned char>((len >> 8) & 0xFF);
        (*compressed)[2] = static_cast<unsigned char>((len >> 16) & 0xFF);
        (*compressed)[3] = static_cast<unsigned char>((len >> 24) & 0xFF);

        // 写入频率表
        unsigned char* header = *compressed + 4;
        for (int i = 0; i < 256; i++) {
            header[i * 4 + 0] = static_cast<unsigned char>(freq[i] & 0xFF);
            header[i * 4 + 1] = static_cast<unsigned char>((freq[i] >> 8) & 0xFF);
            header[i * 4 + 2] = static_cast<unsigned char>((freq[i] >> 16) & 0xFF);
            header[i * 4 + 3] = static_cast<unsigned char>((freq[i] >> 24) & 0xFF);
        }

        // 写入编码数据
        unsigned char* packed = *compressed + 4 + header_size;
        memset(packed, 0, data_bytes);
        for (int i = 0; i < data_bits; i++) {
            if (bits[i] == '1') {
                packed[i / 8] |= static_cast<unsigned char>(1 << (7 - (i % 8)));
            }
        }

        // 释放 Huffman 树
        free_tree(root);

        return total_size;
    }

    /**
     * @brief 解压数据
     * @param compressed 压缩数据
     * @param compressed_len 压缩数据长度
     * @param decompressed 输出解压数据
     * @return 解压后字节数
     */
    static int decompress(const unsigned char* compressed, int compressed_len,
                          unsigned char** decompressed) {
        if (compressed_len < 4 + 256 * 4 + 1) return 0;

        // 读取原始长度
        int orig_len = static_cast<int>(compressed[0])
                     | (static_cast<int>(compressed[1]) << 8)
                     | (static_cast<int>(compressed[2]) << 16)
                     | (static_cast<int>(compressed[3]) << 24);

        // 校验原始长度合理性，防止畸形数据导致分配超大内存
        if (orig_len <= 0 || orig_len > 256 * 1024 * 1024) return 0;

        // 读取频率表
        int freq[256];
        const unsigned char* header = compressed + 4;
        for (int i = 0; i < 256; i++) {
            freq[i] = static_cast<int>(header[i * 4])
                    | (static_cast<int>(header[i * 4 + 1]) << 8)
                    | (static_cast<int>(header[i * 4 + 2]) << 16)
                    | (static_cast<int>(header[i * 4 + 3]) << 24);
        }

        // 重建 Huffman 树
        HuffmanNode* root = build_tree(freq);

        // 解码
        int data_start = 4 + 256 * 4;
        int data_len = compressed_len - data_start;

        std::string bits;
        bits.reserve(data_len * 8);
        for (int i = 0; i < data_len; i++) {
            for (int b = 7; b >= 0; b--) {
                bits += ((compressed[data_start + i] >> b) & 1) ? '1' : '0';
            }
        }

        std::string result = decode(bits, root, orig_len);

        *decompressed = new unsigned char[result.size()];
        memcpy(*decompressed, result.c_str(), result.size());

        free_tree(root);

        return static_cast<int>(result.size());
    }

    /// 计算压缩率
    static double compression_ratio(int original_size, int compressed_size) {
        if (original_size == 0) return 1.0;
        return static_cast<double>(compressed_size) / original_size;
    }

    /// 释放 Huffman 树
    static void free_tree(HuffmanNode* root) {
        if (!root) return;
        free_tree(root->left);
        free_tree(root->right);
        delete root;
    }

private:
    // 自定义比较器（用于堆中比较 HuffmanNode*）
    struct FreqCompare {
        bool operator()(HuffmanNode* a, HuffmanNode* b) const { return a->freq < b->freq; }
    };

    // 递归生成编码表
    static void generate_codes(HuffmanNode* node, std::string current,
                               HuffmanCode* codes, int& count) {
        if (!node) return;
        if (!node->left && !node->right) {
            // 叶子节点
            if (current.empty()) current = "0";  // 单字符特殊处理
            codes[count].ch = node->ch;
            codes[count].code = current;
            count++;
            return;
        }
        if (node->left)  generate_codes(node->left, current + "0", codes, count);
        if (node->right) generate_codes(node->right, current + "1", codes, count);
    }
};

} // namespace algorithm

#endif // HUFFMAN_H
