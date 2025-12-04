/*************************************************************************************
* 此项目使用数据库记录文件信息和状态，以实现文件备份和自动更新备份的功能
* 在源目录运行此程序，此程序将在启动时自动扫描当前目录及其子目录中的文件，
*   比较文件是否有更新、删除，并按照指令完成向目标目录的备份工作
* 文件被分为三种状态：
*   normal (正常)：文件未被修改，已备份且未更新
*   updated (已更新)：文件已被修改，需重新备份
*   deleted (已删除)：文件已被删除，等待删除备份文件
* 
* 用法：
*   启动参数 (dbPath)：数据库文件的路径。数据库的父目录将会被视为源目录。
*   命令 set_all [normal, updated]：将所有文件状态设置为指定状态
*   命令 sync (destPath) [normal, deleted, updated, del_upd]：将源目录中的文件同步到目标目录，
*       可选选项指定要同步的文件状态
*   命令 get [deleted, updated]：获取数据库中所有指定状态的文件的列表
*   命令 get [path]：获取指定路径的文件信息
*   命令 help：显示帮助信息
* *************************************************************************************/


#include "pch.h"


#include "scan_service.h"

using namespace HYDRA15::Union;

void init()
{
    secretary::log::print = [](const std::string& msg) {static secretary::PrintCenter& pc = secretary::PrintCenter::get_instance(); pc.println(msg); };

    database_service::init("file_backup.db");
};

int main()
{
    init();


    file_scan_service fss{ std::filesystem::current_path()};
    //std::this_thread::sleep_for(std::chrono::seconds(10));
}
