#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <vector>
#include "Packer.h"
#include "ArgParser.h"

namespace fs = std::filesystem;

// 定义文件类型
enum class TestFileType {
    Regular,    // 普通文件(包括硬链接)
    Directory,  // 目录
    Symlink,    // 符号链接
    FIFO,       // 管道文件
};

// 定义测试文件结构
struct TestFile {
    std::string path;     // 相对路径
    TestFileType type;    // 文件类型
    std::string content;  // 文件内容(仅普通文件)
    std::string target;   // 链接目标(符号链接)或硬链接源(硬链接)
    bool is_hardlink{false};  // 是否是硬链接
};

class TestFixture {
protected:
    fs::path test_dir;
    fs::path backup_dir;
    std::vector<TestFile> test_files;

    TestFixture(const std::string &test_dir_name = "test_data",
                const std::string &backup_dir_name = "backup")
        : test_dir(fs::absolute(test_dir_name)), 
          backup_dir(fs::absolute(backup_dir_name)) {}

    // 创建测试目录结构
    void create_test_structure(const std::vector<TestFile> &files) {
        test_files = files;

        // 清理并创建基本目录
        fs::remove_all(test_dir);
        fs::remove_all(backup_dir);
        fs::create_directories(test_dir);
        fs::create_directories(backup_dir);

        // 创建测试文件
        for (const auto &file : files) {
            fs::path file_path = test_dir / file.path;

            // 确保父目录存在
            if (file.type != TestFileType::Directory) {
                fs::create_directories(file_path.parent_path());
            }

            switch (file.type) {
                case TestFileType::Regular:
                    if (file.is_hardlink) {
                        fs::create_hard_link(file_path.parent_path() / file.target, file_path);
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

                case TestFileType::FIFO:
                    if (mkfifo(file_path.c_str(), 0666) != 0) {
                        throw std::runtime_error("无法创建管道文件: " + file_path.string());
                    }
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
        {
            Packer packer;
            fs::path backup_path = backup_dir / (test_dir.filename().string() + ".backup");
            REQUIRE(packer.Pack(test_dir, backup_path) == true);
            REQUIRE(fs::exists(backup_path));
        }
        {
            Packer packer;
            fs::path backup_path = backup_dir / (test_dir.filename().string() + ".backup");
            REQUIRE(packer.Verify(backup_path) == true);
        }
        // 恢复操作
        {
            fs::path restore_dir = fs::absolute("restored_data");
            fs::remove_all(restore_dir); // 清理可能存在的恢复目录
            
            fs::path backup_file = backup_dir / (test_dir.filename().string() + ".backup");
            
            Packer packer;
            REQUIRE(packer.Unpack(backup_file, restore_dir) == true);
            fs::path project_dir = restore_dir / test_dir.filename();
            // 验证恢复的文件
            for (const auto &file : test_files) {
                fs::path restored_path = project_dir / file.path;
                
                std::cout << "验证文件" << restored_path << std::endl;
                REQUIRE(fs::exists(restored_path));
                
                // 根据文件类型进行具体验证
                switch (file.type) {
                    case TestFileType::Regular:
                        if (!file.is_hardlink) {
                            std::ifstream restored_file(restored_path);
                            std::string content;
                            std::getline(restored_file, content);
                            REQUIRE(content == file.content);
                        } else {
                            fs::path target_path = restored_path.parent_path() / file.target;
                            REQUIRE(fs::exists(target_path));
                            REQUIRE(fs::hard_link_count(restored_path) > 1);
                            REQUIRE(fs::equivalent(restored_path, target_path));
                        }
                        break;

                    case TestFileType::Directory:
                        REQUIRE(fs::is_directory(restored_path));
                        break;

                    case TestFileType::Symlink:
                        REQUIRE(fs::is_symlink(restored_path));
                        REQUIRE(fs::read_symlink(restored_path) == file.target);
                        break;
                }
            }

            fs::remove_all(restore_dir);
        }
    }

    // 辅助函数：设置测试文件的时间戳（移到 TestFixture 类中）
    void setup_time_test_files() {
        // 创建基本文件结构
        std::vector<TestFile> files = {
            {"file1.txt", TestFileType::Regular, "文件1"},
            {"file2.txt", TestFileType::Regular, "文件2"},
            {"file3.txt", TestFileType::Regular, "文件3"},
            {"dir1", TestFileType::Directory},
            {"dir1/file4.txt", TestFileType::Regular, "文件4"}
        };
        create_test_structure(files);

        // 设置不同的时间戳
        struct timespec times[2];
        struct tm tm = {};

        // 设置file1的时间为2024-01-01 10:00
        tm = {};
        tm.tm_year = 2024 - 1900;
        tm.tm_mon = 0;   // 一月
        tm.tm_mday = 1;
        tm.tm_hour = 10;
        times[0].tv_sec = times[1].tv_sec = mktime(&tm);
        times[0].tv_nsec = times[1].tv_nsec = 0;
        utimensat(AT_FDCWD, (test_dir / "file1.txt").c_str(), times, 0);

        // 设置file2的时间为2024-01-02 15:30
        tm.tm_mday = 2;
        tm.tm_hour = 15;
        tm.tm_min = 30;
        times[0].tv_sec = times[1].tv_sec = mktime(&tm);
        utimensat(AT_FDCWD, (test_dir / "file2.txt").c_str(), times, 0);

        // 设置file3的时间为2024-01-03 20:00
        tm.tm_mday = 3;
        tm.tm_hour = 20;
        tm.tm_min = 0;
        times[0].tv_sec = times[1].tv_sec = mktime(&tm);
        utimensat(AT_FDCWD, (test_dir / "file3.txt").c_str(), times, 0);
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

SCENARIO_METHOD(TestFixture, "备份和恢复纯目录结构",
                "[backup][restore][directories]") {
  GIVEN("一个只包含目录的层级结构") {
    std::vector<TestFile> files = {
        {"dir1", TestFileType::Directory},
        {"dir1/subdir1", TestFileType::Directory},
        {"dir1/subdir2", TestFileType::Directory},
        {"dir2", TestFileType::Directory},
        {"dir2/subdir1", TestFileType::Directory},
        {"dir2/subdir1/subsubdir", TestFileType::Directory}};
    create_test_structure(files);
    test_backup_and_restore();
  }
}

SCENARIO_METHOD(TestFixture, "备份和恢复纯文件结构",
                "[backup][restore][files]") {
  GIVEN("一个只包含普通文件的目录") {
    std::vector<TestFile> files = {
        {"file1.txt", TestFileType::Regular, "文件1内容"},
        {"file2.txt", TestFileType::Regular, "文件2内容"},
        {"file3.dat", TestFileType::Regular, "二进制数据"},
        {"file4.log", TestFileType::Regular, "日志内容"},
        {"file5", TestFileType::Regular, "无扩展名文件"}};
    create_test_structure(files);
    test_backup_and_restore();
  }
}

SCENARIO_METHOD(TestFixture, "备份和恢复复杂链接结构",
                "[backup][restore][links]") {
  GIVEN("一个包含复杂链接关系的目录") {
    std::vector<TestFile> files = {
        {"data.txt", TestFileType::Regular, "源文件"},
        {"links", TestFileType::Directory},
        {"links/link1", TestFileType::Symlink, "", "../data.txt"},
        {"links/link2", TestFileType::Symlink, "", "link1"},
        {"links/hardlink1", TestFileType::Regular, "", "../data.txt", true},
        {"links/subdir", TestFileType::Directory},
        {"links/subdir/hardlink2", TestFileType::Regular, "", "../../data.txt",
         true}};
    create_test_structure(files);
    test_backup_and_restore();
  }
}

SCENARIO_METHOD(TestFixture, "备份和恢复深层嵌套结构",
                "[backup][restore][nested]") {
  GIVEN("一个具有深层嵌套的混合结构目录") {
    std::vector<TestFile> files = {
        {"level1", TestFileType::Directory},
        {"level1/file1.txt", TestFileType::Regular, "level1文件"},
        {"level1/level2", TestFileType::Directory},
        {"level1/level2/file2.txt", TestFileType::Regular, "level2文件"},
        {"level1/level2/link1", TestFileType::Symlink, "", "../file1.txt"},
        {"level1/level2/level3", TestFileType::Directory},
        {"level1/level2/level3/file3.txt", TestFileType::Regular, "level3文件"},
        {"level1/level2/level3/hardlink1", TestFileType::Regular, "",
         "../file2.txt", true}};
    create_test_structure(files);
    test_backup_and_restore();
  }
}

SCENARIO_METHOD(TestFixture, "备份和恢复管道文件",
                "[backup][restore][fifo]") {
    GIVEN("一个包含管道文件的目录") {
        std::vector<TestFile> files = {
            {"pipe1", TestFileType::FIFO},
            {"subdir", TestFileType::Directory},
            {"subdir/pipe2", TestFileType::FIFO},
            {"file1.txt", TestFileType::Regular, "普通文件"},
        };
        create_test_structure(files);
        test_backup_and_restore();
    }
}

SCENARIO_METHOD(TestFixture, "备份时的文件过滤功能",
                "[backup][filter]") {
    GIVEN("一个包含多种类型文件的目录") {
        std::vector<TestFile> files = {
            {"file1.txt", TestFileType::Regular, "文本文件1"},
            {"file2.dat", TestFileType::Regular, "数据文件2"},
            {"dir1", TestFileType::Directory},
            {"dir1/file3.txt", TestFileType::Regular, "文本文件3"},
            {"dir1/pipe1", TestFileType::FIFO},
            {"link1", TestFileType::Symlink, "", "file1.txt"},
            {"dir2", TestFileType::Directory},
            {"dir2/file4.log", TestFileType::Regular, "日志文件4"},
        };
        create_test_structure(files);

        WHEN("按文件类型过滤") {
            cmdline::parser parser;
            ParserConfig::configure_parser(parser);
            const char* args[] = {
                "program",
                "-b",
                "-i", test_dir.string().c_str(),
                "-o", backup_dir.string().c_str(),
                "--type", "n",  // 只备份普通文件
            };
            parser.parse_check(sizeof(args) / sizeof(args[0]), const_cast<char**>(args));

            Packer packer;
            packer.set_filter(ParserConfig::create_filter(parser));
            
            fs::path backup_path = backup_dir / (test_dir.filename().string() + ".backup");
            REQUIRE(packer.Pack(test_dir, backup_path) == true);

            // 恢复并验证
            fs::path restore_dir = fs::absolute("restored_data");
            packer.Unpack(backup_path, restore_dir);
            
            fs::path project_dir = restore_dir / test_dir.filename();
            // 验证只有普通文件被备份
            REQUIRE(fs::exists(project_dir / "file1.txt"));
            REQUIRE(fs::exists(project_dir / "file2.dat"));
            REQUIRE(fs::exists(project_dir / "dir1/file3.txt"));
            REQUIRE(fs::exists(project_dir / "dir2/file4.log"));
            REQUIRE_FALSE(fs::exists(project_dir / "dir1/pipe1"));
            REQUIRE_FALSE(fs::exists(project_dir / "link1"));

            fs::remove_all(restore_dir);
        }

        // WHEN("按路径过滤") {
        //     cmdline::parser parser;
        //     ParserConfig::configure_parser(parser);
        //     const char* args[] = {
        //         "program",
        //         "-b",
        //         "-i", test_dir.string().c_str(),
        //         "-o", backup_dir.string().c_str(),
        //         "--path", "dir1/.*",  // 只备份dir1目录下的文件
        //     };
        //     parser.parse_check(sizeof(args) / sizeof(args[0]), const_cast<char**>(args));

        //     Packer packer;
        //     packer.set_filter(ParserConfig::create_filter(parser));
            
        //     fs::path backup_path = backup_dir / (test_dir.filename().string() + ".backup");
        //     REQUIRE(packer.Pack(test_dir, backup_path) == true);

        //     // 恢复并验证
        //     fs::path restore_dir = fs::absolute("restored_data");
        //     packer.Unpack(backup_path, restore_dir);
            
        //     fs::path project_dir = restore_dir / test_dir.filename();
        //     // 验证只有dir1目录下的文件被备份
        //     REQUIRE_FALSE(fs::exists(project_dir / "file1.txt"));
        //     REQUIRE(fs::exists(project_dir / "dir1/file3.txt"));
        //     REQUIRE(fs::exists(project_dir / "dir1/pipe1"));
        //     REQUIRE_FALSE(fs::exists(project_dir / "dir2/file4.log"));

        //     fs::remove_all(restore_dir);
        // }
    }
}

SCENARIO_METHOD(TestFixture, "备份时的文件名过滤功能",
                "[backup][filter]") {
    GIVEN("一个包含多种类型文件的目录") {
        std::vector<TestFile> files = {
            {"file1.txt", TestFileType::Regular, "文本文件1"},
            {"file2.dat", TestFileType::Regular, "数据文件2"},
            {"dir1", TestFileType::Directory},
            {"dir1/file3.txt", TestFileType::Regular, "文本文件3"},
            {"dir1/pipe1", TestFileType::FIFO},
            {"link1", TestFileType::Symlink, "", "file1.txt"},
            {"dir2", TestFileType::Directory},
            {"dir2/file4.log", TestFileType::Regular, "日志文件4"},
        };
        create_test_structure(files);

        WHEN("按文件名过滤") {
            REQUIRE(true);
            cmdline::parser parser;
            ParserConfig::configure_parser(parser);
            const char* args[] = {
                "program",
                "-b",
                "-i", test_dir.string().c_str(),
                "-o", backup_dir.string().c_str(),
                "--name", ".*\\.txt",  // 只备份.txt文件
            };
            parser.parse_check(sizeof(args) / sizeof(args[0]), const_cast<char**>(args));

            Packer packer;
            packer.set_filter(ParserConfig::create_filter(parser));
            
            fs::path backup_path = backup_dir / (test_dir.filename().string() + ".backup");
            REQUIRE(packer.Pack(test_dir, backup_path) == true);

            // 恢复并验证
            fs::path restore_dir = fs::absolute("restored_data");
            packer.Unpack(backup_path, restore_dir);
            
            fs::path project_dir = restore_dir / test_dir.filename();
            // 验证只有.txt文件被备份
            REQUIRE(fs::exists(project_dir / "file1.txt"));
            REQUIRE_FALSE(fs::exists(project_dir / "file2.dat"));
            REQUIRE(fs::exists(project_dir / "dir1/file3.txt"));
            REQUIRE_FALSE(fs::exists(project_dir / "dir2/file4.log"));
            REQUIRE_FALSE(fs::exists(project_dir / "dir1/pipe1"));
            REQUIRE_FALSE(fs::exists(project_dir / "link1"));


            fs::remove_all(restore_dir);
        }


    }
}            

SCENARIO_METHOD(TestFixture, "备份时的文件路径过滤功能",
                "[backup][filter]") {
    GIVEN("一个包含多种类型文件的目录") {
        std::vector<TestFile> files = {
            {"file1.txt", TestFileType::Regular, "文本文件1"},
            {"file2.dat", TestFileType::Regular, "数据文件2"},
            {"dir1", TestFileType::Directory},
            {"dir1/file3.txt", TestFileType::Regular, "文本文件3"},
            {"dir1/pipe1", TestFileType::FIFO},
            {"link1", TestFileType::Symlink, "", "file1.txt"},
            {"dir2", TestFileType::Directory},
            {"dir2/file4.log", TestFileType::Regular, "日志文件4"},
        };
        create_test_structure(files);

        WHEN("按路径过滤") {
            cmdline::parser parser;
            ParserConfig::configure_parser(parser);
            const char* args[] = {
                "program",
                "-b",
                "-i", test_dir.string().c_str(),
                "-o", backup_dir.string().c_str(),
                "--path", "dir1/.*",  // 只备份dir1目录下的文件
            };
            parser.parse_check(sizeof(args) / sizeof(args[0]), const_cast<char**>(args));

            Packer packer;
            packer.set_filter(ParserConfig::create_filter(parser));
            
            fs::path backup_path = backup_dir / (test_dir.filename().string() + ".backup");
            REQUIRE(packer.Pack(test_dir, backup_path) == true);

            // 恢复并验证
            fs::path restore_dir = fs::absolute("restored_data");
            packer.Unpack(backup_path, restore_dir);
            
            fs::path project_dir = restore_dir / test_dir.filename();
            // 验证只有dir1目录下的文件被备份
            REQUIRE_FALSE(fs::exists(project_dir / "file1.txt"));
            REQUIRE(fs::exists(project_dir / "dir1/file3.txt"));
            REQUIRE(fs::exists(project_dir / "dir1/pipe1"));
            REQUIRE_FALSE(fs::exists(project_dir / "dir2/file4.log"));

            fs::remove_all(restore_dir);
        }
    }
}

SCENARIO_METHOD(TestFixture, "按修改时间过滤（1月1日到1月2日）",
                "[backup][filter][time]") {
    GIVEN("一个包含不同时间的文件的目录") {
        setup_time_test_files();
        WHEN("执行备份") {
            cmdline::parser parser;
            ParserConfig::configure_parser(parser);
            const char* args[] = {
                "program", "-b",
                "-i", test_dir.string().c_str(),
                "-o", backup_dir.string().c_str(),
                "--mtime", "202401010000,202401022359"
            };
            parser.parse_check(sizeof(args) / sizeof(args[0]), const_cast<char**>(args));

            Packer packer;
            packer.set_filter(ParserConfig::create_filter(parser));
            
            fs::path backup_path = backup_dir / (test_dir.filename().string() + ".backup");
            REQUIRE(packer.Pack(test_dir, backup_path) == true);

            // 恢复并验证
            fs::path restore_dir = fs::absolute("restored_data");
            packer.Unpack(backup_path, restore_dir);
            
            fs::path project_dir = restore_dir / test_dir.filename();
            REQUIRE(fs::exists(project_dir / "file1.txt"));
            REQUIRE(fs::exists(project_dir / "file2.txt"));
            REQUIRE_FALSE(fs::exists(project_dir / "file3.txt"));
            REQUIRE(fs::exists(project_dir / "dir1"));
            
            fs::remove_all(restore_dir);
        }
    }
}

SCENARIO_METHOD(TestFixture, "按修改时间过滤（1月2日到1月3日）",
                "[backup][filter][time]") {
    GIVEN("一个包含不同时间的文件的目录") {
        setup_time_test_files();
        WHEN("执行备份") {
            cmdline::parser parser;
            ParserConfig::configure_parser(parser);
            const char* args[] = {
                "program", "-b",
                "-i", test_dir.string().c_str(),
                "-o", backup_dir.string().c_str(),
                "--mtime", "202401020000,202401032359"
            };
            parser.parse_check(sizeof(args) / sizeof(args[0]), const_cast<char**>(args));

            Packer packer;
            packer.set_filter(ParserConfig::create_filter(parser));
            
            fs::path backup_path = backup_dir / (test_dir.filename().string() + ".backup");
            REQUIRE(packer.Pack(test_dir, backup_path) == true);

            // 恢复并验证
            fs::path restore_dir = fs::absolute("restored_data");
            packer.Unpack(backup_path, restore_dir);
            
            fs::path project_dir = restore_dir / test_dir.filename();
            REQUIRE_FALSE(fs::exists(project_dir / "file1.txt"));
            REQUIRE(fs::exists(project_dir / "file2.txt"));
            REQUIRE(fs::exists(project_dir / "file3.txt"));
            REQUIRE(fs::exists(project_dir / "dir1"));
            
            fs::remove_all(restore_dir);
        }
    }
}


SCENARIO_METHOD(TestFixture, "组合使用时间和文件名过滤",
                "[backup][filter][time]") {
    GIVEN("一个包含不同时间的文件的目录") {
        setup_time_test_files();
        WHEN("执行备份") {
            cmdline::parser parser;
            ParserConfig::configure_parser(parser);
            const char* args[] = {
                "program", "-b",
                "-i", test_dir.string().c_str(),
                "-o", backup_dir.string().c_str(),
                "--mtime", "202401010000,202401022359",
                "--name", "file[12]\\.txt"
            };
            parser.parse_check(sizeof(args) / sizeof(args[0]), const_cast<char**>(args));

            Packer packer;
            packer.set_filter(ParserConfig::create_filter(parser));
            
            fs::path backup_path = backup_dir / (test_dir.filename().string() + ".backup");
            REQUIRE(packer.Pack(test_dir, backup_path) == true);

            // 恢复并验证
            fs::path restore_dir = fs::absolute("restored_data");
            packer.Unpack(backup_path, restore_dir);
            
            fs::path project_dir = restore_dir / test_dir.filename();
            REQUIRE(fs::exists(project_dir / "file1.txt"));
            REQUIRE(fs::exists(project_dir / "file2.txt"));
            REQUIRE_FALSE(fs::exists(project_dir / "file3.txt"));
            REQUIRE_FALSE(fs::exists(project_dir / "dir1"));
            REQUIRE_FALSE(fs::exists(project_dir / "dir1/file4.txt"));
            
            fs::remove_all(restore_dir);
        }
    }
}