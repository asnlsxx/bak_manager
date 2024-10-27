#include "FileBase.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>

FileBase::FileBase(const fs::path &filepath_) : std::fstream() {
  // 获取元数据
  struct stat file_stat;
  if (stat(filepath_.c_str(), &file_stat) != 0) {
    throw std::runtime_error("无法获取文件信息: " + filepath_.string());
  }

  // 填充文件头
  std::snprintf(m_fileheader.name, MAX_PATH_LEN, "%s", filepath_.c_str());

  m_fileheader.metadata = file_stat;
}

FileBase::~FileBase() {}

bool FileBase::OpenFile(std::ios_base::openmode mode_) {
  open(m_fileheader.name, mode_);
  return is_open();
}
