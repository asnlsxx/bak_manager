#include "Packer.h"
#include "spdlog/spdlog.h"
#include <iostream>

int main(int argc, char *argv[]) {
    try {
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        spdlog::set_level(spdlog::level::debug);
        
        if (argc != 3) {
            spdlog::error("用法: {} <源路径> <目标路径>", argv[0]);
            return 1;
        }

        std::string sourcePath = argv[1];
        std::string destinationPath = argv[2];

        // Packer packer(sourcePath, destinationPath);
        // if (packer.Pack()) {
        //     spdlog::info("备份完成");
        // } else {
        //     spdlog::error("备份失败");
        //     return 1;
        // }
        Packer packer(sourcePath, destinationPath);
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
