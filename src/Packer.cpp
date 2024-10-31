#include "Packer.h"
#include "FileBase.h"
#include "FileHandler.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <fstream>

namespace fs = std::filesystem;

Packer::Packer(std::string root_path_, std::string pack_path_)
    : root_path(fs::absolute(root_path_)), bak_path(fs::absolute(pack_path_)) {
  InitializeLogger();
  logger->info("开始初始化备份工具...");

  std::string folder_name = root_path.filename().string();
  bak_path = bak_path / (folder_name + ".backup");

  InitializeHandlers();
  logger->info("备份工具初始化完成");
}

void Packer::InitializeHandlers() {
  handlers[fs::file_type::regular] =
      std::make_unique<RegularFileHandler>(inode_table);
  handlers[fs::file_type::directory] =
      std::make_unique<DirectoryHandler>(inode_table);
  handlers[fs::file_type::symlink] =
      std::make_unique<SymlinkHandler>(inode_table);
  // 可以在这里添加更多的文件类型处理器
}

void Packer::InitializeLogger() {
  try {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("backup.log", true);

    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    logger = std::make_shared<spdlog::logger>("backup_logger", sinks.begin(),
                                              sinks.end());

    logger->set_level(spdlog::level::debug);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");

    spdlog::register_logger(logger);
  } catch (const spdlog::spdlog_ex &ex) {
    std::cerr << "日志初始化失败: " << ex.what() << std::endl;
    throw;
  }
}

bool Packer::Pack() {
  logger->info("开始打包: {} -> {}", root_path.string(), bak_path.string());
  try {
    std::ofstream backup_file(bak_path, std::ios::binary);
    if (!backup_file) {
      throw std::runtime_error("无法创建备份文件: " + bak_path.string());
    }

    std::filesystem::current_path(root_path.parent_path());
    logger->debug("切换工作目录到: {}", root_path.parent_path().string());

    for (const auto &entry : fs::recursive_directory_iterator(root_path)) {
      const auto &path = entry.path();
      logger->debug("打包文件: {}", path.string());

      fs::file_type type = fs::status(path).type();

      if (auto it = handlers.find(type); it != handlers.end()) {
        it->second->Pack(path, backup_file);
      } else {
        logger->warn("跳过未知文件类型: {}", path.string());
      }
    }

    return true;
  } catch (const std::exception &e) {
    logger->error("打包过程出错: {}", e.what());
    return false;
  }
}

fs::file_type st_mode_to_file_type(mode_t mode) {
    if (S_ISREG(mode)) {
        return fs::file_type::regular;
    } else if (S_ISDIR(mode)) {
        return fs::file_type::directory;
    } else if (S_ISLNK(mode)) {
        return fs::file_type::symlink;
    } else if (S_ISCHR(mode)) {
        return fs::file_type::character;
    } else if (S_ISBLK(mode)) {
        return fs::file_type::block;
    } else if (S_ISFIFO(mode)) {
        return fs::file_type::fifo;
    } else if (S_ISSOCK(mode)) {
        return fs::file_type::socket;
    } else {
        return fs::file_type::unknown;
    }
}

bool Packer::Unpack() {
  logger->info("开始解包: {} -> {}", bak_path.string(), root_path.string());
  try {
    std::ifstream backup_file(bak_path, std::ios::binary);
    if (!backup_file) {
      throw std::runtime_error("无法打开备份文件: " + bak_path.string());
    }

    // 创建目标目录
    fs::create_directories(root_path);
    std::filesystem::current_path(root_path);

    // 读取并解包文件
    while (backup_file.peek() != EOF) {
      FileHeader header;
      backup_file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
      
      if (backup_file.eof()) break;

      logger->debug("解包文件: {}", header.name);
      fs::file_type type = st_mode_to_file_type(header.metadata.st_mode);

      if (auto it = handlers.find(type); it != handlers.end()) {
        it->second->Unpack(header, backup_file);
      } else {
        logger->warn("跳过未知文件类型: {}", header.name);
      }
    }

    logger->info("解包完成");
    return true;
  } catch (const std::exception &e) {
    logger->error("解包过程出错: {}", e.what());
    return false;
  }
}
