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

    Packer packer(parser);

    if (parser.exist("backup")) {
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
      if (!packer.List()) {
        return 1;
      }
    }

  } catch (const std::exception &e) {
    spdlog::critical("发生错误: {}", e.what());
    return 1;
  }

  return 0;
}
