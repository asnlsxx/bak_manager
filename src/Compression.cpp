#include "compression.h"
#include <fstream>
#include <unordered_map>
#include <vector>
#include <bitset>
#include <iostream>
#include <filesystem>

namespace LZWCompression {

// 定义字典初始大小
const int DICT_INITIAL_SIZE = 256;

// 压缩文件函数实现
bool compressFile(const std::string& inputPath, const std::string& outputPath) {
    // 检查输入文件是否存在
    if (!std::filesystem::exists(inputPath)) {
        std::cerr << "输入文件不存在: " << inputPath << std::endl;
        return false;
    }

    // 打开输入文件
    std::ifstream inputFile(inputPath, std::ios::binary);
    if (!inputFile.is_open()) {
        std::cerr << "无法打开输入文件: " << inputPath << std::endl;
        return false;
    }

    // 读取文件内容
    std::string data((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();

    // 初始化字典
    std::unordered_map<std::string, int> dictionary;
    for (int i = 0; i < DICT_INITIAL_SIZE; ++i) {
        dictionary[std::string(1, static_cast<char>(i))] = i;
    }
    int dictSize = DICT_INITIAL_SIZE;

    std::string w;
    std::vector<int> compressedData;

    for (char c : data) {
        std::string wc = w + c;
        if (dictionary.find(wc) != dictionary.end()) {
            w = wc;
        } else {
            compressedData.push_back(dictionary[w]);
            // 添加wc到字典
            dictionary[wc] = dictSize++;
            w = std::string(1, c);
        }
    }

    if (!w.empty()) {
        compressedData.push_back(dictionary[w]);
    }

    // 将压缩数据写入输出文件
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile.is_open()) {
        std::cerr << "无法打开输出文件: " << outputPath << std::endl;
        return false;
    }

    // 写入字典大小
    outputFile.write(reinterpret_cast<const char*>(&dictSize), sizeof(dictSize));

    // 写入压缩数据
    for (int code : compressedData) {
        outputFile.write(reinterpret_cast<const char*>(&code), sizeof(code));
    }

    outputFile.close();
    return true;
}

// 解压缩文件函数实现
bool decompressFile(const std::string& inputPath, const std::string& outputPath) {
    // 检查输入文件是否存在
    if (!std::filesystem::exists(inputPath)) {
        std::cerr << "输入文件不存在: " << inputPath << std::endl;
        return false;
    }

    // 打开输入文件
    std::ifstream inputFile(inputPath, std::ios::binary);
    if (!inputFile.is_open()) {
        std::cerr << "无法打开输入文件: " << inputPath << std::endl;
        return false;
    }

    // 读取字典大小
    int dictSize;
    inputFile.read(reinterpret_cast<char*>(&dictSize), sizeof(dictSize));

    // 初始化字典
    std::vector<std::string> dictionary(dictSize);
    for (int i = 0; i < DICT_INITIAL_SIZE; ++i) {
        dictionary[i] = std::string(1, static_cast<char>(i));
    }

    std::vector<int> compressedData;
    int code;
    while (inputFile.read(reinterpret_cast<char*>(&code), sizeof(code))) {
        compressedData.push_back(code);
    }
    inputFile.close();

    if (compressedData.empty()) {
        std::cerr << "压缩文件为空或格式错误。" << std::endl;
        return false;
    }

    std::string w( dictionary[compressedData[0]] );
    std::string result = w;

    for (size_t i = 1; i < compressedData.size(); ++i) {
        int k = compressedData[i];
        std::string entry;
        if (k < dictSize) {
            entry = dictionary[k];
        } else if (k == dictSize) {
            entry = w + w[0];
        } else {
            std::cerr << "压缩文件包含无效的代码: " << k << std::endl;
            return false;
        }

        result += entry;

        // 添加w + entry[0]到字典
        dictionary.push_back(w + entry[0]);
        dictSize++;

        w = entry;
    }

    // 将解压缩数据写入输出文件
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile.is_open()) {
        std::cerr << "无法打开输出文件: " << outputPath << std::endl;
        return false;
    }

    outputFile.write(result.c_str(), result.size());
    outputFile.close();

    return true;
}

} // namespace LZWCompression
