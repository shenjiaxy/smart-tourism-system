/**
 * @file huffman.h
 * @brief Huffman coding for lossless diary compression.
 */

#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "heap.h"

#include <cstring>
#include <string>

namespace algorithm {

struct HuffmanNode {
    char ch;
    int freq;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode() : ch('\0'), freq(0), left(nullptr), right(nullptr) {}
    HuffmanNode(char c, int f) : ch(c), freq(f), left(nullptr), right(nullptr) {}

    bool operator>(const HuffmanNode& other) const { return freq > other.freq; }
    bool operator<(const HuffmanNode& other) const { return freq < other.freq; }
};

struct HuffmanCode {
    char ch;
    std::string code;
};

class Huffman {
public:
    static void count_frequency(const unsigned char* data, int len, int* freq_table) {
        memset(freq_table, 0, sizeof(int) * 256);
        if (!data || len <= 0) return;
        for (int i = 0; i < len; i++) {
            freq_table[data[i]]++;
        }
    }

    static HuffmanNode* build_tree(const int* freq_table) {
        int valid_chars = 0;
        for (int i = 0; i < 256; i++) {
            if (freq_table[i] > 0) valid_chars++;
        }

        if (valid_chars == 0) return nullptr;
        if (valid_chars == 1) {
            HuffmanNode* root = new HuffmanNode('\0', 0);
            for (int i = 0; i < 256; i++) {
                if (freq_table[i] > 0) {
                    root->left = new HuffmanNode(static_cast<char>(i), freq_table[i]);
                    break;
                }
            }
            return root;
        }

        Heap<HuffmanNode*, FreqCompare> min_heap(valid_chars + 1);
        for (int i = 0; i < 256; i++) {
            if (freq_table[i] > 0) {
                min_heap.push(new HuffmanNode(static_cast<char>(i), freq_table[i]));
            }
        }

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

    static int build_code_table(HuffmanNode* root, HuffmanCode* codes) {
        if (!root || !codes) return 0;
        int count = 0;
        generate_codes(root, "", codes, count);
        return count;
    }

    static std::string encode(const unsigned char* data, int len,
                              const HuffmanCode* codes, int code_count) {
        if (!data || len <= 0 || !codes || code_count <= 0) return "";

        std::string lookup[256];
        for (int i = 0; i < code_count; i++) {
            lookup[static_cast<unsigned char>(codes[i].ch)] = codes[i].code;
        }

        std::string result;
        result.reserve(len * 8);
        for (int i = 0; i < len; i++) {
            result += lookup[data[i]];
        }
        return result;
    }

    static std::string decode(const std::string& bits, HuffmanNode* root, int expected_len) {
        if (!root || expected_len <= 0) return "";

        std::string result;
        result.reserve(expected_len);
        HuffmanNode* cur = root;

        for (size_t i = 0; i < bits.size() && static_cast<int>(result.size()) < expected_len; i++) {
            cur = bits[i] == '0' ? cur->left : cur->right;
            if (!cur) return "";
            if (!cur->left && !cur->right) {
                result += cur->ch;
                cur = root;
            }
        }

        return result;
    }

    static int compress(const unsigned char* data, int len, unsigned char** compressed) {
        if (compressed) *compressed = nullptr;
        if (!data || len <= 0 || !compressed) return 0;

        int freq[256];
        count_frequency(data, len, freq);

        HuffmanNode* root = build_tree(freq);
        if (!root) return 0;

        HuffmanCode codes[256];
        int code_count = build_code_table(root, codes);
        std::string bits = encode(data, len, codes, code_count);

        int unique_count = 0;
        for (int i = 0; i < 256; i++) {
            if (freq[i] > 0) unique_count++;
        }

        const int header_size = 2 + unique_count * 5;
        const int data_bits = static_cast<int>(bits.size());
        const int data_bytes = (data_bits + 7) / 8;
        const int total_size = 4 + header_size + data_bytes;

        *compressed = new unsigned char[total_size];
        (*compressed)[0] = static_cast<unsigned char>(len & 0xFF);
        (*compressed)[1] = static_cast<unsigned char>((len >> 8) & 0xFF);
        (*compressed)[2] = static_cast<unsigned char>((len >> 16) & 0xFF);
        (*compressed)[3] = static_cast<unsigned char>((len >> 24) & 0xFF);
        (*compressed)[4] = static_cast<unsigned char>('H');
        (*compressed)[5] = static_cast<unsigned char>(unique_count == 256 ? 0 : unique_count);

        int pos = 6;
        for (int i = 0; i < 256; i++) {
            if (freq[i] <= 0) continue;
            (*compressed)[pos++] = static_cast<unsigned char>(i);
            (*compressed)[pos++] = static_cast<unsigned char>(freq[i] & 0xFF);
            (*compressed)[pos++] = static_cast<unsigned char>((freq[i] >> 8) & 0xFF);
            (*compressed)[pos++] = static_cast<unsigned char>((freq[i] >> 16) & 0xFF);
            (*compressed)[pos++] = static_cast<unsigned char>((freq[i] >> 24) & 0xFF);
        }

        unsigned char* packed = *compressed + 4 + header_size;
        memset(packed, 0, data_bytes);
        for (int i = 0; i < data_bits; i++) {
            if (bits[i] == '1') {
                packed[i / 8] |= static_cast<unsigned char>(1 << (7 - (i % 8)));
            }
        }

        free_tree(root);
        return total_size;
    }

    static int decompress(const unsigned char* compressed, int compressed_len,
                          unsigned char** decompressed) {
        if (decompressed) *decompressed = nullptr;
        if (!compressed || compressed_len < 6 || !decompressed) return 0;

        int orig_len = static_cast<int>(compressed[0])
                     | (static_cast<int>(compressed[1]) << 8)
                     | (static_cast<int>(compressed[2]) << 16)
                     | (static_cast<int>(compressed[3]) << 24);
        if (orig_len <= 0 || orig_len > 256 * 1024 * 1024) return 0;

        int freq[256];
        memset(freq, 0, sizeof(freq));
        int data_start = 0;

        if (compressed[4] == static_cast<unsigned char>('H')) {
            const int unique_count = compressed[5] == 0 ? 256 : static_cast<int>(compressed[5]);
            const int header_size = 2 + unique_count * 5;
            if (unique_count <= 0 || compressed_len < 4 + header_size) return 0;

            int pos = 6;
            for (int i = 0; i < unique_count; i++) {
                const int ch = static_cast<int>(compressed[pos++]);
                freq[ch] = static_cast<int>(compressed[pos])
                         | (static_cast<int>(compressed[pos + 1]) << 8)
                         | (static_cast<int>(compressed[pos + 2]) << 16)
                         | (static_cast<int>(compressed[pos + 3]) << 24);
                pos += 4;
            }
            data_start = 4 + header_size;
        } else {
            if (compressed_len < 4 + 256 * 4 + 1) return 0;
            const unsigned char* header = compressed + 4;
            for (int i = 0; i < 256; i++) {
                freq[i] = static_cast<int>(header[i * 4])
                        | (static_cast<int>(header[i * 4 + 1]) << 8)
                        | (static_cast<int>(header[i * 4 + 2]) << 16)
                        | (static_cast<int>(header[i * 4 + 3]) << 24);
            }
            data_start = 4 + 256 * 4;
        }

        if (data_start >= compressed_len) return 0;

        HuffmanNode* root = build_tree(freq);
        if (!root) return 0;

        const int data_len = compressed_len - data_start;
        std::string bits;
        bits.reserve(data_len * 8);
        for (int i = 0; i < data_len; i++) {
            for (int b = 7; b >= 0; b--) {
                bits += ((compressed[data_start + i] >> b) & 1) ? '1' : '0';
            }
        }

        std::string result = decode(bits, root, orig_len);
        free_tree(root);
        if (static_cast<int>(result.size()) != orig_len) return 0;

        *decompressed = new unsigned char[result.size()];
        memcpy(*decompressed, result.data(), result.size());
        return static_cast<int>(result.size());
    }

    static double compression_ratio(int original_size, int compressed_size) {
        if (original_size == 0) return 1.0;
        return static_cast<double>(compressed_size) / original_size;
    }

    static void free_tree(HuffmanNode* root) {
        if (!root) return;
        free_tree(root->left);
        free_tree(root->right);
        delete root;
    }

private:
    struct FreqCompare {
        bool operator()(HuffmanNode* a, HuffmanNode* b) const {
            if (a->freq == b->freq) {
                return static_cast<unsigned char>(a->ch) > static_cast<unsigned char>(b->ch);
            }
            return a->freq > b->freq;
        }
    };

    static void generate_codes(HuffmanNode* node, const std::string& current,
                               HuffmanCode* codes, int& count) {
        if (!node) return;
        if (!node->left && !node->right) {
            codes[count].ch = node->ch;
            codes[count].code = current.empty() ? "0" : current;
            count++;
            return;
        }
        generate_codes(node->left, current + "0", codes, count);
        generate_codes(node->right, current + "1", codes, count);
    }
};

} // namespace algorithm

#endif // HUFFMAN_H
