# 添加测试可执行文件
add_executable(simple_test simple_test.cpp)

# 链接Catch2
target_link_libraries(simple_test PRIVATE Catch2::Catch2WithMain)

# 将测试添加到CTest
include(Catch)
catch_discover_tests(simple_test)
