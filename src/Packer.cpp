#include "Packer.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <stdexcept>


namespace fs = std::filesystem;

Packer::Packer(std::string root_path_, std::string pack_path_) {
  // 构造函数实现
}

Packer::~Packer() {
  // 析构函数实现
}


bool Packer::Pack() {
    try {
        // 创建备份文件
        std::ofstream backup_file(pack_path_, std::ios::binary);
        if (!backup_file) {
            std::cerr << "无法创建备份文件: " << pack_path_ << std::endl;
            return false;
        }

        // 递归遍历根目录
        for (const auto& entry : fs::recursive_directory_iterator(root_path_)) {
            if (fs::is_regular_file(entry)) {
                FileBase file(entry.path().string());
                
                // 写入文件头
                file.WriteHeader(backup_file);
                
                // 写入文件内容
                std::ifstream input_file(entry.path(), std::ios::binary);
                if (!input_file) {
                    std::cerr << "无法打开文件: " << entry.path() << std::endl;
                    continue;
                }
                
                backup_file << input_file.rdbuf();
                input_file.close();
            }
        }

        backup_file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "打包过程中发生错误: " << e.what() << std::endl;
        return false;
    }
}

bool Packer::Unpack() {return false;}
