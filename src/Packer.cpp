#include "Packer.h"
#include "Compression.h"
#include <fstream>
#include <array>
#include <filesystem>
#include <spdlog/spdlog.h>

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
    try {
        spdlog::info("开始打包: {} -> {}", source_path.string(), target_path.string());
        // 创建临时文件用于打包
        fs::path temp_path = target_path.parent_path() / (target_path.stem().string() + ".tmp");
        
        // 先执行正常的打包（不包含header和校验和）
        if (!PackToFile(source_path, temp_path)) {
            throw std::runtime_error("打包到临时文件失败");
        }

        // 读取临时文件数据
        std::ifstream temp_file(temp_path, std::ios::binary);
        if (!temp_file) {
            fs::remove(temp_path);
            throw std::runtime_error("无法打开临时文件");
        }

        std::vector<char> file_data((std::istreambuf_iterator<char>(temp_file)),
                                   std::istreambuf_iterator<char>());
        temp_file.close();
        fs::remove(temp_path);  // 立即删除临时文件

        std::vector<char> final_data;
        if (compress_) {
            // 压缩数据
            spdlog::info("压缩数据");
            final_data = LZWCompression::compress({file_data.data(), file_data.size()});
        } else {
            final_data = std::move(file_data);
        }

        if (encrypt_) {
            // 加密数据
            spdlog::info("加密数据");
            final_data = aes_->encrypt({final_data.data(), final_data.size()});
        }

        // 计算校验和
        uint32_t checksum = calculateCRC32(final_data.data(), final_data.size());

        // 更新header
        backup_header_.timestamp = std::time(nullptr);
        backup_header_.checksum = checksum;

        // 写入最终文件
        std::ofstream target_file(target_path, std::ios::binary);
        if (!target_file) {
            throw std::runtime_error("无法创建最终备份文件");
        }

        // 写入header
        target_file.write(reinterpret_cast<const char*>(&backup_header_), sizeof(BackupHeader));

        // 写入数据
        target_file.write(final_data.data(), final_data.size());
        target_file.close();

        return true;
    } catch (const std::exception& e) {
        spdlog::error("打包过程出错: {}", e.what());
        return false;
    }
}

bool Packer::PackToFile(const fs::path& source_path, const fs::path& target_path) {
    inode_table.clear();

    const fs::path normalized_source = source_path.lexically_normal();
    const fs::path normalized_target = target_path.lexically_normal();
    
    try {
        std::ofstream backup_file(normalized_target, std::ios::binary);
        if (!backup_file) {
            throw std::runtime_error("无法创建备份文件: " + normalized_target.string());
        }

        // 切换到源目录，使得相对路径正确
        std::filesystem::current_path(normalized_source);
        spdlog::info("切换工作目录到: {}", normalized_source.string());

        // 将所有数据写入备份文件
        for (const auto &entry : fs::recursive_directory_iterator(normalized_source)) {
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

        backup_file.close();
        return true;
    } catch (const std::exception &e) {
        spdlog::error("打包过程出错: {}", e.what());
        return false;
    }
}

bool Packer::Unpack(const fs::path& backup_path, const fs::path& restore_path) {
    try {
        spdlog::info("开始解包: {} -> {}", backup_path.string(), restore_path.string());

        // 读取备份文件
        std::ifstream backup_file(backup_path, std::ios::binary);
        if (!backup_file) {
            throw std::runtime_error("无法打开备份文件: " + backup_path.string());
        }
        if (!fs::exists(restore_path)) {
            fs::create_directories(restore_path);
        }
        // 读取header
        BackupHeader stored_header;
        backup_file.read(reinterpret_cast<char*>(&stored_header), sizeof(BackupHeader));

        // 读取剩余数据
        std::vector<char> final_data((std::istreambuf_iterator<char>(backup_file)),
                                   std::istreambuf_iterator<char>());
        backup_file.close();

        if (stored_header.mod & MOD_ENCRYPTED) {
            spdlog::info("解密数据");
            if (!aes_) {
                throw std::runtime_error("需要解密密钥");
            }
            // 先解密
            final_data = aes_->decrypt(final_data.data(), final_data.size());
        }

        if (stored_header.mod & MOD_COMPRESSED) {
            spdlog::info("解压数据");
            // 不再需要传入size参数
            final_data = LZWCompression::decompress(final_data.data());
        }

        // 创建临时文件
        fs::path temp_path = backup_path.parent_path() / (backup_path.stem().string() + ".tmp");
        // 写入临时文件
        std::ofstream temp_file(temp_path, std::ios::binary);
        if (!temp_file) {
            throw std::runtime_error("无法创建临时文件");
        }

        // 写入解压后的数据
        temp_file.write(final_data.data(), final_data.size());
        temp_file.close();

        // 解包
        bool result = UnpackFromFile(temp_path, restore_path);
        fs::remove(temp_path);
        return result;

    } catch (const std::exception& e) {
        spdlog::error("解包过程出错: {}", e.what());
        return false;
    }
}

// 将原来的Unpack函数重命名为UnpackFromFile
bool Packer::UnpackFromFile(const fs::path& backup_path, const fs::path& restore_path) {
    try {
        std::ifstream backup_file(backup_path, std::ios::binary);
        if (!backup_file) {
            throw std::runtime_error("无法打开备份文件: " + backup_path.string());
        }
        
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

        // 计算校验和（对压缩后的数据计算）
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
        // spdlog::info("备份描述: {}", stored_header.comment);
        if (stored_header.mod & MOD_COMPRESSED) {
            spdlog::info("文件已压缩");
        }
        if (stored_header.mod & MOD_ENCRYPTED) {
            spdlog::info("文件已加密");
        }
        return true;

    } catch (const std::exception &e) {
        spdlog::error("验证过程出错: {}", e.what());
        return false;
    }
}
