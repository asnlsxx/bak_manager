#include <catch2/catch_test_macros.hpp>
#include "AES.h"
#include <string>
#include <random>
#include <vector>

TEST_CASE("AES加密模块基础功能测试", "[aes]") {
    const std::string password = "test_password";
    AESModule aes(password);

    SECTION("空字符串加密解密") {
        std::string empty_str = "";
        auto encrypted = aes.encrypt(empty_str);
        auto decrypted = aes.decrypt(encrypted.data(), encrypted.size());
        std::string decrypted_str(decrypted.begin(), decrypted.end());
        REQUIRE(decrypted_str == empty_str);
    }

    SECTION("普通文本加密解密") {
        std::string normal_text = "Hello, 这是一段普通文本！@#$%^&*()";
        auto encrypted = aes.encrypt(normal_text);
        auto decrypted = aes.decrypt(encrypted.data(), encrypted.size());
        std::string decrypted_str(decrypted.begin(), decrypted.end());
        REQUIRE(decrypted_str == normal_text);
    }

    SECTION("超大字符串加密解密") {
        std::string large_text(1024 * 1024, 'A'); // 1MB的数据
        auto encrypted = aes.encrypt(large_text);
        auto decrypted = aes.decrypt(encrypted.data(), encrypted.size());
        std::string decrypted_str(decrypted.begin(), decrypted.end());
        REQUIRE(decrypted_str == large_text);
    }

    SECTION("随机数据加密解密") {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        std::vector<char> random_data(10000);
        for(auto& byte : random_data) {
            byte = static_cast<char>(dis(gen));
        }
        
        std::string random_str(random_data.begin(), random_data.end());
        auto encrypted = aes.encrypt(random_str);
        auto decrypted = aes.decrypt(encrypted.data(), encrypted.size());
        std::string decrypted_str(decrypted.begin(), decrypted.end());
        REQUIRE(decrypted_str == random_str);
    }

    SECTION("特殊字符加密解密") {
        std::string special_chars;
        for(int i = 1; i < 128; i++) {
            special_chars += static_cast<char>(i);
        }
        auto encrypted = aes.encrypt(special_chars);
        auto decrypted = aes.decrypt(encrypted.data(), encrypted.size());
        std::string decrypted_str(decrypted.begin(), decrypted.end());
        REQUIRE(decrypted_str == special_chars);
    }
}

TEST_CASE("AES加密模块错误处理", "[aes][error]") {
    const std::string correct_password = "correct_password";
    const std::string wrong_password = "wrong_password";
    AESModule aes(correct_password);
    AESModule wrong_aes(wrong_password);

    SECTION("错误密码解密") {
        std::string original = "sensitive data";
        auto encrypted = aes.encrypt(original);
        
        // 使用错误密码解密应该抛出异常
        REQUIRE_THROWS(wrong_aes.decrypt(encrypted.data(), encrypted.size()));
        
        // 正确密码应该能解密
        auto decrypted = aes.decrypt(encrypted.data(), encrypted.size());
        std::string decrypted_str(decrypted.begin(), decrypted.end());
        REQUIRE(decrypted_str == original);
    }

    SECTION("损坏的密文") {
        std::string original = "test data";
        auto encrypted = aes.encrypt(original);
        encrypted[encrypted.size()/2] ^= 0xFF; // 修改中间的一个字节
        
        REQUIRE_THROWS(aes.decrypt(encrypted.data(), encrypted.size()));
    }
} 