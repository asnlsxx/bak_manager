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
  std::snprintf(fileheader.name, MAX_PATH_LEN, "%s", filepath_.c_str());
  fileheader.metadata = file_stat;
}

FileBase::~FileBase() {}

bool FileBase::IsHardLink() const { 
  return fileheader.metadata.st_nlink > 1; 
}

bool FileBase::OpenFile(std::ios_base::openmode mode_) {
  open(fileheader.name, mode_);
  return is_open();
}

const FileHeader& FileBase::getFileHeader() const { 
  return fileheader;
}

void FileBase::WriteHeader(std::ofstream& backup_file) const {
  if (!backup_file) {
    throw std::runtime_error("备份文件未打开或无效");
  }
  
  backup_file.write(reinterpret_cast<const char*>(&fileheader), sizeof(FileHeader));
  
  if (backup_file.fail()) {
    throw std::runtime_error("写入文件头失败");
  }
}