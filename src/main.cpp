#include "Packer.h"
#include "ArgParser.h"
#include "GUI.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <iostream>

void initialize_logger(bool verbose) {
  try {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("backup.log", true);

    // 设置控制台输出的日志级别
    console_sink->set_level(verbose ? spdlog::level::info : spdlog::level::err);
    
    // 文件日志保持详细记录
    file_sink->set_level(spdlog::level::debug);

    auto logger = std::make_shared<spdlog::logger>(
        "backup_logger", spdlog::sinks_init_list{console_sink, file_sink});

    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
    
    // 设置全局日志级别为最低级别，让单个sink控制显示
    logger->set_level(spdlog::level::trace);

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

  try {
    initialize_logger(parser.exist("verbose"));
    
    // 如果指定了 GUI 模式
    if (parser.exist("gui")) {
      GUI gui;
      gui.run();
      return 0;
    }

    // 如果指定了帮助选项或没有指定任何操作，显示帮助信息
    if (parser.exist("help") ) {
      std::cout << parser.usage();
      return 0;
    }
    ParserConfig::check_conflicts(parser);

    Packer packer;
    fs::path input_path,output_path; 
    if (parser.exist("input")) 
      input_path = fs::absolute(parser.get<std::string>("input"));
    if (parser.exist("output")) 
      output_path = fs::absolute(parser.get<std::string>("output"));
    if (parser.exist("backup")) {
      // 设置过滤器
      packer.set_filter(ParserConfig::create_filter(parser));
      
      // 设置是否压缩
      packer.set_compress(parser.exist("compress"));
      
      // 设置是否加密
      packer.set_encrypt(parser.exist("encrypt"), parser.get<std::string>("password"));
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
      packer.set_encrypt(parser.exist("password"), parser.get<std::string>("password"));
      if (!packer.Unpack(input_path, output_path)) {
        spdlog::error("恢复失败");
        return 1;
      }
      spdlog::info("恢复完成");
    } 
    else if (parser.exist("verify")) {
      if (!packer.Verify(input_path)) {
        spdlog::error("验证失败");
        return 1;
      }
      spdlog::info("验证完成");
    }
    else {
      spdlog::error("请选择操作：备份、恢复、验证");
      return 1;
    }
  } catch (const std::exception &e) {
    spdlog::critical("发生错误: {}", e.what());
    return 1;
  }

  return 0;
}
