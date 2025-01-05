#ifndef AES_MODULE_H
#define AES_MODULE_H

#include <string>
#include <vector>
#include <string_view>
#include <array>

/**
 * @brief AES加密模块类，提供256位AES-CBC加密功能
 */
class AESModule {
public:
    static constexpr size_t KEY_SIZE = 32;  // 256位密钥
    static constexpr size_t IV_SIZE = 16;   // 128位IV
    using KeyType = std::array<unsigned char, KEY_SIZE>;
    using IVType = std::array<unsigned char, IV_SIZE>;

    /**
     * @brief 构造函数
     * @param password 用于生成密钥和IV的密码
     */
    explicit AESModule(const std::string& password);

    /**
     * @brief 加密数据
     * @param data 要加密的数据
     * @return 加密后的数据
     */
    std::vector<char> encrypt(const std::string_view& data);

    /**
     * @brief 解密数据
     * @param data 要解密的数据
     * @param size 数据大小
     * @return 解密后的数据
     */
    std::vector<char> decrypt(const char* data, size_t size);

private:
    KeyType key_;
    IVType iv_;

    /**
     * @brief 从密码派生密钥和IV
     * @param password 用户密码
     * @return 密钥和IV的pair
     */
    static std::pair<KeyType, IVType> derive_key_iv(const std::string& password);

    /**
     * @brief 加密数据的辅助函数
     * @param plaintext 要加密的数据
     * @param plaintext_len 数据长度
     * @param ciphertext 加密后的数据
     * @param ciphertext_len 加密后的数据长度
     * @return 是否成功
     */
    bool encryptData(const unsigned char* plaintext, int plaintext_len,
                    unsigned char* ciphertext, int& ciphertext_len);

    /**
     * @brief 解密数据的辅助函数
     * @param ciphertext 要解密的数据
     * @param ciphertext_len 数据长度
     * @param plaintext 解密后的数据
     * @param plaintext_len 解密后的数据长度
     * @return 是否成功
     */
    bool decryptData(const unsigned char* ciphertext, int ciphertext_len,
                    unsigned char* plaintext, int& plaintext_len);
};

#endif // AES_MODULE_H
