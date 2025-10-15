# Union

提供应用中常用的工具和模块。    

包含的模块：
- archivist：数据存储相关
- assistant：工具函数
- commander：命令行相关
- expressman：数据传递相关
- labourer：多线程相关
- referee：异常和异常处理
- secretary：输入输出相关，日志相关
内部模块：
- framework：本库配置
- exceptions：本库异常

本库全部内容都在 ``HYDRA15::Union`` 命名空间下，
每个模块单独拥有自己的命名空间，模块命名空间和模块名相同。    
以下文档分模块简要介绍本库内容，并给出使用示例，
详细的使用方式请阅读源代码及其中的注释。

## labourer

主要包含后台线程类和线程池两个模块，以及一些可供多线程使用的工具模块

### background：后台线程基类

对于需要创建后台工作线程的类，可以``protect``继承此类并重写
``virtual void work(thread_info)``函数，后台线程将在``start()``
函数被调用之后开始执行``work()``中的内容。在对象析构时，可以调用
``wait_for_end()``等待后台线程完成任务，也可以不做任何操作以放任后台线程
自己退出，程序不会因此报错，但是你仍需考虑成员变量声明周期的问题。

> 后台线程不是异常安全的，如果工作中出现未处理的错误，线程会退出，并且无法再次启动


### ThreadLake：线程池

如你所见，这是一个线程池。    
创建时需要指定后台工作的线程数，而后你便可使用多种方法提交任务。    
每次提交任务都将返回一个``std::shared_future``对象，你可以从中获取任务的返回值，
任务所抛出的异常也会在获取返回值时重新抛出。    
通过特别的提交接口，你可以注册任务执行完成后执行的回调函数。    
可以通过迭代器接口访问每一个线程的``labourer::background::thread_info``
结构，用于监控每个线程的健康状态。

用法示例：
创建了一个线程池，向其中提交了多个任务，并获取返回值
```cpp
using namespace HYDRA15::Union;

int async_work(int a, int b)
{
    std::this_thread::sleep_for(std::chrono::seconds(a));
    return b;
}

void callback(int a)
{
    std::cout<< "callback recived value: " << a << std::endl;
}

int main()
{
    labourer::ThreadLake tl(4); // 指定线程数为4
    auto ret1 = tl.submit(async_work, 1, 2);    // 通过函数指针添加任务
    auto ret2 = tl.submit(std::bind(async_work, 3, 4)); // 通过 `std::function` 添加任务
    tl.submit(std::bind(async_work, 5, 6), callback);   // 添加任务并注册回调

    std::cout << ret1.get() << " " << ret2.get() << std::endl;
    return 0;
}

```

### wirte_first_mutex

写优先的锁。   
标准库的``std::shared_mutex``无法处理读多写少的情况，
写线程很容易被大量读线程永远挡在门外。``wirte_first_mutex``支持在有
写线程等待时，禁止新的读线程继续上锁，只有没有写线程等待时读线程才可自由上锁。    
与``std::shared_mutex``类似，``wirte_first_mutex``拥有``lock()``
和``lock_shared()``两种上锁接口。使用前者上锁将视为写线程，
使用后者则会被视为读线程。

### shared_container_base

一个线程安全的容的模板类，使用其接口调用容器接口时会按照需要自动上锁。    
> 由于过度包装，其使用极其复杂，此处不做过多介绍，
> 具体的使用方法在源码注释中有介绍

## secretary

## assistant

## archivist

## expressman

## referee

## commander



