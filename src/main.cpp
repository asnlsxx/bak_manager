#include "Packer.h"
#include "ArgParser.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <iostream>

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
  ParserConfig::configure_parser(parser);
  parser.parse_check(argc, argv);

  // 如果指定了帮助选项或没有指定任何操作，显示帮助信息
  if (parser.exist("help") ) {
    std::cout << parser.usage();
    return 0;
  }
  if (parser.exist("verify") ) {
    Packer packer;
    fs::path input_path = fs::absolute(parser.get<std::string>("input"));
    if (!packer.Verify(input_path)) {
      spdlog::error("验证失败");
      return 1;
    }
    return 0;
  }
  if (!(parser.exist("backup") || parser.exist("restore"))) {
    std::cout << "请选择还原或备份选项" << std::endl;
    return 0;
  }
  try {
    ParserConfig::check_conflicts(parser);
    initialize_logger(parser.exist("verbose"));

    Packer packer;
    fs::path input_path = fs::absolute(parser.get<std::string>("input"));
    fs::path output_path = fs::absolute(parser.get<std::string>("output"));

    if (parser.exist("backup")) {
      // 设置过滤器
      packer.set_filter(ParserConfig::create_filter(parser));
      
      // 设置是否压缩
      packer.set_compress(parser.exist("compress"));
      
      // 设置是否加密
      if (parser.exist("encrypt")) {
          packer.set_encrypt(true, parser.get<std::string>("password"));
      }
      
      // 构造备份文件路径
      fs::path backup_path = output_path / (input_path.filename().string() + ".backup");
      
      if (!packer.Pack(input_path, backup_path)) {
        spdlog::error("备份失败");
        return 1;
      }
      spdlog::info("备份完成");
    } else if (parser.exist("restore")) {
      // 设置是否恢复元数据
      packer.set_restore_metadata(parser.exist("metadata"));
      
      // 如果提供了密码，设置解密
      if (parser.exist("password")) {
          packer.set_encrypt(true, parser.get<std::string>("password"));
      }
      
      if (!packer.Unpack(input_path, output_path)) {
        spdlog::error("恢复失败");
        return 1;
      }
      spdlog::info("恢复完成");
    } 
  } catch (const std::exception &e) {
    spdlog::critical("发生错误: {}", e.what());
    return 1;
  }

  return 0;
}
