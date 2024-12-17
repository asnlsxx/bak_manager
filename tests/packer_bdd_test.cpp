#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include "Packer.h"
#include "cmdline.h"

namespace fs = std::filesystem;

class TestFixture {
protected:
    fs::path test_dir{"test_data"};
    fs::path backup_dir{"backup"};
    
    TestFixture() {
        // 创建测试目录结构
        fs::create_directories(test_dir);
        fs::create_directories(backup_dir);
        
        // 创建测试文件
        std::ofstream{test_dir / "file1.txt"} << "测试文件1";
        std::ofstream{test_dir / "file2.txt"} << "测试文件2";
        
        // 创建子目录和文件
        fs::create_directories(test_dir / "subdir");
        std::ofstream{test_dir / "subdir/file3.txt"} << "测试文件3";
        
        // 创建符号链接
        fs::create_symlink("file1.txt", test_dir / "link1");
    }
    
    ~TestFixture() {
        fs::remove_all(test_dir);
        fs::remove_all(backup_dir);
    }
    
    cmdline::parser create_backup_parser() {
        cmdline::parser parser;
        parser.add<std::string>("input", 'i', "输入路径", true);
        parser.add<std::string>("output", 'o', "输出路径", true);
        parser.add("backup", 'b', "备份");
        parser.add("restore", 'r', "恢复");
        return parser;
    }
};

SCENARIO_METHOD(TestFixture, "备份基本文件", "[backup][basic]") {
    GIVEN("一个包含普通文件的目录") {
        WHEN("执行基本备份") {
            auto parser = create_backup_parser();
            const char* argv[] = {
                "test",
                "-i", test_dir.string().c_str(),
                "-o", backup_dir.string().c_str(),
                "-b"
            };
            parser.parse_check(6, const_cast<char**>(argv));
            
            Packer packer(parser);
            
            THEN("备份应该成功") {
                REQUIRE(packer.Pack() == true);
                
                AND_THEN("备份文件应该存在") {
                    REQUIRE(fs::exists(backup_dir / (test_dir.filename().string() + ".backup")));
                }
            }
        }
    }
}

SCENARIO_METHOD(TestFixture, "过滤特定类型文件", "[backup][filter]") {
    GIVEN("一个包含多种类型文件的目录") {
        WHEN("设置只备份txt文件") {
            auto parser = create_backup_parser();
            parser.add<std::string>("type", '\0', "文件类��", false);
            
            const char* argv[] = {
                "test",
                "-i", test_dir.string().c_str(),
                "-o", backup_dir.string().c_str(),
                "-b",
                "--type", "n"
            };
            parser.parse_check(8, const_cast<char**>(argv));
            
            Packer packer(parser);
            
            THEN("备份应该成功且只包含普通文件") {
                REQUIRE(packer.Pack() == true);
            }
        }
    }
}

SCENARIO_METHOD(TestFixture, "恢复备份文件", "[restore]") {
    GIVEN("一个已经备份的目录") {
        // 先创建备份
        auto backup_parser = create_backup_parser();
        const char* backup_argv[] = {
            "test",
            "-i", test_dir.string().c_str(),
            "-o", backup_dir.string().c_str(),
            "-b"
        };
        backup_parser.parse_check(6, const_cast<char**>(backup_argv));
        Packer backup_packer(backup_parser);
        REQUIRE(backup_packer.Pack() == true);
        
        WHEN("恢复到新位置") {
            fs::path restore_dir = "restored_data";
            auto restore_parser = create_backup_parser();
            const char* restore_argv[] = {
                "test",
                "-i", restore_dir.string().c_str(),
                "-o", (backup_dir / (test_dir.filename().string() + ".backup")).string().c_str(),
                "-r"
            };
            restore_parser.parse_check(6, const_cast<char**>(restore_argv));
            
            Packer restore_packer(restore_parser);
            
            THEN("恢复应该成功") {
                REQUIRE(restore_packer.Unpack() == true);
                
                AND_THEN("恢复的文件应该与原始文件相同") {
                    REQUIRE(fs::exists(restore_dir / "file1.txt"));
                    REQUIRE(fs::exists(restore_dir / "file2.txt"));
                    REQUIRE(fs::exists(restore_dir / "subdir/file3.txt"));
                    REQUIRE(fs::exists(restore_dir / "link1"));
                }
            }
            
            fs::remove_all(restore_dir);
        }
    }
} 