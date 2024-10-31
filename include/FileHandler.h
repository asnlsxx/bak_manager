#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include "FileBase.h"

namespace fs = std::filesystem;

class FileHandler {
public:
  FileHandler(std::unordered_map<ino_t, std::string> &inode_table)
      : inode_table_(inode_table) {}
  virtual void Pack(const fs::path &path, std::ofstream &backup_file) = 0;
  virtual void Unpack(const FileHeader &header, std::ifstream &backup_file) = 0;
  virtual ~FileHandler() = default;

protected:
  std::unordered_map<ino_t, std::string> &inode_table_;
};

class RegularFileHandler : public FileHandler {
public:
  RegularFileHandler(std::unordered_map<ino_t, std::string> &inode_table)
      : FileHandler(inode_table) {}
  void Pack(const fs::path &path, std::ofstream &backup_file) override;
  void Unpack(const FileHeader &header, std::ifstream &backup_file) override;
};

class DirectoryHandler : public FileHandler {
public:
  DirectoryHandler(std::unordered_map<ino_t, std::string> &inode_table)
      : FileHandler(inode_table) {}
  void Pack(const fs::path &path, std::ofstream &backup_file) override;
  void Unpack(const FileHeader &header, std::ifstream &backup_file) override;
};

class SymlinkHandler : public FileHandler {
public:
  SymlinkHandler(std::unordered_map<ino_t, std::string> &inode_table)
      : FileHandler(inode_table) {}
  void Pack(const fs::path &path, std::ofstream &backup_file) override;
  void Unpack(const FileHeader &header, std::ifstream &backup_file) override;
};

#endif // FILE_HANDLER_H
