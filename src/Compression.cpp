#include "Compression.h"
#include <unordered_map>
#include <cstring>

namespace LZWCompression {

std::vector<char> compress(const std::string_view& data) {
    std::unordered_map<std::string, int> dictionary;
    std::vector<int> compressed;
    
    // 初始化字典（ASCII字符）
    for (int i = 0; i < 256; i++) {
        dictionary[std::string(1, static_cast<char>(i))] = i;
    }
    
    std::string current;
    int next_code = 256;
    
    // 压缩数据
    for (char c : data) {
        std::string next = current + c;
        if (dictionary.find(next) != dictionary.end()) {
            current = next;
        } else {
            compressed.push_back(dictionary[current]);
            dictionary[next] = next_code++;
            current = std::string(1, c);
        }
    }
    
    if (!current.empty()) {
        compressed.push_back(dictionary[current]);
    }
    
    // 将压缩后的整数序列转换为字节序列
    std::vector<char> result;
    result.reserve(compressed.size() * sizeof(int));
    
    // 写入压缩后的大小
    size_t comp_size = compressed.size();
    result.insert(result.end(), 
                 reinterpret_cast<const char*>(&comp_size),
                 reinterpret_cast<const char*>(&comp_size) + sizeof(size_t));
    
    // 写入压缩数据
    for (int code : compressed) {
        result.insert(result.end(),
                     reinterpret_cast<const char*>(&code),
                     reinterpret_cast<const char*>(&code) + sizeof(int));
    }
    
    return result;
}

std::vector<char> decompress(const char* data, size_t size) {
    if (size < sizeof(size_t)) {
        return {};
    }
    
    // 读取压缩数据大小
    size_t comp_size;
    std::memcpy(&comp_size, data, sizeof(size_t));
    data += sizeof(size_t);
    
    // 读取压缩数据
    std::vector<int> compressed;
    compressed.reserve(comp_size);
    for (size_t i = 0; i < comp_size; ++i) {
        int code;
        std::memcpy(&code, data + i * sizeof(int), sizeof(int));
        compressed.push_back(code);
    }
    
    // 初始化字典
    std::vector<std::string> dictionary(256);
    for (int i = 0; i < 256; i++) {
        dictionary[i] = std::string(1, static_cast<char>(i));
    }
    
    std::vector<char> result;
    std::string w(dictionary[compressed[0]]);
    result.insert(result.end(), w.begin(), w.end());
    
    // 解压数据
    for (size_t i = 1; i < compressed.size(); ++i) {
        int k = compressed[i];
        std::string entry;
        
        if (k < dictionary.size()) {
            entry = dictionary[k];
        } else {
            entry = w + w[0];
        }
        
        result.insert(result.end(), entry.begin(), entry.end());
        dictionary.push_back(w + entry[0]);
        w = entry;
    }
    
    return result;
}

} // namespace LZWCompression
