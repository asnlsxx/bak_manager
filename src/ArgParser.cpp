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
  parser.add<std::string>("password", 'p', "指定密码", false);
  parser.add("help", 'h', "查看帮助文档");

  // 备份选项
  parser.add("compress", 'c', "备份时压缩文件");
  parser.add("encrypt", 'e', "备份时加密文件");
  parser.add<std::string>("path", '\0', "过滤路径：正则表达式", false);
  parser.add<std::string>("type", '\0',
                          "备份文件类型: n普通文件,d目录文件,l符号链接,p管道文件", false);
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

}

void ParserConfig::check_conflicts(const cmdline::parser& parser) {
  // 检查冲突选项
  if (parser.exist("backup") && parser.exist("restore")) {
    throw std::runtime_error("不能同时指定备份(-b)和恢复(-r)选项");
  }

  // 检查过滤选项是否在正确的模式下使用
  if (parser.exist("restore") &&
      (parser.exist("type") || parser.exist("path") || parser.exist("name") ||
       parser.exist("atime") || parser.exist("mtime") ||
       parser.exist("ctime"))) {
    throw std::runtime_error("过滤选项只能在备份模式下使用");
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
        case fs::file_type::directory:
          file_type = 'd';
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

    return true;
  };
}