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
  FileHandler() {}
  virtual void Pack(const fs::path &path, std::ofstream &backup_file, std::unordered_map<ino_t, std::string>& inode_table) = 0;
  virtual void Unpack(const FileHeader &header, std::ifstream &backup_file) = 0;
  virtual ~FileHandler() = default;


};

class RegularFileHandler : public FileHandler {
public:
  RegularFileHandler() {}
  void Pack(const fs::path &path, std::ofstream &backup_file, std::unordered_map<ino_t, std::string>& inode_table) override;
  void Unpack(const FileHeader &header, std::ifstream &backup_file) override;
};

class DirectoryHandler : public FileHandler {
public:
  DirectoryHandler() {}
  void Pack(const fs::path &path, std::ofstream &backup_file, std::unordered_map<ino_t, std::string>& inode_table) override;
  void Unpack(const FileHeader &header, std::ifstream &backup_file) override;
};

class SymlinkHandler : public FileHandler {
public:
  SymlinkHandler() {}
  void Pack(const fs::path &path, std::ofstream &backup_file, std::unordered_map<ino_t, std::string>& inode_table) override;
  void Unpack(const FileHeader &header, std::ifstream &backup_file) override;
};

#endif // FILE_HANDLER_H
