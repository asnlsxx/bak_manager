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
    }
}

SCENARIO_METHOD(TestFixture, "按文件类型过滤 - 仅符号链接",
                "[backup][filter][type]") {
    GIVEN("一个包含多种类型文件的目录") {
        std::vector<TestFile> files = {
            {"file1.txt", TestFileType::Regular, "普通文件1"},
            {"file2.txt", TestFileType::Regular, "普通文件2"},
            {"dir1", TestFileType::Directory},
            {"dir1/file3.txt", TestFileType::Regular, "普通文件3"},
            {"link1", TestFileType::Symlink, "", "file1.txt"},
            {"dir1/link2", TestFileType::Symlink, "", "../file2.txt"},
            {"pipe1", TestFileType::FIFO}
        };
        create_test_structure(files);

        WHEN("仅过滤符号链接") {
            cmdline::parser parser;
            ParserConfig::configure_parser(parser);
            const char* args[] = {
                "program",
                "-b",
                "-i", test_dir.string().c_str(),
                "-o", backup_dir.string().c_str(),
                "--type", "l"  // 只备份符号链接
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
            // 验证只有符号链接被备份
            REQUIRE_FALSE(fs::exists(project_dir / "file1.txt"));
            REQUIRE_FALSE(fs::exists(project_dir / "file2.txt"));
            REQUIRE_FALSE(fs::exists(project_dir / "dir1/file3.txt"));
            REQUIRE(fs::read_symlink(project_dir / "link1") == "file1.txt");
            REQUIRE(fs::read_symlink(project_dir / "dir1/link2") == "../file2.txt");
            REQUIRE_FALSE(fs::exists(project_dir / "pipe1"));

            fs::remove_all(restore_dir);
        }
    }
}

SCENARIO_METHOD(TestFixture, "按文件类型过滤 - 仅管道文件",
                "[backup][filter][type]") {
    GIVEN("一个包含多种类型文件的目录") {
        std::vector<TestFile> files = {
            {"file1.txt", TestFileType::Regular, "普通文件1"},
            {"dir1", TestFileType::Directory},
            {"dir1/file2.txt", TestFileType::Regular, "普通文件2"},
            {"link1", TestFileType::Symlink, "", "file1.txt"},
            {"pipe1", TestFileType::FIFO},
            {"dir1/pipe2", TestFileType::FIFO}
        };
        create_test_structure(files);

        WHEN("仅过滤管道文件") {
            cmdline::parser parser;
            ParserConfig::configure_parser(parser);
            const char* args[] = {
                "program",
                "-b",
                "-i", test_dir.string().c_str(),
                "-o", backup_dir.string().c_str(),
                "--type", "p"  // 只备份管道文件
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
            
            // 验证只有管道文件被备份
            REQUIRE_FALSE(fs::exists(project_dir / "file1.txt"));
            REQUIRE_FALSE(fs::exists(project_dir / "dir1/file2.txt"));
            REQUIRE_FALSE(fs::exists(project_dir / "link1"));
            REQUIRE(fs::exists(project_dir / "pipe1"));
            REQUIRE(fs::exists(project_dir / "dir1/pipe2"));

            fs::remove_all(restore_dir);
        }
    }
}

SCENARIO_METHOD(TestFixture, "按文件类型过滤 - 符号链接和管道文件组合",
                "[backup][filter][type]") {
    GIVEN("一个包含多种类型文件的目录") {
        std::vector<TestFile> files = {
            {"file1.txt", TestFileType::Regular, "普通文件1"},
            {"dir1", TestFileType::Directory},
            {"dir1/file2.txt", TestFileType::Regular, "普通文件2"},
            {"link1", TestFileType::Symlink, "", "file1.txt"},
            {"dir1/link2", TestFileType::Symlink, "", "../file1.txt"},
            {"pipe1", TestFileType::FIFO},
            {"dir1/pipe2", TestFileType::FIFO}
        };
        create_test_structure(files);

        WHEN("过滤符号链接和管道文件") {
            cmdline::parser parser;
            ParserConfig::configure_parser(parser);
            const char* args[] = {
                "program",
                "-b",
                "-i", test_dir.string().c_str(),
                "-o", backup_dir.string().c_str(),
                "--type", "lp"  // 备份符号链接和管道文件
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
            
            // 验证只有符号链接和管道文件被备份
            REQUIRE_FALSE(fs::exists(project_dir / "file1.txt"));
            REQUIRE_FALSE(fs::exists(project_dir / "dir1/file2.txt"));
            REQUIRE(fs::read_symlink(project_dir / "link1") == "file1.txt");
            REQUIRE(fs::read_symlink(project_dir / "dir1/link2") == "../file1.txt");
            REQUIRE(fs::exists(project_dir / "pipe1"));
            REQUIRE(fs::exists(project_dir / "dir1/pipe2"));

            fs::remove_all(restore_dir);
        }
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
            REQUIRE_FALSE(fs::exists(project_dir / "dir1/file4.txt"));
            
            fs::remove_all(restore_dir);
        }
    }
}

SCENARIO_METHOD(TestFixture, "按文件大小过滤 - 大于指定大小",
                "[backup][filter][size]") {
    GIVEN("一个包含不同大小文件的目录") {
        std::vector<TestFile> files = {
            {"small.txt", TestFileType::Regular, std::string(100, 'a')},      // 100B
            {"medium.txt", TestFileType::Regular, std::string(2000, 'b')},    // 2KB
            {"large.txt", TestFileType::Regular, std::string(5000, 'c')},     // 5KB
            {"dir1", TestFileType::Directory},
            {"dir1/test.txt", TestFileType::Regular, std::string(3000, 'd')}, // 3KB
        };
        create_test_structure(files);

        WHEN("过滤大于2KB的文件") {
            cmdline::parser parser;
            ParserConfig::configure_parser(parser);
            const char* args[] = {
                "program", "-b",
                "-i", test_dir.string().c_str(),
                "-o", backup_dir.string().c_str(),
                "--size", ">2000b"  // 大于2000字节
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
            REQUIRE_FALSE(fs::exists(project_dir / "small.txt"));
            REQUIRE(fs::exists(project_dir / "large.txt"));
            REQUIRE_FALSE(fs::exists(project_dir / "medium.txt"));
            REQUIRE(fs::exists(project_dir / "dir1"));
            REQUIRE(fs::exists(project_dir / "dir1/test.txt"));
            
            fs::remove_all(restore_dir);
        }
    }
}

SCENARIO_METHOD(TestFixture, "按文件大小过滤 - 小于指定大小",
                "[backup][filter][size]") {
    GIVEN("一个包含不同大小文件的目录") {
        std::vector<TestFile> files = {
            {"small.txt", TestFileType::Regular, std::string(100, 'a')},      // 100B
            {"medium.txt", TestFileType::Regular, std::string(2000, 'b')},    // 2KB
            {"large.txt", TestFileType::Regular, std::string(5000, 'c')},     // 5KB
            {"dir1", TestFileType::Directory},
            {"dir1/test.txt", TestFileType::Regular, std::string(3000, 'd')}, // 3KB
        };
        create_test_structure(files);

        WHEN("过滤小于3KB的文件") {
            cmdline::parser parser;
            ParserConfig::configure_parser(parser);
            const char* args[] = {
                "program", "-b",
                "-i", test_dir.string().c_str(),
                "-o", backup_dir.string().c_str(),
                "--size", "<3000b"  // 小于3000字节
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
            REQUIRE(fs::exists(project_dir / "small.txt"));
            REQUIRE(fs::exists(project_dir / "medium.txt"));
            REQUIRE_FALSE(fs::exists(project_dir / "large.txt"));
            REQUIRE(fs::exists(project_dir / "dir1"));
            REQUIRE_FALSE(fs::exists(project_dir / "dir1/test.txt"));
            
            fs::remove_all(restore_dir);
        }
    }
}

SCENARIO_METHOD(TestFixture, "测试压缩功能的有效性",
                "[backup][compression]") {
    GIVEN("一个包含高度可压缩内容的测试目录") {
        // 创建测试文件，包含高度可压缩的内容
        std::string repeated_hello;
        repeated_hello.reserve(25000);  // 预分配空间
        for (int i = 0; i < 5000; ++i) {
            repeated_hello += "hello";
        }

        std::vector<TestFile> files = {
            {"repeated.txt", TestFileType::Regular, std::string(10000, 'a')},  // 10KB 重复字符
            {"dir1", TestFileType::Directory},
            {"dir1/pattern.txt", TestFileType::Regular, repeated_hello}  // 25KB 重复模式
        };
        create_test_structure(files);

        WHEN("使用压缩选项进行备份") {
            Packer packer;
            packer.set_compress(true);
            
            fs::path backup_path = backup_dir / (test_dir.filename().string() + ".backup");
            REQUIRE(packer.Pack(test_dir, backup_path) == true);

            THEN("备份文件应该被显著压缩") {
                // 验证文件存在
                REQUIRE(fs::exists(backup_path));

                // 计算原始大小（不包括文件头等元数据）
                size_t original_size = 10000 + 25000;  // 35KB 文本数据
                size_t compressed_size = fs::file_size(backup_path);

                // 验证压缩效果
                REQUIRE(compressed_size < original_size / 2);  // 期望至少压缩50%
                
                // 验证文件完整性
                REQUIRE(packer.Verify(backup_path) == true);

                // 恢复文件并验证内容
                fs::path restore_path = backup_dir / "restored";
                REQUIRE(packer.Unpack(backup_path, restore_path) == true);

                // 验证恢复的文件内容
                fs::path restored_dir = restore_path / test_dir.filename();
                
                std::ifstream repeated_file(restored_dir / "repeated.txt");
                std::string repeated_content((std::istreambuf_iterator<char>(repeated_file)), {});
                REQUIRE(repeated_content == std::string(10000, 'a'));

                std::ifstream pattern_file(restored_dir / "dir1/pattern.txt");
                std::string pattern_content((std::istreambuf_iterator<char>(pattern_file)), {});
                REQUIRE(pattern_content == repeated_hello);

                fs::remove_all(restore_path);
            }
        }
    }
}

SCENARIO_METHOD(TestFixture, "测试加密功能的有效性",
                "[backup][encryption]") {
    GIVEN("一个包含敏感数据的测试目录") {
        // 创建测试文件
        std::vector<TestFile> files = {
            {"secret.txt", TestFileType::Regular, "这是一些敏感数据"},
            {"dir1", TestFileType::Directory},
            {"dir1/password.txt", TestFileType::Regular, "my_secret_password"}
        };
        create_test_structure(files);

        WHEN("使用密码加密备份") {
            Packer packer;
            packer.set_encrypt(true, "test_password");
            
            fs::path backup_path = backup_dir / (test_dir.filename().string() + ".backup");
            REQUIRE(packer.Pack(test_dir, backup_path) == true);

            THEN("备份文件应该被正确加密") {
                // 验证文件存在
                REQUIRE(fs::exists(backup_path));
                
                // 验证文件完整性
                REQUIRE(packer.Verify(backup_path) == true);

                // 尝试不提供密码恢复
                {
                    Packer wrong_packer;
                    fs::path wrong_restore_path = backup_dir / "wrong_restore";
                    REQUIRE(wrong_packer.Unpack(backup_path, wrong_restore_path) == false);
                }

                // 使用错误密码恢复
                {
                    Packer wrong_packer;
                    wrong_packer.set_encrypt(true, "wrong_password");
                    fs::path wrong_restore_path = backup_dir / "wrong_restore";
                    REQUIRE(wrong_packer.Unpack(backup_path, wrong_restore_path) == false);
                }

                // 使用正确密码恢复
                fs::path restore_path = backup_dir / "restored";
                Packer restore_packer;
                restore_packer.set_encrypt(true, "test_password");
                REQUIRE(restore_packer.Unpack(backup_path, restore_path) == true);

                // 验证恢复的文件内容
                fs::path restored_dir = restore_path / test_dir.filename();
                
                std::ifstream secret_file(restored_dir / "secret.txt");
                std::string secret_content((std::istreambuf_iterator<char>(secret_file)), {});
                REQUIRE(secret_content == "这是一些敏感数据");

                std::ifstream password_file(restored_dir / "dir1/password.txt");
                std::string password_content((std::istreambuf_iterator<char>(password_file)), {});
                REQUIRE(password_content == "my_secret_password");

                fs::remove_all(restore_path);
            }
        }
    }
}

SCENARIO_METHOD(TestFixture, "测试加密和压缩的组合功能",
                "[backup][encryption][compression]") {
    GIVEN("一个包含可压缩的敏感数据的测试目录") {
        // 创建包含重复内容的敏感数据
        std::string repeated_secret;
        repeated_secret.reserve(10000);
        for (int i = 0; i < 1000; ++i) {
            repeated_secret += "sensitive_data_block_";
        }

        std::vector<TestFile> files = {
            {"large_secret.txt", TestFileType::Regular, repeated_secret},
            {"dir1", TestFileType::Directory},
            {"dir1/config.txt", TestFileType::Regular, std::string(500, 'S')}  // 500个S
        };
        create_test_structure(files);

        WHEN("同时使用压缩和加密进行备份") {
            Packer packer;
            packer.set_compress(true);
            packer.set_encrypt(true, "test_password");
            
            fs::path backup_path = backup_dir / (test_dir.filename().string() + ".backup");
            REQUIRE(packer.Pack(test_dir, backup_path) == true);

            THEN("备份文件应该被压缩且加密") {
                // 验证文件存在
                REQUIRE(fs::exists(backup_path));
                
                // 验证压缩效果（文件应该明显小于原始大小）
                size_t original_size = repeated_secret.size() + 500;
                size_t compressed_size = fs::file_size(backup_path);
                REQUIRE(compressed_size < original_size / 2);

                // 验证文件完整性
                REQUIRE(packer.Verify(backup_path) == true);

                // 使用正确密码恢复
                fs::path restore_path = backup_dir / "restored";
                Packer restore_packer;
                restore_packer.set_encrypt(true, "test_password");
                REQUIRE(restore_packer.Unpack(backup_path, restore_path) == true);

                // 验证恢复的文件内容
                fs::path restored_dir = restore_path / test_dir.filename();
                
                std::ifstream large_file(restored_dir / "large_secret.txt");
                std::string large_content((std::istreambuf_iterator<char>(large_file)), {});
                REQUIRE(large_content == repeated_secret);

                std::ifstream config_file(restored_dir / "dir1/config.txt");
                std::string config_content((std::istreambuf_iterator<char>(config_file)), {});
                REQUIRE(config_content == std::string(500, 'S'));

                fs::remove_all(restore_path);
            }
        }
    }
}

SCENARIO_METHOD(TestFixture, "测试验证功能在不同模式下的表现",
                "[backup][verify]") {
    GIVEN("一个包含各种类型文件的测试目录") {
        // 创建测试文件
        std::string repeated_data(10000, 'R');  // 可压缩数据
        std::string random_data;                 // 不易压缩数据
        random_data.reserve(10000);
        for(int i = 0; i < 10000; i++) {
            random_data += static_cast<char>(rand() % 256);
        }

        std::vector<TestFile> files = {
            {"compressible.txt", TestFileType::Regular, repeated_data},
            {"random.bin", TestFileType::Regular, random_data},
            {"dir1", TestFileType::Directory},
            {"dir1/config.txt", TestFileType::Regular, "important=true\nkey=value"}
        };
        create_test_structure(files);

        WHEN("同时使用压缩和加密模式备份") {
            Packer packer;
            packer.set_compress(true);
            packer.set_encrypt(true, "test_password");
            
            fs::path backup_path = backup_dir / (test_dir.filename().string() + ".backup");
            REQUIRE(packer.Pack(test_dir, backup_path) == true);

            THEN("验证应该检查压缩和加密状态") {
                // 使用正确配置验证
                Packer verify_packer;
                verify_packer.set_encrypt(true, "test_password");
                REQUIRE(verify_packer.Verify(backup_path) == true);

                // 修改备份文件内容以测试验证失败情况
                {
                    std::fstream file(backup_path, std::ios::in | std::ios::out | std::ios::binary);
                    file.seekp(0, std::ios::end);
                    std::streampos file_size = file.tellp();
                    
                    // 修改文件中间的一个字节
                    file.seekp(file_size / 2, std::ios::beg);
                    char c;
                    file.read(&c, 1);
                    c = ~c; // 对字节取反来修改内容
                    file.seekp(-1, std::ios::cur);
                    file.write(&c, 1);
                    file.close();
                }
                
                Packer corrupt_verify_packer;
                corrupt_verify_packer.set_encrypt(true, "test_password");
                REQUIRE(corrupt_verify_packer.Verify(backup_path) == false);
            }
        }
    }
}