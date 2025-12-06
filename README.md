# Union

> HYDRA15.Union ver.lib.beta.1.2.0 @HYDRA15 MIT

提供应用中常用的工具和模块。    

包含的模块（仅列出部分常用）：
- archivist：数据库相关组件
    - sfstream：分节文件
    - simple_memory_table：内存表
- labourer：多线程相关
    - ThreadLake：线程池
    - iMutexies：各种互斥锁
- secretary：输入输出相关，日志相关
    - PrintCenter：协调控制台输出
    - log：格式化日志
- assistant：工具函数和类

更多详细信息，请参见源码

本库全部内容都在 ``HYDRA15::Union`` 命名空间下，
每个模块单独拥有自己的命名空间，模块命名空间和模块名相同。    

要使用本库：    
1. 包含 Union 目录下你想要的头文件，或者直接包含单头文件 Union.h
1. 链接对应版本的静态库 Union.lib

## update lib.release.1.2.1

更新内容：
- 新增示例程序 demo.EasyFileBackup，演示本库部分常用模块的用法，详见项目源码

## update lib.beta.1.2.0

> 本版本开始不再提供编译后的二进制文件，因为绝大部分模块已经完成单头文件改造而无需发布时编译。    
> 剩余未完成改造的模块还有：``secretary::PrintCenter``, ``secretary::ScanCenter``    

更新内容：
- 异常系统改造，新的异常系统更加简洁
- 新增 assistant::bfstream, assistant::bsfstream 二进制文件包装类
- 新增 archivist::sfstream, archivist::single_loader, archivist::simple_memory_table 数据库组件
- 修补错误，优化代码

