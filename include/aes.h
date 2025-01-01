#ifndef AES_MODULE_H
#define AES_MODULE_H

#include <string>

// AESModule类提供文件或目录的加密和解密功能
class AESModule {
public:
    // 构造函数，接受加密密钥和初始化向量（IV）
    AESModule(const std::string& key, const std::string& iv);

    // 加密指定路径的文件或目录
    // inputPath: 输入文件或目录路径
    // outputPath: 输出加密文件路径
    bool encrypt(const std::string& inputPath, const std::string& outputPath);

    // 解密指定路径的加密文件
    // inputPath: 输入加密文件路径
    // outputPath: 输出解密文件或目录路径
    bool deencrypt(const std::string& inputPath, const std::string& outputPath);

private:
    std::string key_;
    std::string iv_;

    // 辅助函数：加密数据
    bool encryptData(const unsigned char* plaintext, int plaintext_len,
                    unsigned char* ciphertext, int& ciphertext_len);

    // 辅助函数：解密数据
    bool decryptData(const unsigned char* ciphertext, int ciphertext_len,
                    unsigned char* plaintext, int& plaintext_len);
};

#endif // AES_MODULE_H
