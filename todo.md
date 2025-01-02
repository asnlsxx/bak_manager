采用相对路径打包（相对路径的根目录为备份目录）
硬链接修改（原文件在没被备份，则所有硬链接当作普通文件）
长文件名
验证
加密
压缩

统一
backup_file.write(link_buffer, MAX_PATH_LEN);
this->WriteHeader(backup_file);

超大文件（夹）测试

参数冲突检测

任务分配

长文件名

测试 再多样并简化
> 传入整个结构，参数以及预期结果
> 一个 BDD 测试的内容太少了，能不能丰富点

 验证功能的测试

打包失败（例如待备份文件夹不存在时）不要创建备份文件

让TestFixture更通用

注释（/**
 * @brief 压缩指定路径的文件。
 * 
 * @param inputPath 输入文件的路径。
 * @return false 压缩失败。
 */）

 std::string data((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());

 // 通过buackupinfo传入给packer

 pack and unpack 不再用文件

对规则引擎的测试

测试除了type -n以外的文件类型过滤