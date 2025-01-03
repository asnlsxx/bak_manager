# 设置CMake的最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版本
project(backupmanager VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置 ImGui 目录
set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/third_party/imgui)

# 添加 ImGui 源文件
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# 查找必要的包
find_package(OpenSSL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# 添加include目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# 创建核心库（不包含GUI）
add_library(core STATIC
    src/Packer.cpp
    src/FileHandler.cpp
    src/ArgParser.cpp
    src/Compression.cpp
    src/aes.cpp
)

# 链接核心库的依赖
target_link_libraries(core
    PRIVATE
    OpenSSL::Crypto
)

# 创建GUI库
add_library(gui STATIC
    src/GUI.cpp
    ${IMGUI_SOURCES}
)

# 链接GUI库的依赖
target_link_libraries(gui
    PRIVATE
    core
    glfw
    OpenGL::GL
)

# 创建可执行文件
add_executable(BackupManager src/main.cpp)

# 链接库到可执行文件
target_link_libraries(BackupManager
    PRIVATE 
    core
    gui
)

# 如果需要链接文件系统库
if(UNIX AND NOT APPLE)
    target_link_libraries(core
        PRIVATE
        stdc++fs
    )
endif()

# 添加安装规则
install(TARGETS BackupManager DESTINATION bin)
install(TARGETS core gui DESTINATION lib)
install(FILES include/Packer.h DESTINATION include)

# 添加测试支持
enable_testing()

# 查找Catch2包
find_package(Catch2 3 REQUIRED)

# 添加BDD测试可执行文件
add_executable(BDD_test tests/BDD_test.cpp)
target_link_libraries(BDD_test 
    PRIVATE 
    Catch2::Catch2WithMain
    core
)

# 添加单元测试可执行文件
add_executable(unit_tests
    tests/AESModule_test.cpp
    tests/LZWCompression_test.cpp
    tests/ArgParser_test.cpp
)
target_link_libraries(unit_tests 
    PRIVATE
    Catch2::Catch2WithMain
    core
)

# 将测试添加到CTest
include(Catch)
catch_discover_tests(BDD_test)
catch_discover_tests(unit_tests)