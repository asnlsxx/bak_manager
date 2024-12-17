# 添加BDD测试可执行文件
add_executable(packer_bdd_test packer_bdd_test.cpp)

# 链接必要的库
target_link_libraries(packer_bdd_test 
    PRIVATE 
    Catch2::Catch2WithMain
    packer
)

# 将测试添加到CTest
include(Catch)
catch_discover_tests(packer_bdd_test)

# 添加过滤器测试
add_executable(filter_test filter_test.cpp)
target_link_libraries(filter_test 
    PRIVATE 
    Catch2::Catch2WithMain
    packer
)

# 将测试添加到CTest
catch_discover_tests(filter_test)