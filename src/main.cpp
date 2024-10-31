#include "Packer.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <iostream>

void initialize_logger() {
    try {
        // 创建控制台和文件输出
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("backup.log", true);

        // 创建多输出日志记录器
        auto logger = std::make_shared<spdlog::logger>("backup_logger", 
            spdlog::sinks_init_list{console_sink, file_sink});

        // 设置日志格式和级别
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
        logger->set_level(spdlog::level::debug);

        // 注册为默认日志记录器
        spdlog::set_default_logger(logger);

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "日志初始化失败: " << ex.what() << std::endl;
        throw;
    }
}

int main(int argc, char *argv[]) {
    try {
        // 初始化全局日志系统
        initialize_logger();
        
        if (argc != 3) {
            spdlog::error("用法: {} <源路径> <目标路径>", argv[0]);
            return 1;
        }

        std::string sourcePath = argv[1];
        std::string targetPath = argv[2];

        Packer packer(sourcePath, targetPath);
        if (packer.Unpack()) {
            spdlog::info("备份完成");
        } else {
            spdlog::error("备份失败");
            return 1;
        }
    } catch (const std::exception& e) {
        spdlog::critical("发生错误: {}", e.what());
        return 1;
    }

    return 0;
}
