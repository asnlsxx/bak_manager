#include "Packer.h"
#include <filesystem>
#include <iostream>


namespace fs = std::filesystem;

Packer::Packer(std::string root_path_, std::string pack_path_) {
  // 构造函数实现
}

Packer::~Packer() {
  // 析构函数实现
}

void Packer::DfsFile(FileBase &bak_file, fs::path cur_path) {}

bool Packer::Pack() {return false;}
bool Packer::Unpack() {return false;}
