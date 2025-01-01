#include "aes.h"
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/kdf.h>
#include <stdexcept>

AESModule::AESModule(const std::string& password) {
    auto [derived_key, derived_iv] = derive_key_iv(password);
    key_ = derived_key;
    iv_ = derived_iv;
}

std::pair<AESModule::KeyType, AESModule::IVType> 
AESModule::derive_key_iv(const std::string& password) {
    KeyType key;
    IVType iv;
    
    // 使用 PBKDF2 从密码派生密钥和IV
    const unsigned char salt[] = "BackupManagerSalt";  // 固定盐值
    const int iterations = 10000;  // 迭代次数

    if (!PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                          salt, sizeof(salt) - 1,
                          iterations,
                          EVP_sha256(),
                          KEY_SIZE + IV_SIZE,
                          key.data())) {
        throw std::runtime_error("密钥派生失败");
    }

    // 复制IV（从密钥数据之后的部分）
    std::copy_n(key.data() + KEY_SIZE, IV_SIZE, iv.data());

    return {key, iv};
}

std::vector<char> AESModule::encrypt(const std::string_view& data) {
    // 分配加密后的缓冲区
    std::vector<char> ciphertext(data.size() + EVP_MAX_BLOCK_LENGTH);
    int ciphertext_len = 0;

    if (!encryptData(reinterpret_cast<const unsigned char*>(data.data()), 
                    data.size(), 
                    reinterpret_cast<unsigned char*>(ciphertext.data()), 
                    ciphertext_len)) {
        throw std::runtime_error("加密失败");
    }

    // 调整到实际大小
    ciphertext.resize(ciphertext_len);
    return ciphertext;
}

std::vector<char> AESModule::decrypt(const char* data, size_t size) {
    // 分配解密后的缓冲区
    std::vector<char> plaintext(size);
    int plaintext_len = 0;

    if (!decryptData(reinterpret_cast<const unsigned char*>(data), 
                    size, 
                    reinterpret_cast<unsigned char*>(plaintext.data()), 
                    plaintext_len)) {
        throw std::runtime_error("解密失败");
    }

    // 调整到实际大小
    plaintext.resize(plaintext_len);
    return plaintext;
}

// 加密数据
bool AESModule::encryptData(const unsigned char* plaintext, int plaintext_len,
                            unsigned char* ciphertext, int& ciphertext_len) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL,
                               reinterpret_cast<const unsigned char*>(key_.data()),
                               reinterpret_cast<const unsigned char*>(iv_.data()))) {
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
                               reinterpret_cast<const unsigned char*>(key_.data()),
                               reinterpret_cast<const unsigned char*>(iv_.data()))) {
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
