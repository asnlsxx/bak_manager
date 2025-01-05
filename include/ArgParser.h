#ifndef PARSER_CONFIG_H
#define PARSER_CONFIG_H

#include "cmdline.h"
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

/**
 * @brief 文件过滤器类型定义
 */
using FileFilter = std::function<bool(const fs::path&)>;

/**
 * @brief 命令行参数解析配置类
 */
class ParserConfig {
public:
    /**
     * @brief 配置命令行解析器
     * @param parser 要配置的解析器
     */
    static void configure_parser(cmdline::parser& parser);
    
    /**
     * @brief 检查选项冲突
     * @param parser 解析器
     */
    static void check_conflicts(const cmdline::parser& parser);
    
    /**
     * @brief 创建文件过滤器
     * @param parser 解析器
     * @return 文件过滤器函数
     */
    static FileFilter create_filter(const cmdline::parser& parser);
};

#endif // PARSER_CONFIG_H 