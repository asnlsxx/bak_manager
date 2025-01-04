#ifndef AES_MODULE_H
#define AES_MODULE_H

#include <string>
#include <vector>
#include <string_view>
#include <array>

class AESModule {
public:
    static constexpr size_t KEY_SIZE = 32;  // 256位密钥
    static constexpr size_t IV_SIZE = 16;   // 128位IV
    using KeyType = std::array<unsigned char, KEY_SIZE>;
    using IVType = std::array<unsigned char, IV_SIZE>;

    // 构造函数，只接受密码
    explicit AESModule(const std::string& password);

    // 加密数据
    std::vector<char> encrypt(const std::string_view& data);

    // 解密数据
    std::vector<char> decrypt(const char* data, size_t size);

private:
    KeyType key_;
    IVType iv_;

    // 从密码派生密钥和IV
    static std::pair<KeyType, IVType> derive_key_iv(const std::string& password);

    // 辅助函数：加密数据
    bool encryptData(const unsigned char* plaintext, int plaintext_len,
                    unsigned char* ciphertext, int& ciphertext_len);

    // 辅助函数：解密数据
    bool decryptData(const unsigned char* ciphertext, int ciphertext_len,
                    unsigned char* plaintext, int& plaintext_len);
};

#endif // AES_MODULE_H
