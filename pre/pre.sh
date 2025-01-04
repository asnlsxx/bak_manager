#!/bin/bash

# 创建origin目录
mkdir -p origin

# 进入origin目录
cd origin

# 创建普通文件
echo "测试文件1" > file1.txt
echo "测试文件2" > file2.dat
echo "这是一个配置文件" > config.conf
echo "<?xml version=\"1.0\"?><root>测试XML</root>" > data.xml

# 创建子目录及其文件
mkdir -p subdir
echo "子目录中的文件" > subdir/file2.txt
echo "日志文件" > subdir/app.log
mkdir -p subdir/empty_dir

# 创建深层目录结构
mkdir -p subdir/level2/level3
echo "深层目录文件" > subdir/level2/level3/deep.txt

# 创建软链接
ln -s file1.txt link1
ln -s subdir/file2.txt link2
ln -s ../file1.txt subdir/link3

# 创建硬链接
ln file1.txt hardlink1
ln subdir/file2.txt subdir/hardlink2

# 创建管道文件
mkfifo pipe1
mkfifo subdir/pipe2
mkfifo subdir/level2/pipe3

# 创建大文件
dd if=/dev/zero of=large_file.dat bs=1M count=10

# 创建中文名称的文件和目录
mkdir -p "中文目录"
echo "中文内容" > "中文目录/中文文件.txt"
ln -s "中文目录/中文文件.txt" "中文链接"
mkfifo "中文目录/中文管道"

# 设置不同的权限
chmod 644 file1.txt
chmod 600 file2.dat
chmod 755 subdir
chmod 444 subdir/file2.txt
chmod 666 pipe1
chmod 777 "中文目录"

# 设置不同的时间戳
touch -t 202301010000 file1.txt
touch -t 202302020000 file2.dat
touch -t 202303030000 subdir/file2.txt

# 返回上级目录
cd ..

# 输出创建的文件结构
echo "文件结构已创建:"
echo "./origin"
echo "├── file1.txt"
echo "├── file2.dat"
echo "├── config.conf"
echo "├── data.xml"
echo "├── large_file.dat (10MB)"
echo "├── hardlink1 -> file1.txt (硬链接)"
echo "├── link1 -> file1.txt (软链接)"
echo "├── pipe1 (管道文件)"
echo "├── 中文目录"
echo "│   ├── 中文文件.txt"
echo "│   └── 中文管道 (管道文件)"
echo "├── 中文链接 -> 中文目录/中文文件.txt (软链接)"
echo "└── subdir"
echo "    ├── file2.txt"
echo "    ├── app.log"
echo "    ├── empty_dir"
echo "    ├── hardlink2 -> file2.txt (硬链接)"
echo "    ├── link3 -> ../file1.txt (软链接)"
echo "    ├── pipe2 (管道文件)"
echo "    └── level2"
echo "        ├── level3"
echo "        │   └── deep.txt"
echo "        └── pipe3 (管道文件)"