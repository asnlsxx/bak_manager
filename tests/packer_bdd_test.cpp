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
    
    // 执行备份和恢复测试
    void test_backup_and_restore() {
        std::vector<std::string> backup_args = {
            "-i", test_dir.string(),
            "-o", backup_dir.string(),
            "-b"
        };
        auto backup_parser = create_parser(backup_args);
        Packer backup_packer(backup_parser);
        
        REQUIRE(backup_packer.Pack() == true);
        
        auto backup_file = backup_dir / (test_dir.filename().string() + ".backup");
        REQUIRE(fs::exists(backup_file));
        
        fs::path restore_dir = "restored_data";
        std::vector<std::string> restore_args = {
            "-i", restore_dir.string(),
            "-o", backup_file.string(),
            "-r"
        };
        auto restore_parser = create_parser(restore_args);
        Packer restore_packer(restore_parser);
        
        REQUIRE(restore_packer.Unpack() == true);
        
        // 验证恢复的文件
        for (const auto& file : test_files) {
            REQUIRE(fs::exists(restore_dir / file.path));
            
            if (file.type == TestFileType::Regular && !file.is_hardlink) {
                std::ifstream restored_file(restore_dir / file.path);
                std::string content;
                std::getline(restored_file, content);
                REQUIRE(content == file.content);
            }
        }
        
        fs::remove_all(restore_dir);
    }
    
private:
    // 创建命令行解析器
    cmdline::parser create_parser(const std::vector<std::string>& args) {
        cmdline::parser parser;
        parser.add<std::string>("input", 'i', "输入路径", true);
        parser.add<std::string>("output", 'o', "输出路径", true);
        parser.add("backup", 'b', "备份");
        parser.add("restore", 'r', "恢复");
        parser.add<std::string>("type", '\0', "文件类型: n普通文件,d目录文件,l符号链接", false);
        parser.add<std::string>("name", '\0', "文件名", false);
        parser.add<std::string>("path", '\0', "路径", false);
        
        std::vector<const char*> argv;
        argv.push_back("test");
        for (const auto& arg : args) {
            argv.push_back(arg.c_str());
        }
        
        parser.parse_check(argv.size(), const_cast<char**>(argv.data()));
        return parser;
    }
};

SCENARIO_METHOD(TestFixture, "备份和恢复基本文件结构", "[backup][restore][basic]") {
    GIVEN("一个包含基本文件类型的目录") {
        std::vector<TestFile> files = {
            {"file1.txt", TestFileType::Regular, "测试文件1"},
            {"subdir", TestFileType::Directory},
            {"subdir/file2.txt", TestFileType::Regular, "测试文件2"},
            {"link1", TestFileType::Symlink, "", "file1.txt"},
            {"hardlink1", TestFileType::Regular, "", "file1.txt", true}
        };
        create_test_structure(files);
        test_backup_and_restore();
    }
}

SCENARIO_METHOD(TestFixture, "备份和恢复纯目录结构", "[backup][restore][directories]") {
    GIVEN("一个只包含目录的层级结构") {
        std::vector<TestFile> files = {
            {"dir1", TestFileType::Directory},
            {"dir1/subdir1", TestFileType::Directory},
            {"dir1/subdir2", TestFileType::Directory},
            {"dir2", TestFileType::Directory},
            {"dir2/subdir1", TestFileType::Directory},
            {"dir2/subdir1/subsubdir", TestFileType::Directory}
        };
        create_test_structure(files);
        test_backup_and_restore();
    }
}

SCENARIO_METHOD(TestFixture, "备份和恢复纯文件结构", "[backup][restore][files]") {
    GIVEN("一个只包含普通文件的目录") {
        std::vector<TestFile> files = {
            {"file1.txt", TestFileType::Regular, "文件1内容"},
            {"file2.txt", TestFileType::Regular, "文件2内容"},
            {"file3.dat", TestFileType::Regular, "二进制数据"},
            {"file4.log", TestFileType::Regular, "日志内容"},
            {"file5", TestFileType::Regular, "无扩展名文件"}
        };
        create_test_structure(files);
        test_backup_and_restore();
    }
}

SCENARIO_METHOD(TestFixture, "备份和恢复复杂链接结构", "[backup][restore][links]") {
    GIVEN("一个包含复杂链接关系的目录") {
        std::vector<TestFile> files = {
            {"data.txt", TestFileType::Regular, "源文件"},
            {"links", TestFileType::Directory},
            {"links/link1", TestFileType::Symlink, "", "../data.txt"},
            {"links/link2", TestFileType::Symlink, "", "link1"},
            {"links/hardlink1", TestFileType::Regular, "", "../data.txt", true},
            {"links/subdir", TestFileType::Directory},
            {"links/subdir/hardlink2", TestFileType::Regular, "", "../../data.txt", true}
        };
        create_test_structure(files);
        test_backup_and_restore();
    }
}

SCENARIO_METHOD(TestFixture, "备份和恢复深层嵌套结构", "[backup][restore][nested]") {
    GIVEN("一个具有深层嵌套的混合结构目录") {
        std::vector<TestFile> files = {
            {"level1", TestFileType::Directory},
            {"level1/file1.txt", TestFileType::Regular, "level1文件"},
            {"level1/level2", TestFileType::Directory},
            {"level1/level2/file2.txt", TestFileType::Regular, "level2文件"},
            {"level1/level2/link1", TestFileType::Symlink, "", "../file1.txt"},
            {"level1/level2/level3", TestFileType::Directory},
            {"level1/level2/level3/file3.txt", TestFileType::Regular, "level3文件"},
            {"level1/level2/level3/hardlink1", TestFileType::Regular, "", "../../file2.txt", true}
        };
        create_test_structure(files);
        test_backup_and_restore();
    }
} 