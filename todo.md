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

多个短选项支持

文件大小过滤

测试 再多样并简化
> 传入整个结构，参数以及预期结果