#include "Packer.h"
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "用法: " << argv[0] << " <源路径> <目标路径>" << std::endl;
        return 1;
    }

    try {
        std::string sourcePath = argv[1];
        std::string destinationPath = argv[2];

        Packer packer(sourcePath, destinationPath);
        if (packer.Pack()) {
            std::cout << "备份完成" << std::endl;
        } else {
            std::cerr << "备份失败" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
