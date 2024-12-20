#include "Packer.h"
#include <fstream>

bool Packer::Pack(const fs::path& source_path, const fs::path& target_path) {
    spdlog::info("开始打包: {} -> {}", source_path.string(), target_path.string());
    try {
        std::ofstream backup_file(target_path, std::ios::binary);
        if (!backup_file) {
            throw std::runtime_error("无法创建备份文件: " + target_path.string());
        }

        // 切换到源目录的父目录，使得相对路径正确
        std::filesystem::current_path(source_path);
        spdlog::info("切换工作目录到: {}", source_path.string());

        for (const auto &entry : fs::recursive_directory_iterator(source_path)) {
            const auto &path =  fs::path(entry.path()).lexically_relative(fs::current_path());
            
            // 过滤器判断是否需要打包该文件
            if (!filter_(path)) {
                spdlog::info("跳过文件: {}", path.string());
                continue;
            }

            spdlog::info("打包文件: {}", path.string());

            if (auto handler = FileHandler::Create(path)) {
                handler->Pack(backup_file, inode_table);
            } else {
                spdlog::warn("跳过未知文件类型: {}", path.string());
            }
        }

        return true;
    } catch (const std::exception &e) {
        spdlog::error("打包过程出错: {}", e.what());
        return false;
    }
}

bool Packer::Unpack(const fs::path& backup_path, const fs::path& restore_path) {
    spdlog::info("开始解包: {} -> {}", backup_path.string(), restore_path.string());
    try {
        std::ifstream backup_file(backup_path, std::ios::binary);
        if (!backup_file) {
            throw std::runtime_error("无法打开备份文件: " + backup_path.string());
        }

        // 创建目标目录
        fs::create_directories(restore_path);
        std::filesystem::current_path(restore_path);

        // 读取并解包文件
        while (backup_file.peek() != EOF) {
            FileHeader header;
            backup_file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
            
            spdlog::info("解包文件: {}", header.path);

            if (auto handler = FileHandler::Create(header)) {
                handler->Unpack(backup_file);
            } else {
                spdlog::warn("跳过未知文件类型: {}", std::string(header.path));
            }
        }

        spdlog::info("解包完成");
        return true;
    } catch (const std::exception &e) {
        spdlog::error("解包过程出错: {}", e.what());
        return false;
    }
}
