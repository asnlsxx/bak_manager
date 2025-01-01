#include "aes.h"
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <fstream>
#include <vector>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

// 构造函数，初始化密钥和IV
AESModule::AESModule(const std::string& key, const std::string& iv)
    : key_(key), iv_(iv) {}

// 加密数据
bool AESModule::encryptData(const unsigned char* plaintext, int plaintext_len,
                            unsigned char* ciphertext, int& ciphertext_len) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL,
                               reinterpret_cast<const unsigned char*>(key_.c_str()),
                               reinterpret_cast<const unsigned char*>(iv_.c_str()))) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int len;
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

// 解密数据
bool AESModule::decryptData(const unsigned char* ciphertext, int ciphertext_len,
                            unsigned char* plaintext, int& plaintext_len) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL,
                               reinterpret_cast<const unsigned char*>(key_.c_str()),
                               reinterpret_cast<const unsigned char*>(iv_.c_str()))) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int len;
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

// 加密指定路径
bool AESModule::encrypt(const std::string& inputPath, const std::string& outputPath) {
    // 读取输入文件
    std::ifstream ifs(inputPath, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "无法打开输入文件: " << inputPath << std::endl;
        return false;
    }

    std::vector<unsigned char> plaintext((std::istreambuf_iterator<char>(ifs)),
                                         std::istreambuf_iterator<char>());
    ifs.close();

    // 分配加密后的缓冲区
    std::vector<unsigned char> ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH);
    int ciphertext_len = 0;

    if (!encryptData(plaintext.data(), plaintext.size(), ciphertext.data(), ciphertext_len)) {
        std::cerr << "加密失败。" << std::endl;
        return false;
    }

    // 写入加密后的数据
    std::ofstream ofs(outputPath, std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "无法打开输出文件: " << outputPath << std::endl;
        return false;
    }
    ofs.write(reinterpret_cast<char*>(ciphertext.data()), ciphertext_len);
    ofs.close();

    return true;
}

// 解密指定路径
bool AESModule::deencrypt(const std::string& inputPath, const std::string& outputPath) {
    // 读取加密文件
    std::ifstream ifs(inputPath, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "无法打开输入文件: " << inputPath << std::endl;
        return false;
    }

    std::vector<unsigned char> ciphertext((std::istreambuf_iterator<char>(ifs)),
                                          std::istreambuf_iterator<char>());
    ifs.close();

    // 分配解密后的缓冲区
    std::vector<unsigned char> plaintext(ciphertext.size());
    int plaintext_len = 0;

    if (!decryptData(ciphertext.data(), ciphertext.size(), plaintext.data(), plaintext_len)) {
        std::cerr << "解密失败。" << std::endl;
        return false;
    }

    // 写入解密后的数据
    std::ofstream ofs(outputPath, std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "无法打开输出文件: " << outputPath << std::endl;
        return false;
    }
    ofs.write(reinterpret_cast<char*>(plaintext.data()), plaintext_len);
    ofs.close();

    return true;
}
