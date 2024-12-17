#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include "Packer.h"
#include "cmdline.h"

namespace fs = std::filesystem;

// 定义文件类型
enum class TestFileType {
    Regular,    // 普通文件(包括硬链接)
    Directory,  // 目录
    Symlink,    // 符号链接
};

// 定义测试文件结构
struct TestFile {
    std::string path;           // 相对路径
    TestFileType type;          // 文件类型
    std::string content;        // 文件内容(仅普通文件)
    std::string target;         // 链接目标(符号链接)或硬链接源(硬链接)
    bool is_hardlink{false};    // 是否是硬链接
};

class TestFixture {
protected:
    fs::path test_dir;
    fs::path backup_dir;
    std::vector<TestFile> test_files;
    
    TestFixture(const std::string& test_dir_name = "test_data",
                const std::string& backup_dir_name = "backup") 
        : test_dir(test_dir_name), backup_dir(backup_dir_name) {}
    
    // 创建测试目录结构
    void create_test_structure(const std::vector<TestFile>& files) {
        test_files = files;
        
        // 创建基本目录
        fs::create_directories(test_dir);
        fs::create_directories(backup_dir);
        
        // 创建测试文件
        for (const auto& file : files) {
            fs::path file_path = test_dir / file.path;
            
            // 确保父目录存在
            if (file.type != TestFileType::Directory) {
                fs::create_directories(file_path.parent_path());
            }
            
            switch (file.type) {
                case TestFileType::Regular:
                    if (file.is_hardlink) {
                        fs::create_hard_link(test_dir / file.target, file_path);
                    } else {
                        std::ofstream{file_path} << file.content;
                    }
                    break;
                    
                case TestFileType::Directory:
                    fs::create_directories(file_path);
                    break;
                    
                case TestFileType::Symlink:
                    fs::create_symlink(file.target, file_path);
                    break;
            }
        }
    }
    
    ~TestFixture() {
        fs::remove_all(test_dir);
        fs::remove_all(backup_dir);
    }
    
    // 创建命令行解析器
    cmdline::parser create_parser(const std::vector<std::string>& args) {
        cmdline::parser parser;
        // 基本选项
        parser.add<std::string>("input", 'i', "输入路径", true);
        parser.add<std::string>("output", 'o', "输出路径", true);
        parser.add("backup", 'b', "备份");
        parser.add("restore", 'r', "恢复");
        parser.add<std::string>("type", '\0', "文件类型: n普通文件,d目录文件,l符号链接", false);
        parser.add<std::string>("name", '\0', "文件名", false);
        parser.add<std::string>("path", '\0', "路径", false);
        
        // 转换参数为char**
        std::vector<const char*> argv;
        argv.push_back("test");  // 程序名
        for (const auto& arg : args) {
            argv.push_back(arg.c_str());
        }
        
        parser.parse_check(argv.size(), const_cast<char**>(argv.data()));
        return parser;
    }
};

SCENARIO_METHOD(TestFixture, "备份基本文件", "[backup][basic]") {
    GIVEN("一个包含多种类型文件的目录") {
        std::vector<TestFile> files = {
            {"file1.txt", TestFileType::Regular, "测试文件1"},
            {"subdir", TestFileType::Directory},
            {"subdir/file2.txt", TestFileType::Regular, "测试文件2"},
            {"link1", TestFileType::Symlink, "", "file1.txt"},
            {"hardlink1", TestFileType::Regular, "", "file1.txt", true}
        };
        create_test_structure(files);
        
        WHEN("执行基本备份") {
            std::vector<std::string> args = {
                "-i", test_dir.string(),
                "-o", backup_dir.string(),
                "-b"
            };
            auto parser = create_parser(args);
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
        std::vector<TestFile> files = {
            {"file1.txt", TestFileType::Regular, "普通文件1"},
            {"subdir", TestFileType::Directory},
            {"link1", TestFileType::Symlink, "", "file1.txt"},
            {"hardlink1", TestFileType::Regular, "", "file1.txt", true}
        };
        create_test_structure(files);
        
        WHEN("设置只备份普通文件") {
            std::vector<std::string> args = {
                "-i", test_dir.string(),
                "-o", backup_dir.string(),
                "-b",
                "--type", "n"
            };
            auto parser = create_parser(args);
            Packer packer(parser);
            
            THEN("备份应该成功且只包含普通文件") {
                REQUIRE(packer.Pack() == true);
                // TODO: 验证备份文件只包含普通文件
            }
        }
    }
}

SCENARIO_METHOD(TestFixture, "恢复备份文件", "[restore]") {
    GIVEN("一个已经备份的目录") {
        std::vector<TestFile> files = {
            {"file1.txt", TestFileType::Regular, "测试文件1"},
            {"subdir", TestFileType::Directory},
            {"subdir/file2.txt", TestFileType::Regular, "测试文件2"},
            {"link1", TestFileType::Symlink, "", "file1.txt"}
        };
        create_test_structure(files);
        
        // 先创建备份
        std::vector<std::string> backup_args = {
            "-i", test_dir.string(),
            "-o", backup_dir.string(),
            "-b"
        };
        auto backup_parser = create_parser(backup_args);
        Packer backup_packer(backup_parser);
        REQUIRE(backup_packer.Pack() == true);
        
        WHEN("恢复到新位置") {
            fs::path restore_dir = "restored_data";
            std::vector<std::string> restore_args = {
                "-i", restore_dir.string(),
                "-o", (backup_dir / (test_dir.filename().string() + ".backup")).string(),
                "-r"
            };
            auto restore_parser = create_parser(restore_args);
            Packer restore_packer(restore_parser);
            
            THEN("恢复应该成功") {
                REQUIRE(restore_packer.Unpack() == true);
                
                AND_THEN("恢复的文件应该与原始文件相同") {
                    for (const auto& file : test_files) {
                        REQUIRE(fs::exists(restore_dir / file.path));
                    }
                }
            }
            
            fs::remove_all(restore_dir);
        }
    }
} 