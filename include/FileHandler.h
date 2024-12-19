#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unordered_map>


namespace fs = std::filesystem;

constexpr std::size_t MAX_PATH_LEN = 100;

struct FileHeader {
  char path[MAX_PATH_LEN];
  struct stat metadata;
};

class FileHandler : public std::fstream {
public:
  // 默认构造函数
  FileHandler() = default;
  FileHandler(const FileHeader &header) : fileheader(header) {}

  // 根据路径构造
  explicit FileHandler(const fs::path &filepath);

  // 根据FileHeader构造

  static std::unique_ptr<FileHandler> Create(const fs::path& path);
  static std::unique_ptr<FileHandler> Create(const FileHeader &header);

  // TODO 是否可以将 table 放在 Pack 的全局空间中？
  virtual void Pack(std::ofstream &backup_file,
                    std::unordered_map<ino_t, std::string> &inode_table) = 0;
  virtual void Unpack(std::ifstream &backup_file) = 0;
  virtual ~FileHandler() = default;

private:
  FileHeader fileheader;

protected:
  bool IsHardLink() const;
  bool OpenFile(std::ios_base::openmode mode_ = std::ios_base::in |
                                                std::ios_base::binary);
  const FileHeader &getFileHeader() const;
  void WriteHeader(std::ofstream &backup_file) const;

  // 添加一个辅助函数来处理长路径
  void WriteLongPath(std::ofstream &backup_file, const std::string &path) const;
  // 读取可能超过MAX_PATH_LEN的路径
  std::string ReadLongPath(std::ifstream &backup_file) const;
};

class RegularFileHandler : public FileHandler {
public:
  RegularFileHandler() : FileHandler() {}
  RegularFileHandler(const fs::path &path) : FileHandler(path) {}
  RegularFileHandler(const FileHeader &header) : FileHandler(header) {}

  void Pack(std::ofstream &backup_file,
            std::unordered_map<ino_t, std::string> &inode_table) override;
  void Unpack(std::ifstream &backup_file) override;
};

class DirectoryHandler : public FileHandler {
public:
  DirectoryHandler() : FileHandler() {}
  DirectoryHandler(const fs::path &path) : FileHandler(path) {}
  DirectoryHandler(const FileHeader &header) : FileHandler(header) {}

  void Pack(std::ofstream &backup_file,
            std::unordered_map<ino_t, std::string> &inode_table) override;
  void Unpack(std::ifstream &backup_file) override;
};

class SymlinkHandler : public FileHandler {
public:
  SymlinkHandler() : FileHandler() {}
  SymlinkHandler(const fs::path &path) : FileHandler(path) {}
  SymlinkHandler(const FileHeader &header) : FileHandler(header) {}

  void Pack(std::ofstream &backup_file,
            std::unordered_map<ino_t, std::string> &inode_table) override;
  void Unpack(std::ifstream &backup_file) override;
};

#endif // FILE_HANDLER_H
