# 备份管理器 (BackupManager)

一个功能强大的文件备份工具，支持图形界面和命令行操作，提供文件过滤、压缩加密等多种功能。

## ✨ 主要特性

### 基础功能
- 文件和目录的备份与还原
- 备份数据完整性验证
- 支持命令行和图形界面操作

### 高级特性
- 文件过滤功能
  - 支持按文件名、路径过滤（正则表达式）
  - 支持按文件类型过滤（普通文件、符号链接、管道文件）
  - 支持按文件大小过滤
  - 支持按访问/修改/创建时间过滤
- 数据处理
  - LZW压缩算法支持
  - AES加密保护
  - 文件元数据保存和还原
- 特殊文件支持
  - 软链接文件
  - 硬链接文件
  - 管道文件

## 🚀 快速开始

### 环境要求
- C++20
- CMake 3.10+
- OpenSSL
- OpenGL
- GLFW3
- ImGui
- Catch2 (用于测试)

### 安装方式

请注意拉取本项目后，还需要拉取子模块
```bash
git submodule update --init --recursive
```

#### 使用 Docker
我们提供了 Dockerfile 来简化环境配置：

```bash
# 构建镜像，编译并安装软件
docker build -t backupmanager:latest .

# 运行开发环境
docker run -it --rm --name backupmanager backupmanager /bin/bash

# 成功运行
BackupManager -h
```

### 运行测试
```bash
cd build
ctest --output-on-failure
```

## 💻 使用说明

### 命令格式
```bash
BackupManager [操作] [选项] <参数>

操作:
  -b, --backup            备份模式
  -r, --restore          还原模式
  -l, --verify           验证模式
  -g, --gui              启动图形界面

必选参数:
  -i, --input <路径>     输入路径/备份文件
  -o, --output <路径>    输出路径（备份/还原位置）

可选功能:
  -c, --compress         启用压缩
  -e, --encrypt          启用加密
  -p, --password <密码>  设置加密密码
  -a, --metadata        还原元数据

过滤选项:
  --type <类型>         按类型过滤，可选值:
                        n (普通文件)
                        l (符号链接)
                        p (管道文件)
  --name <模式>         按文件名过滤(正则表达式)
  --path <模式>         按路径过滤(正则表达式)
  --size <表达式>       按大小过滤(例如:>1k,<1m)
  --atime <时间范围>    按访问时间过滤
  --mtime <时间范围>    按修改时间过滤
  --ctime <时间范围>    按创建时间过滤
```

### 命令行模式
```bash
# 基本操作
./BackupManager -b -i <输入路径> -o <输出路径>  # 备份
./BackupManager -r -i <备份文件> -o <还原路径>  # 还原
./BackupManager -l -i <备份文件>                # 验证

# 高级选项
-c, --compress         # 启用压缩
-e, --encrypt          # 启用加密
-p, --password <密码>  # 设置加密密码
-a, --metadata        # 还原元数据
--type <n,l,p>        # 按类型过滤(n:普通文件,l:符号链接,p:管道文件)
--name <pattern>      # 按文件名过滤(正则表达式)
--path <pattern>      # 按路径过滤(正则表达式)
--size <[<>]N[bkmg]>  # 按大小过滤(例如:>1k,<1m)
```

### 图形界面模式
```bash
./BackupManager -g
```

## ⚠️ 注意事项

1. 加密备份
   - 请妥善保管加密密码，密码丢失将无法恢复数据
   - 确保在还原时使用正确的密码

2. 备份操作
   - 确保有足够的磁盘空间
   - 建议在备份完成后进行验证
   - 查看日志窗口获取详细的操作信息

3. 文件过滤
   - 路径过滤使用正则表达式
   - 确保过滤规则正确，避免遗漏重要文件

## 🔨 开发相关

### 项目结构
- `src/`: 源代码文件
- `include/`: 头文件
- `tests/`: 测试文件
- `third_party/`: 第三方库

修改代码后，需要重新编译安装
```bash
rm -rf build && mkdir build
cd build && cmake .. && make && make install
```

## 📝 使用示例

### 基础备份还原

```bash
# 备份整个目录
./BackupManager -b -i ~/Documents -o ~/Backups

# 还原备份文件
./BackupManager -r -i ~/Backups/Documents.backup -o ~/Restored

# 验证备份文件完整性
./BackupManager -l -i ~/Backups/Documents.backup
```

### 文件过滤示例

```bash
# 只备份.txt和.doc文件
./BackupManager -b -i ~/Documents -o ~/Backups --name ".*\.(txt|doc)$"

# 备份大于1MB的文件
./BackupManager -b -i ~/Documents -o ~/Backups --size ">1m"

# 备份特定目录下的文件
./BackupManager -b -i ~/Documents -o ~/Backups --path "^projects/.*"

# 只备份普通文件和符号链接
./BackupManager -b -i ~/Documents -o ~/Backups --type nl
```

### 压缩和加密

```bash
# 启用压缩的备份
./BackupManager -b -i ~/Documents -o ~/Backups -c

# 启用加密的备份
./BackupManager -b -i ~/Documents -o ~/Backups -e -p mypassword

# 压缩并加密备份
./BackupManager -b -i ~/Documents -o ~/Backups -c -e -p mypassword

# 还原加密的备份
./BackupManager -r -i ~/Backups/Documents.backup -o ~/Restored -p mypassword
```

### 按时间过滤

```bash
# 备份特定时间段内访问的文件
./BackupManager -b -i ~/Documents -o ~/Backups --atime "202403010000,202403312359"
```

### 元数据处理

```bash
# 备份时保存元数据
./BackupManager -b -i ~/Documents -o ~/Backups -a

# 还原时恢复元数据
./BackupManager -r -i ~/Backups/Documents.backup -o ~/Restored -a
```

### 图形界面操作

1. 启动图形界面：
```bash
./BackupManager -g
```

2. 在界面中：
   - 选择源文件/目录和目标路径
   - 配置过滤选项
   - 选择是否启用压缩和加密
   - 点击"备份"/"还原"/"验证"按钮执行操作
   - 查看实时日志输出




