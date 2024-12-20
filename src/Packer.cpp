#include "Packer.h"
#include <fstream>

uint32_t Packer::calculateCRC32(const char* data, size_t length, uint32_t crc) const {
    static const std::array<uint32_t, 256> crc32_table = []() {
        std::array<uint32_t, 256> table;
        constexpr uint32_t POLYNOMIAL = 0xEDB88320;
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (uint32_t j = 0; j < 8; j++) {
                c = (c >> 1) ^ ((c & 1) ? POLYNOMIAL : 0);
            }
            table[i] = c;
        }
        return table;
    }();

    crc = ~crc;
    while (length--) {
        crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ static_cast<uint8_t>(*data++)];
    }
    return ~crc;
}

bool Packer::Pack(const fs::path& source_path, const fs::path& target_path) {
    spdlog::info("开始打包: {} -> {}", source_path.string(), target_path.string());
    try {
        std::ofstream backup_file(target_path, std::ios::binary);
        if (!backup_file) {
            throw std::runtime_error("无法创建备份文件: " + target_path.string());
        }

        // 写入备份信息头，校验和先设为0
        backup_header_.timestamp = std::time(nullptr);
        backup_header_.checksum = 0;
        backup_file.write(reinterpret_cast<const char*>(&backup_header_), sizeof(BackupHeader));

        // 切换到源目录，使得相对路径正确
        std::filesystem::current_path(source_path);
        spdlog::info("切换工作目录到: {}", source_path.string());

        // 先将所有数据写入备份文件
        for (const auto &entry : fs::recursive_directory_iterator(source_path)) {
            const auto &path = fs::path(entry.path()).lexically_relative(fs::current_path());
            
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

        // 关闭输出流并重新打开用于读取
        backup_file.close();
        std::ifstream check_file(target_path, std::ios::binary);

        // 计算校验和
        check_file.seekg(sizeof(BackupHeader));  // 跳过备份信息头
        std::vector<char> buffer(4096);
        uint32_t checksum = 0xFFFFFFFF;

        while (check_file) {
            check_file.read(buffer.data(), buffer.size());
            std::streamsize count = check_file.gcount();
            if (count > 0) {
                checksum = calculateCRC32(buffer.data(), count, checksum);
            }
        }

        // 更新校验和
        backup_header_.checksum = checksum;
        
        // 重新打开文件用于更新校验和
        std::ofstream update_file(target_path, std::ios::binary | std::ios::in | std::ios::out);
        update_file.write(reinterpret_cast<const char*>(&backup_header_), sizeof(BackupHeader));

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

        // 读取备份信息（并跳过）
        BackupHeader stored_header;
        backup_file.read(reinterpret_cast<char*>(&stored_header), sizeof(BackupHeader));
        
        // 在目标路径下创建以备份文件名命名的文件夹
        fs::path project_dir = restore_path / backup_path.stem();
        fs::create_directories(project_dir);
        std::filesystem::current_path(project_dir);
        
        spdlog::info("创建项目目录: {}", project_dir.string());

        // 读取并解包文件
        while (backup_file.peek() != EOF) {
            FileHeader header;
            backup_file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
            
            spdlog::info("解包文件: {}", header.path);

            if (auto handler = FileHandler::Create(header)) {
                handler->Unpack(backup_file, restore_metadata_);
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

bool Packer::Verify(const fs::path& backup_path) {
    try {
        std::ifstream backup_file(backup_path, std::ios::binary);
        if (!backup_file) {
            throw std::runtime_error("无法打开备份文件: " + backup_path.string());
        }

        // 读取备份信息
        BackupHeader stored_header;
        backup_file.read(reinterpret_cast<char*>(&stored_header), sizeof(BackupHeader));

        // 保存原始校验和
        uint32_t stored_checksum = stored_header.checksum;

        // 计算校验和
        backup_file.seekg(sizeof(BackupHeader));  // 跳过备份信息头
        std::vector<char> buffer(4096);
        uint32_t calculated_checksum = 0xFFFFFFFF;

        while (backup_file) {
            backup_file.read(buffer.data(), buffer.size());
            std::streamsize count = backup_file.gcount();
            if (count > 0) {
                calculated_checksum = calculateCRC32(buffer.data(), count, calculated_checksum);
            }
        }

        if (calculated_checksum != stored_checksum) {
            spdlog::error("备份文件校验失败！");
            spdlog::error("存储的校验和: {:#x}", stored_checksum);
            spdlog::error("计算的校验和: {:#x}", calculated_checksum);
            return false;
        }

        spdlog::info("备份文件验证成功");
        spdlog::info("备份时间: {}", std::ctime(&stored_header.timestamp));
        spdlog::info("备份描述: {}", stored_header.comment);
        return true;

    } catch (const std::exception &e) {
        spdlog::error("验证过程出错: {}", e.what());
        return false;
    }
}
