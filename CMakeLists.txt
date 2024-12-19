# 设置CMake的最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版本
project(backupmanager VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加include目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 添加源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 创建库
add_library(packer STATIC ${SOURCES})

# 创建可执行文件
add_executable(BackupManager src/main.cpp)

# 链接库到可执行文件
target_link_libraries(BackupManager packer)

# 如果需要链接文件系统库（在某些系统上可能需要）
if(UNIX AND NOT APPLE)
    target_link_libraries(BackupManager stdc++fs)
endif()

# 添加安装规则
install(TARGETS BackupManager DESTINATION bin)
install(TARGETS packer DESTINATION lib)
# install(FILES include/Packer.h DESTINATION include)

# 添加测试支持
enable_testing()

# 查找Catch2包
find_package(Catch2 3 REQUIRED)


# 添加BDD测试可执行文件
add_executable(packer_bdd_test tests/packer_bdd_test.cpp)

# 链接必要的库
target_link_libraries(packer_bdd_test 
    PRIVATE 
    Catch2::Catch2WithMain
    packer
)

# 将测试添加到CTest
include(Catch)
catch_discover_tests(packer_bdd_test)