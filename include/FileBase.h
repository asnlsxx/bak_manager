#ifndef FILEBASE_H
#define FILEBASE_H

#include <fstream>
#include <string>
#include <filesystem>
#include <sys/stat.h>

namespace fs = std::filesystem;

class FileBase : public std::fstream {
public:
    // 构造函数
    FileBase();
    FileBase(const fs::path& path, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);

    // 析构函数
    virtual ~FileBase();

private:
    fs::path m_path;
    struct stat m_metadata;
};

#endif // FILEBASE_H