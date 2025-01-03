#include <catch2/catch_test_macros.hpp>
#include "ArgParser.h"

TEST_CASE("参数解析基础功能测试", "[argparser]") {
    SECTION("必选参数测试") {
        const char* args[] = {
            "program",
            "-b",
            "-i", "/input/path",
            "-o", "/output/path"
        };
        cmdline::parser parser;
        ParserConfig::configure_parser(parser);
        REQUIRE(parser.parse(sizeof(args)/sizeof(args[0]), const_cast<char**>(args)));
        REQUIRE(parser.get<std::string>("input") == "/input/path");
        REQUIRE(parser.get<std::string>("output") == "/output/path");
    }

    SECTION("可选参数测试") {
        const char* args[] = {
            "program",
            "-b",
            "-i", "/input/path",
            "-o", "/output/path",
            "-c",
            "-e",
            "-p", "password",
            "-a"
        };
        cmdline::parser parser;
        ParserConfig::configure_parser(parser);
        REQUIRE(parser.parse(sizeof(args)/sizeof(args[0]), const_cast<char**>(args)));
        REQUIRE(parser.exist("compress"));
        REQUIRE(parser.exist("encrypt"));
        REQUIRE(parser.get<std::string>("password") == "password");
        REQUIRE(parser.exist("metadata"));
    }

    SECTION("过滤选项测试") {
        const char* args[] = {
            "program",
            "-b",
            "-i", "/input/path",
            "-o", "/output/path",
            "--type", "nl",
            "--name", ".*\\.txt$",
            "--size", ">1m",
            "--path", "^/home/.*"
        };
        cmdline::parser parser;
        ParserConfig::configure_parser(parser);
        REQUIRE(parser.parse(sizeof(args)/sizeof(args[0]), const_cast<char**>(args)));
        REQUIRE(parser.get<std::string>("type") == "nl");
        REQUIRE(parser.get<std::string>("name") == ".*\\.txt$");
        REQUIRE(parser.get<std::string>("size") == ">1m");
        REQUIRE(parser.get<std::string>("path") == "^/home/.*");
    }
}

TEST_CASE("参数解析错误处理", "[argparser][error]") {
    SECTION("缺少必选参数") {
        const char* args[] = {
            "program",
            "-b"  // 缺少-i和-o
        };
        cmdline::parser parser;
        ParserConfig::configure_parser(parser);
        parser.parse(sizeof(args)/sizeof(args[0]), const_cast<char**>(args));
        REQUIRE_THROWS(ParserConfig::check_conflicts(parser));
    }

    SECTION("模式互斥测试") {
        const char* args[] = {
            "program",
            "-b", "-r",  // 备份和还原模式不能同时使用
            "-i", "/input/path",
            "-o", "/output/path"
        };
        cmdline::parser parser;
        ParserConfig::configure_parser(parser);
        parser.parse(sizeof(args)/sizeof(args[0]), const_cast<char**>(args));
        REQUIRE_THROWS(ParserConfig::check_conflicts(parser));
    }

    SECTION("加密缺少密码") {
        const char* args[] = {
            "program",
            "-b",
            "-i", "/input/path",
            "-o", "/output/path",
            "-e"  // 启用加密但未提供密码
        };
        cmdline::parser parser;
        ParserConfig::configure_parser(parser);
        parser.parse(sizeof(args)/sizeof(args[0]), const_cast<char**>(args));
        REQUIRE_THROWS(ParserConfig::check_conflicts(parser));
    }

} 