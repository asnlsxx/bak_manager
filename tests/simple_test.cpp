#define CATCH_CONFIG_MAIN  // 这个宏告诉Catch2生成main()函数
#include <catch2/catch.hpp>

TEST_CASE("简单的测试用例", "[sample]") {
    REQUIRE(1 + 1 == 2);
    REQUIRE_FALSE(1 + 1 == 3);
    
    SECTION("测试基本数学运算") {
        CHECK(2 * 2 == 4);
        CHECK(10 - 5 == 5);
    }
}
