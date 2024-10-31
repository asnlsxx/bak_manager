#ifndef FILEBASE_H
#define FILEBASE_H

#include <fstream>
#include <string>
#include <filesystem>
#include <sys/stat.h>

namespace fs = std::filesystem;

constexpr std::size_t MAX_PATH_LEN = 100;

struct FileHeader
{
    char name[MAX_PATH_LEN];
    struct stat metadata;
};

#define FILE_HEADER_SIZE (sizeof(FileHeader))

class FileBase : public std::fstream {
public:
    FileBase(const fs::path &filepath_);
    bool OpenFile(std::ios_base::openmode mode_ = std::ios_base::in | std::ios_base::binary);
    virtual ~FileBase();

    bool IsHardLink() const;
    const FileHeader& getFileHeader() const;
    void WriteHeader(std::ofstream& backup_file) const;

private:
    FileHeader fileheader;
};

#endif // FILEBASE_H
