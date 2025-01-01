#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <string>

// 命名空间以避免命名冲突
namespace LZWCompression {

/**
 * @brief 压缩指定路径的文件。
 * 
 * @param inputPath 输入文件的路径。
 * @param outputPath 压缩后文件的保存路径。
 * @return true 压缩成功。
 * @return false 压缩失败。
 */
bool compressFile(const std::string& inputPath, const std::string& outputPath);

/**
 * @brief 解压缩指定路径的文件。
 * 
 * @param inputPath 压缩文件的路径。
 * @param outputPath 解压缩后文件的保存路径。
 * @return true 解压缩成功。
 * @return false 解压缩失败。
 */
bool decompressFile(const std::string& inputPath, const std::string& outputPath);

} // namespace LZWCompression

#endif // COMPRESSION_H
