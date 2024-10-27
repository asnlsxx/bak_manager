#include "Packer.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "用法: " << argv[0] << " <源路径> <目标路径>" << std::endl;
        return 1;
    }

    std::string sourcePath = argv[1];
    std::string destinationPath = argv[2];


    std::cout << "备份完成" << std::endl;
    return 0;
}
