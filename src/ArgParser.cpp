#include "ArgParser.h"

#include <regex>
#include <stdexcept>
#include <sys/stat.h>
#include <spdlog/spdlog.h>
#include <ctime>

void ParserConfig::configure_parser(cmdline::parser& parser) {
  // 基本选项
  parser.add<std::string>("input", 'i', "程序输入文件路径", false);
  parser.add<std::string>("output", 'o', "程序输出文件路径", false);
  parser.add("backup", 'b', "备份");
  parser.add("restore", 'r', "恢复");
  parser.add("verbose", 'v', "输出执行过程信息");
  parser.add("help", 'h', "查看帮助文档");

  // 备份选项
  parser.add("compress", 'c', "备份时压缩文件");
  parser.add("encrypt", 'e', "备份时加密文件");
  parser.add<std::string>("password", 'p', "加密/解密密码", false, std::string(""));
  parser.add<std::string>("path", '\0', "过滤路径：正则表达式", false);
  parser.add<std::string>("type", '\0',
                          "备份文件类型，可组合使用: n普通文件,l符号链接,p管道文件", false);
  parser.add<std::string>("name", '\0', "过滤文件名：正则表达式", false);
  parser.add<std::string>("atime", '\0', 
      "按访问时间过滤，格式: START,END 例如: 202401010000,202401312359", false);
  parser.add<std::string>("mtime", '\0', 
      "按修改时间过滤，格式: START,END 例如: 202312010000,202312312359", false);
  parser.add<std::string>("ctime", '\0', 
      "按状态改变时间过滤，格式: START,END 例如: 202401010000,202401012359", false);
  parser.add<std::string>("message", 'm', "添加备注信息", false);
  // 恢复选项
  parser.add("metadata", 'a', "恢复文件的元数据");

  // 验证选项
  parser.add("verify", 'l', "验证备份数据");

  // 添加文件大小过滤选项
  parser.add<std::string>("size", '\0', 
      "按文件大小过滤，格式: [<>]N[bkmg]，例如: >1k表示大于1KB, <1m表示小于1MB", false);

  
}

void ParserConfig::check_conflicts(const cmdline::parser& parser) {
    // 检查基本操作选项
    if (parser.exist("backup") && parser.exist("restore")) {
        throw std::runtime_error("Cannot specify both backup (-b) and restore (-r) options");
    }

    // 检查必需的输入/输出路径
    if (parser.exist("backup") || parser.exist("restore")) {
        if (!parser.exist("input")) {
            throw std::runtime_error("Input path (-i) is required");
        }
        if (!parser.exist("output")) {
            throw std::runtime_error("Output path (-o) is required");
        }

        // 检查路径是否存在
        fs::path input_path = fs::absolute(parser.get<std::string>("input"));
        fs::path output_path = fs::absolute(parser.get<std::string>("output"));

        if (parser.exist("backup")) {
            // 备份模式：输入路径必须存在
            if (!fs::exists(input_path)) {
                throw std::runtime_error("Input path does not exist: " + input_path.string());
            }
            
            // 检查输出目录是否存在，如果不存在则尝试创建
            if (!fs::exists(output_path)) {
                if (!fs::create_directories(output_path)) {
                    throw std::runtime_error("Failed to create output directory: " + output_path.string());
                }
            } else if (!fs::is_directory(output_path)) {
                throw std::runtime_error("Output path is not a directory: " + output_path.string());
            }
        } else if (parser.exist("restore")) {
            // 还原模式：输入文件必须存在且是文件
            if (!fs::exists(input_path)) {
                throw std::runtime_error("Backup file does not exist: " + input_path.string());
            }
            if (!fs::is_regular_file(input_path)) {
                throw std::runtime_error("Input path is not a valid backup file: " + input_path.string());
            }
            
            // 检查输出目录
            if (!fs::exists(output_path)) {
                if (!fs::create_directories(output_path)) {
                    throw std::runtime_error("Failed to create restore directory: " + output_path.string());
                }
            } else if (!fs::is_directory(output_path)) {
                throw std::runtime_error("Restore path is not a directory: " + output_path.string());
            }
        }
    }

    // 检查验证模式的参数
    if (parser.exist("verify")) {
        if (!parser.exist("input")) {
            throw std::runtime_error("Input path (-i) is required for verify operation");
        }
        fs::path input_path = fs::absolute(parser.get<std::string>("input"));
        if (!fs::exists(input_path)) {
            throw std::runtime_error("Backup file does not exist: " + input_path.string());
        }
        if (!fs::is_regular_file(input_path)) {
            throw std::runtime_error("Input path is not a valid backup file: " + input_path.string());
        }
    }

    // 检查过滤选项是否在正确的模式下使用
    if (parser.exist("restore") &&
        (parser.exist("type") || parser.exist("path") || parser.exist("name") ||
         parser.exist("atime") || parser.exist("mtime") || parser.exist("ctime") ||
         parser.exist("size"))) {
        throw std::runtime_error("Filter options can only be used in backup mode");
    }

    // 加密相关的检查
    if (parser.exist("encrypt")) {
        if (!parser.exist("backup")) {
            throw std::runtime_error("Encryption can only be used in backup mode");
        }
        if (parser.get<std::string>("password").empty()) {
            throw std::runtime_error("Password is required when encryption is enabled");
        }
    }

    // 密码相关的检查
    if (parser.exist("password")) {
        if (!parser.exist("encrypt") && !parser.exist("restore")) {
            throw std::runtime_error("Password can only be used with encryption or restore");
        }
    }

    // 元数据相关的检查
    if (parser.exist("metadata") && !parser.exist("restore")) {
        throw std::runtime_error("Metadata option can only be used in restore mode");
    }

    // 时间范围格式检查
    auto check_time_format = [](const std::string& time_str) {
        if (!time_str.empty() && !std::regex_match(time_str, 
            std::regex("\\d{12},\\d{12}"))) {
            throw std::runtime_error("Invalid time format. Expected: YYYYMMDDHHMM,YYYYMMDDHHMM");
        }
    };

    if (parser.exist("atime")) check_time_format(parser.get<std::string>("atime"));
    if (parser.exist("mtime")) check_time_format(parser.get<std::string>("mtime"));
    if (parser.exist("ctime")) check_time_format(parser.get<std::string>("ctime"));

    // 文件大小格式检查
    if (parser.exist("size")) {
        const std::string& size_str = parser.get<std::string>("size");
        if (!std::regex_match(size_str, 
            std::regex("[<>]\\d+[bkmg]"))) {
            throw std::runtime_error("Invalid size format. Expected: [<>]N[bkmg]");
        }
    }
}

namespace {
    // 解析时间字符串为time_t
    time_t parse_time(const std::string& time_str) {
        if (time_str.length() != 12) {  // YYYYMMDDHHMM
            throw std::runtime_error("时间格式错误，应为12位数字: " + time_str);
        }

        struct tm tm = {};
        try {
            tm.tm_year = std::stoi(time_str.substr(0, 4)) - 1900;
            tm.tm_mon  = std::stoi(time_str.substr(4, 2)) - 1;
            tm.tm_mday = std::stoi(time_str.substr(6, 2));
            tm.tm_hour = std::stoi(time_str.substr(8, 2));
            tm.tm_min  = std::stoi(time_str.substr(10, 2));
            tm.tm_sec  = 0;
        } catch (const std::exception& e) {
            throw std::runtime_error("时间格式解析错误: " + time_str);
        }

        time_t time = mktime(&tm);
        if (time == -1) {
            throw std::runtime_error("无效的时间: " + time_str);
        }
        return time;
    }

    // 解析时间区间
    std::pair<time_t, time_t> parse_time_range(const std::string& range_str) {
        auto pos = range_str.find(',');
        if (pos == std::string::npos) {
            throw std::runtime_error("时间区间格式错误，应为START,END: " + range_str);
        }

        std::string start_str = range_str.substr(0, pos);
        std::string end_str = range_str.substr(pos + 1);

        time_t start_time = parse_time(start_str);
        time_t end_time = parse_time(end_str);

        if (end_time < start_time) {
            throw std::runtime_error("结束时间不能早于开始时间");
        }

        return {start_time, end_time};
    }

    // 解析文件大小
    int64_t parse_size(const std::string& size_str) {
        if (size_str.empty() || size_str.length() < 2) {
            throw std::runtime_error("大小格式错误: " + size_str);
        }

        char compare = size_str[0];
        bool is_greater = false;
        size_t start_pos = 0;

        if (compare == '>' || compare == '<') {
            is_greater = (compare == '>');
            start_pos = 1;
        } else {
            throw std::runtime_error("大小必须以>或<开头: " + size_str);
        }

        // 获取数值和单位
        size_t unit_pos = size_str.find_first_not_of("0123456789", start_pos);
        if (unit_pos == std::string::npos) {
            throw std::runtime_error("缺少大小单位: " + size_str);
        }

        int64_t value = std::stoll(size_str.substr(start_pos, unit_pos - start_pos));
        char unit = std::tolower(size_str[unit_pos]);

        // 转换为字节
        switch (unit) {
            case 'b': break;                    // 字节
            case 'k': value *= 1024; break;     // KB
            case 'm': value *= 1024*1024; break;// MB
            case 'g': value *= 1024*1024*1024; break; // GB
            default:
                throw std::runtime_error("无效的大小单位(b/k/m/g): " + size_str);
        }

        return is_greater ? value : -value;  // 负值表示小于
    }
}

FileFilter ParserConfig::create_filter(const cmdline::parser& parser) {
  return [&parser](const fs::path& path) {
    // 路径过滤
    if (parser.exist("path")) {
      std::regex path_pattern(parser.get<std::string>("path"));
      if (!std::regex_match(path.string(), path_pattern)) {
        return false;
      }
    }

    // 文件名过滤
    if (parser.exist("name")) {
      std::regex name_pattern(parser.get<std::string>("name"));
      if (!std::regex_match(path.filename().string(), name_pattern)) {
        return false;
      }
    }
    // 如果是文件夹，则不进行过滤
    if (fs::is_directory(path)) {
      return true;
    }

    // 文件类型过滤
    if (parser.exist("type")) {
      fs::file_status status = fs::symlink_status(path);
      std::string type = parser.get<std::string>("type");
      char file_type;
      switch (status.type()) {
        case fs::file_type::regular:
          file_type = 'n';
          break;
        case fs::file_type::symlink:
          file_type = 'l';
          break;
        case fs::file_type::fifo:
          file_type = 'p';
          break;
        default:
          file_type = 'x';
          break;
      }
      if (type.find(file_type) == std::string::npos) {
        return false;
      }
    }

    // 获取文件状态
    struct stat file_stat;
    if (lstat(path.c_str(), &file_stat) != 0) {
        spdlog::warn("无法获取文件时间信息: {}", path.string());
        return false;
    }

    // 访问时间过滤
    if (parser.exist("atime")) {
        auto [start, end] = parse_time_range(parser.get<std::string>("atime"));
        if (file_stat.st_atime < start || file_stat.st_atime > end) {
            return false;
        }
    }

    // 修改时间过滤
    if (parser.exist("mtime")) {
        auto [start, end] = parse_time_range(parser.get<std::string>("mtime"));
        if (file_stat.st_mtime < start || file_stat.st_mtime > end) {
            return false;
        }
    }

    // 状态改变时间过滤
    if (parser.exist("ctime")) {
        auto [start, end] = parse_time_range(parser.get<std::string>("ctime"));
        if (file_stat.st_ctime < start || file_stat.st_ctime > end) {
            return false;
        }
    }

    // 文件大小过滤
    if (parser.exist("size")) {
        // 如果是目录，跳过大小检查
        if (fs::is_directory(path)) {
            return true;
        }

        int64_t threshold = parse_size(parser.get<std::string>("size"));
        uintmax_t file_size = fs::file_size(path);

        if (threshold > 0) {  // 大于
            if (file_size <= threshold) {
                return false;
            }
        } else {  // 小于
            if (file_size >= -threshold) {
                return false;
            }
        }
    }

    return true;
  };
}