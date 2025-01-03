#include "ArgParser.h"

#include <spdlog/spdlog.h>
#include <sys/stat.h>

#include <ctime>
#include <regex>
#include <stdexcept>

class ArgumentRule {
 public:
  virtual ~ArgumentRule() = default;
  virtual void check(const cmdline::parser& parser) const = 0;
};

// 互斥规则：参数之间互相排斥
class MutuallyExclusiveRule : public ArgumentRule {
  std::vector<std::string> options_;

 public:
  MutuallyExclusiveRule(std::initializer_list<std::string> options)
      : options_(options) {}

  void check(const cmdline::parser& parser) const override {
    int count = 0;
    std::string found;
    for (const auto& opt : options_) {
      if (parser.exist(opt)) {
        count++;
        found = opt;
      }
    }
    if (count > 1) {
      throw std::runtime_error("Conflicting options: " + found);
    }
  }
};

// 依赖规则：一个参数依赖于另一个参数
class DependencyRule : public ArgumentRule {
  std::string dependent_;
  std::vector<std::string> requirements_;

 public:
  DependencyRule(const std::string& dependent,
                 std::initializer_list<std::string> requirements)
      : dependent_(dependent), requirements_(requirements) {}

  void check(const cmdline::parser& parser) const override {
    if (parser.exist(dependent_)) {
      for (const auto& req : requirements_) {
        if (!parser.exist(req)) {
          throw std::runtime_error(dependent_ + " requires " + req);
        }
      }
    }
  }
};

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
  parser.add<std::string>("password", 'p', "加密/解密密码", false,
                          std::string(""));
  parser.add<std::string>("path", '\0', "过滤路径：正则表达式", false);
  parser.add<std::string>(
      "type", '\0', "备份文件类型，可组合使用: n普通文件,l符号链接,p管道文件",
      false);
  parser.add<std::string>("name", '\0', "过滤文件名：正则表达式", false);
  parser.add<std::string>(
      "atime", '\0',
      "按访问时间过滤，格式: START,END 例如: 202401010000,202401312359", false);
  parser.add<std::string>(
      "mtime", '\0',
      "按修改时间过滤，格式: START,END 例如: 202312010000,202312312359", false);
  parser.add<std::string>(
      "ctime", '\0',
      "按状态改变时间过滤，格式: START,END 例如: 202401010000,202401012359",
      false);
  parser.add<std::string>("message", 'm', "添加备注信息", false);
  // 恢复选项
  parser.add("metadata", 'a', "恢复文件的元数据");

  // 验证选项
  parser.add("verify", 'l', "验证备份数据");

  // 添加文件大小过滤选项
  parser.add<std::string>(
      "size", '\0',
      "按文件大小过滤，格式: [<>]N[bkmg]，例如: >1k表示大于1KB, <1m表示小于1MB",
      false);
  // 添加 GUI 选项
  parser.add("gui", 'g', "启动图形界面");
}

void ParserConfig::check_conflicts(const cmdline::parser& parser) {
  std::vector<std::unique_ptr<ArgumentRule>> rules;

  // 添加规则
  rules.emplace_back(
      new MutuallyExclusiveRule({"backup", "restore", "verify"}));

  rules.emplace_back(new DependencyRule("backup", {"input", "output"}));
  rules.emplace_back(new DependencyRule("restore", {"input", "output"}));
  rules.emplace_back(new DependencyRule("verify", {"input"}));
  rules.emplace_back(new DependencyRule("encrypt", {"password"}));

  rules.emplace_back(new PathValidationRule("input", true, false));
  rules.emplace_back(new PathValidationRule("output", false, false));

  // 检查所有规则
  for (const auto& rule : rules) {
    rule->check(parser);
  }
}

namespace {
// 解析时间区间，使用正则表达式一次性验证格式并提取值
std::pair<time_t, time_t> parse_time_range(const std::string& range_str) {
  std::regex time_pattern("(\\d{12}),(\\d{12})");
  std::smatch matches;

  if (!std::regex_match(range_str, matches, time_pattern)) {
    throw std::runtime_error(
        "Invalid time format. Expected: YYYYMMDDHHMM,YYYYMMDDHHMM");
  }

  auto parse_single_time = [](const std::string& time_str) {
    struct tm tm = {};
    tm.tm_year = std::stoi(time_str.substr(0, 4)) - 1900;
    tm.tm_mon = std::stoi(time_str.substr(4, 2)) - 1;
    tm.tm_mday = std::stoi(time_str.substr(6, 2));
    tm.tm_hour = std::stoi(time_str.substr(8, 2));
    tm.tm_min = std::stoi(time_str.substr(10, 2));
    tm.tm_sec = 0;

    time_t time = mktime(&tm);
    if (time == -1) {
      throw std::runtime_error("Invalid time value: " + time_str);
    }
    return time;
  };

  time_t start_time = parse_single_time(matches[1].str());
  time_t end_time = parse_single_time(matches[2].str());

  if (end_time < start_time) {
    throw std::runtime_error("End time cannot be earlier than start time");
  }

  return {start_time, end_time};
}

// 解析文件大小，使用正则表达式一次性验证格式并提取值
int64_t parse_size(const std::string& size_str) {
  std::regex size_pattern("([<>])(\\d+)([bkmg])");
  std::smatch matches;

  if (!std::regex_match(size_str, matches, size_pattern)) {
    throw std::runtime_error("Invalid size format. Expected: [<>]N[bkmg]");
  }

  bool is_greater = (matches[1].str() == ">");
  int64_t value = std::stoll(matches[2].str());
  char unit = matches[3].str()[0];

  // 转换为字节
  switch (unit) {
    case 'b':
      break;
    case 'k':
      value *= 1024;
      break;
    case 'm':
      value *= 1024 * 1024;
      break;
    case 'g':
      value *= 1024 * 1024 * 1024;
      break;
    default:  // 这种情况实际上不会发生，因为正则表达式已经限制了单位
      throw std::runtime_error("Invalid size unit");
  }

  return is_greater ? value : -value;  // 负值表示小于
}
}  // namespace

// 创建文件过滤器
FileFilter ParserConfig::create_filter(const cmdline::parser& parser) {
  return [&parser](const fs::path& path) {
    // 路径过滤
    if (parser.exist("path")) {
      std::regex path_pattern(parser.get<std::string>("path"));
      if (!std::regex_search(path.string(), path_pattern)) {
        return false;
      }
    }

    // 文件名过滤
    if (parser.exist("name")) {
      std::regex name_pattern(parser.get<std::string>("name"));
      if (!std::regex_search(path.filename().string(), name_pattern)) {
        return false;
      }
    }
    // 如果是文件夹就不再进行过滤
    if (fs::is_directory(path)) {
      return true;
    }
    // 文件类型过滤
    if (parser.exist("type")) {
      const std::string& types = parser.get<std::string>("type");
      bool match = false;
      // 先检查是否为软链接,如果是软链接则必须显式指定'l'
      if (fs::is_symlink(path)) {
        match = types.find('l') != std::string::npos;
      } else {
        // 不是软链接时再检查其他类型
        if (fs::is_regular_file(path))
          match |= types.find('n') != std::string::npos;
        if (fs::is_fifo(path)) 
          match |= types.find('p') != std::string::npos;
      }
      if (!match) return false;
    }

    // 获取文件状态
    struct stat st;
    if (lstat(path.c_str(), &st) != 0) {
      spdlog::warn("无法获取文件时间信息: {}", path.string());
      return false;
    }

    auto check_time = [&st](const std::string& time_str, const timespec& ts) {
      if (!time_str.empty()) {
        auto [start, end] = parse_time_range(time_str);
        time_t file_time = ts.tv_sec;
        if (file_time < start || file_time > end) {
          return false;
        }
      }
      return true;
    };

    if (!check_time(parser.get<std::string>("atime"), st.st_atim)) return false;
    if (!check_time(parser.get<std::string>("mtime"), st.st_mtim)) return false;
    if (!check_time(parser.get<std::string>("ctime"), st.st_ctim)) return false;

    // 大小过滤
    if (parser.exist("size")) {
      int64_t threshold = parse_size(parser.get<std::string>("size"));
      if (threshold > 0) {  // 大于
        if (st.st_size <= threshold) return false;
      } else {  // 小于
        if (st.st_size >= -threshold) return false;
      }
    }

    return true;
  };
}