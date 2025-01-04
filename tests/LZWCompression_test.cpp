#include <catch2/catch_test_macros.hpp>
#include "Compression.h"
#include <string>
#include <random>
TEST_CASE("LZW压缩基础功能测试", "[lzw]") {
    SECTION("高度可压缩数据") {
        // 重复文本
        std::string repeated = std::string(1000, 'A');
        auto compressed = LZWCompression::compress(repeated);
        auto decompressed = LZWCompression::decompress(compressed.data());
        std::string decompressed_str(decompressed.begin(), decompressed.end());
        REQUIRE(decompressed_str == repeated);
        REQUIRE(compressed.size() < repeated.length());
    }

    SECTION("随机数据") {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        std::string random_data;
        random_data.reserve(10000);
        for(int i = 0; i < 10000; i++) {
            random_data += static_cast<char>(dis(gen));
        }
        
        auto compressed = LZWCompression::compress(random_data);
        auto decompressed = LZWCompression::decompress(compressed.data());
        std::string decompressed_str(decompressed.begin(), decompressed.end());
        REQUIRE(decompressed_str == random_data);
    }

    SECTION("多行文本") {
        std::string multiline = "Line 1\nLine 2\nLine 3\n"
                               "这是中文行\n"
                               "Special chars: !@#$%^&*()\n";
        auto compressed = LZWCompression::compress(multiline);
        auto decompressed = LZWCompression::decompress(compressed.data());
        std::string decompressed_str(decompressed.begin(), decompressed.end());
        REQUIRE(decompressed_str == multiline);
    }

    SECTION("不同编码文本") {
        // UTF-8编码的中文文本
        std::string chinese = "测试中文压缩效果";
        auto compressed = LZWCompression::compress(chinese);
        auto decompressed = LZWCompression::decompress(compressed.data());
        std::string decompressed_str(decompressed.begin(), decompressed.end());
        REQUIRE(decompressed_str == chinese);

        // 包含emoji的文本
        std::string emoji = "Hello 👋 World 🌍";
        compressed = LZWCompression::compress(emoji);
        decompressed = LZWCompression::decompress(compressed.data());
        decompressed_str = std::string(decompressed.begin(), decompressed.end());
        REQUIRE(decompressed_str == emoji);
    }
}

TEST_CASE("LZW压缩边界情况测试", "[lzw]") {
    SECTION("空字符串") {
        std::string empty = "";
        auto compressed = LZWCompression::compress(empty);

        auto decompressed = LZWCompression::decompress(compressed.data());
        // std::string decompressed_str(decompressed.begin(), decompressed.end());
        // REQUIRE(decompressed_str == empty);
    }
    SECTION("只有一个字符") {
        std::string single_char = "X";
        auto compressed = LZWCompression::compress(single_char);
        auto decompressed = LZWCompression::decompress(compressed.data());
        std::string decompressed_str(decompressed.begin(), decompressed.end());
        REQUIRE(decompressed_str == single_char);
    }
    SECTION("单字符重复") {
        std::string single_char(10000, 'X');
        auto compressed = LZWCompression::compress(single_char);
        auto decompressed = LZWCompression::decompress(compressed.data());
        std::string decompressed_str(decompressed.begin(), decompressed.end());
        REQUIRE(decompressed_str == single_char);
    }

    SECTION("二进制数据") {
        std::string binary_data;
        binary_data.reserve(1000);
        for(int i = 0; i < 256; i++) {
            binary_data += static_cast<char>(i);
        }
        auto compressed = LZWCompression::compress(binary_data);
        auto decompressed = LZWCompression::decompress(compressed.data());
        std::string decompressed_str(decompressed.begin(), decompressed.end());
        REQUIRE(decompressed_str == binary_data);
    }
} 