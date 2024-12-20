#include "ArgParser.h"

#include <regex>
#include <stdexcept>

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
                          "过滤文件类型: n普通文件,d目录文件,l符号链接", false);
  parser.add<std::string>("name", '\0', "过滤文件名：正则表达式", false);
  parser.add<std::string>("atime", '\0', "文件的访问时间区间", false);
  parser.add<std::string>("mtime", '\0', "文件的修改时间区间", false);
  parser.add<std::string>("ctime", '\0', "文件的改变时间区间", false);
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

    // 文件类型过滤
    if (parser.exist("type")) {
      std::string type = parser.get<std::string>("type");
      fs::file_status status = fs::status(path);
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
        default:
          file_type = 'x';
          break;
      }
      if (type.find(file_type) == std::string::npos) {
        return false;
      }
    }

    // TODO: 添加时间过滤功能

    return true;
  };
}