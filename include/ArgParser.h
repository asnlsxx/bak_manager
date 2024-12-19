#ifndef PARSER_CONFIG_H
#define PARSER_CONFIG_H

#include "cmdline.h"
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

// 定义文件过滤器类型
using FileFilter = std::function<bool(const fs::path&)>;

class ParserConfig {
public:
    // 配置命令行解析器
    static void configure_parser(cmdline::parser& parser);
    
    // 检查选项冲突
    static void check_conflicts(const cmdline::parser& parser);
    
    // 创建文件过滤器
    static FileFilter create_filter(const cmdline::parser& parser);
};

#endif // PARSER_CONFIG_H 