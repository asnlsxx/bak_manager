#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <string>
#include <vector>
#include <string_view>

namespace LZWCompression {

/**
 * @brief 压缩数据。
 * 
 * @param data 要压缩的数据
 * @return std::vector<char> 压缩后的数据
 */
std::vector<char> compress(const std::string_view& data);

/**
 * @brief 解压缩数据。
 * 
 * @param data 压缩的数据
 * @param size 压缩数据的大小
 * @return std::vector<char> 解压后的数据
 */
std::vector<char> decompress(const char* data, size_t size);

} // namespace LZWCompression

#endif // COMPRESSION_H
