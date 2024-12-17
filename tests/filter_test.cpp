#include <catch2/catch_test_macros.hpp>
#include "Packer.h"
#include "cmdline.h"
#include <filesystem>

namespace fs = std::filesystem;

// 辅助函数：创建测试文件结构
void create_test_files() {
    fs::create_directories("test_files");
    std::ofstream{"test_files/test1.txt"} << "test1";
    std::ofstream{"test_files/test2.cpp"} << "test2";
    fs::create_directories("test_files/subdir");
    std::ofstream{"test_files/subdir/test3.txt"} << "test3";
    fs::create_symlink("test1.txt", "test_files/link1");
}

// 辅助函数：清理测试文件
void cleanup_test_files() {
    fs::remove_all("test_files");
}

// 辅助函数：创建命令行解析器
cmdline::parser create_parser() {
    cmdline::parser parser;
    parser.add<std::string>("input", 'i', "input path", true);
    parser.add<std::string>("output", 'o', "output path", true);
    parser.add("backup", 'b', "backup mode");
    parser.add<std::string>("type", '\0', "file type filter", false);
    parser.add<std::string>("path", '\0', "path filter", false);
    parser.add<std::string>("name", '\0', "name filter", false);
    return parser;
}

TEST_CASE("文件类型过滤器测试", "[filter][type]") {
    create_test_files();
    
    SECTION("只过滤普通文件") {
        auto parser = create_parser();
        const char* argv[] = {
            "test",
            "-i", "test_files",
            "-o", "backup",
            "-b",
            "--type", "n"
        };
        parser.parse_check(8, const_cast<char**>(argv));
        
        Packer packer(parser);
        REQUIRE(packer.Pack() == true);
        
        // 验证只有普通文件被备份
        REQUIRE(fs::exists("backup/test_files.backup"));
        // TODO: 验证备份文件内容
    }
    
    cleanup_test_files();
}

TEST_CASE("文件名过滤器测试", "[filter][name]") {
    create_test_files();
    
    SECTION("只备份.txt文件") {
        auto parser = create_parser();
        const char* argv[] = {
            "test",
            "-i", "test_files",
            "-o", "backup",
            "-b",
            "--name", ".*\\.txt$"
        };
        parser.parse_check(8, const_cast<char**>(argv));
        
        Packer packer(parser);
        REQUIRE(packer.Pack() == true);
        
        // 验证只有.txt文件被备份
        REQUIRE(fs::exists("backup/test_files.backup"));
        // TODO: 验证备份文件内容
    }
    
    cleanup_test_files();
}

TEST_CASE("路径过滤器测试", "[filter][path]") {
    create_test_files();
    
    SECTION("只备份子目录") {
        auto parser = create_parser();
        const char* argv[] = {
            "test",
            "-i", "test_files",
            "-o", "backup",
            "-b",
            "--path", ".*/subdir/.*"
        };
        parser.parse_check(8, const_cast<char**>(argv));
        
        Packer packer(parser);
        REQUIRE(packer.Pack() == true);
        
        // 验证只有子目录被备份
        REQUIRE(fs::exists("backup/test_files.backup"));
        // TODO: 验证备份文件内容
    }
    
    cleanup_test_files();
}

TEST_CASE("冲突选项检查", "[filter][conflict]") {
    create_test_files();
    
    SECTION("备份和恢复不能同时指定") {
        auto parser = create_parser();
        const char* argv[] = {
            "test",
            "-i", "test_files",
            "-o", "backup",
            "-b",  // 同时指定备份
            "-r"   // 和恢复
        };
        
        REQUIRE_THROWS_WITH(
            parser.parse_check(7, const_cast<char**>(argv)),
            "不能同时指定备份(-b)和恢复(-r)选项"
        );
    }
    
    SECTION("过滤选项只能在备份时使用") {
        auto parser = create_parser();
        const char* argv[] = {
            "test",
            "-i", "test_files",
            "-o", "backup",
            "-r",  // 恢复模式
            "--type", "n"  // 但指定了过滤选项
        };
        
        REQUIRE_THROWS_WITH(
            parser.parse_check(8, const_cast<char**>(argv)),
            "过滤选项只能在备份模式下使用"
        );
    }
    
    cleanup_test_files();
} 