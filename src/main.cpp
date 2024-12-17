#include "Packer.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "cmdline.h"
#include <iostream>
#include <regex>

void initialize_logger(bool verbose) {
  try {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("backup.log", true);

    auto logger = std::make_shared<spdlog::logger>(
        "backup_logger", spdlog::sinks_init_list{console_sink, file_sink});

    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
    logger->set_level(verbose ? spdlog::level::debug : spdlog::level::info);

    spdlog::set_default_logger(logger);
  } catch (const spdlog::spdlog_ex &ex) {
    std::cerr << "日志初始化失败: " << ex.what() << std::endl;
    throw;
  }
}

int main(int argc, char *argv[]) {
  cmdline::parser parser;
  
  // 基本选项
  parser.add<std::string>("input", 'i', "程序输入文件路径", true);
  parser.add<std::string>("output", 'o', "程序输出文件路径", true);
  parser.add("backup", 'b', "备份");
  parser.add("restore", 'r', "恢复");
  parser.add<std::string>("list", 'l', "查看指定备份文件的信息", false);
  parser.add("verbose", 'v', "输出执行过程信息");
  parser.add<std::string>("password", 'p', "指定密码", false);
  parser.add("help", 'h', "查看帮助文档");

  // 备份选项
  parser.add("compress", 'c', "备份时压缩文件");
  parser.add("encrypt", 'e', "备份时加密文件");
  parser.add<std::string>("path", '\0', "过滤路径：正则表达式", false);
  parser.add<std::string>("type", '\0', "过滤文件类型: n普通文件,d目录文件,l链接文件,p管道文件", false);
  parser.add<std::string>("name", '\0', "过滤文件名：正则表达式", false);
  parser.add<std::string>("atime", '\0', "文件的访问时间区间", false);
  parser.add<std::string>("mtime", '\0', "文件的修改时间区间", false);
  parser.add<std::string>("ctime", '\0', "文件的改变时间区间", false);
  parser.add<std::string>("message", 'm', "添加备注信息", false);

  // 恢复选项
  parser.add("metadata", 'a', "恢复文件的元数据");

  parser.parse_check(argc, argv);

  // 如果指定了帮助选项或没有指定任何操作，显示帮助信息
  if (parser.exist("help") || 
      !(parser.exist("backup") || parser.exist("restore") || parser.exist("list"))) {
    std::cout << parser.usage();
    return 0;
  }

  try {
    initialize_logger(parser.exist("verbose"));

    std::string input_path = parser.get<std::string>("input");
    std::string output_path = parser.get<std::string>("output");

    Packer packer(input_path, output_path);

    // 如果指定了过滤选项，设置过滤器
    // TODO 冲突选项检测
    if (parser.exist("backup")) {
      // 构建文件过滤器
      packer.set_filter([&](const fs::path& path) {
        // 路径过滤
        if (parser.exist("path")) {
          std::regex path_pattern(parser.get<std::string>("path"));
          if (!std::regex_match(path.string(), path_pattern)) {
            return false;
          }
        }

        // 文件名过滤
        if (parser.exist("name")) {
          std::regex name_pattern(parser.get<std::string>("name"));
          if (!std::regex_match(path.filename().string(), name_pattern)) {
            return false;
          }
        }

        // 文件类型过滤
        if (parser.exist("type")) {
          std::string type = parser.get<std::string>("type");
          fs::file_status status = fs::status(path);
          char file_type;
          switch (status.type()) {
            case fs::file_type::regular: file_type = 'n'; break;
            case fs::file_type::directory: file_type = 'd'; break;
            case fs::file_type::symlink: file_type = 'l'; break;
            case fs::file_type::fifo: file_type = 'p'; break;
            default: file_type = 'x'; break;
          }
          if (type.find(file_type) == std::string::npos) {
            return false;
          }
        }

        // TODO: 添加时间过滤功能

        return true;
      });

      if (!packer.Pack()) {
        spdlog::error("备份失败");
        return 1;
      }
      spdlog::info("备份完成");
    } else if (parser.exist("restore")) {
      if (!packer.Unpack()) {
        spdlog::error("恢复失败");
        return 1;
      }
      spdlog::info("恢复完成");
    } else if (parser.exist("list")) {
      // TODO: 实现查看备份文件信息的功能
      spdlog::error("暂不支持查看备份文件信息功能");
      return 1;
    }

  } catch (const std::exception &e) {
    spdlog::critical("发生错误: {}", e.what());
    return 1;
  }

  return 0;
}
