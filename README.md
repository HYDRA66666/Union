# Union

提供应用中常用的工具和模块。    

包含的模块：
- assistant：工具函数
- commander：命令行相关
- expressman：数据存储、传输、传递相关
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

主要包含后台 background 和 ThreadLake 两个类，以及一些可供多线程使用的工具模块

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
根据提交的方式不同，每次提交任务都将返回一个``std::shared_future``或一个``std::future``
对象，你可以从中获取任务的返回值，任务所抛出的异常也会在获取返回值时重新抛出。    
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
    auto ret1 = tl.submit(async_work, 1, 2);    // 直接调用函数，此情况不支持回调函数
    auto ret2 = tl.submit(std::bind(async_work, 3, 4));    // 传入 bind 对象，或者 std::function 加参数亦可，同样不支持回调
    auto ret3 = tl.submit(  // 支持回调的调用版本，必须显式指定两个参数为 std::function
        std::function<int()>(std::bind(async_work, 5, 6)),  // 任务函数对象
        std::function<void(int)>(call_back)                 // 回调函数
    );

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
你可以和使用``std::shared_mutex``一样使用本锁。

用法示例：    
实现了一个字典类，提供了查字典与添加字典条目的接口
```cpp
using namespace HYDRA15::Union;

class dictionary
{
private:
    std::unordered_map<std::string, std::string> dict;
    labourer::write_first_mutex mtx;

public:
    void modify(std::string key, std::string value)
    {
        std::unique_lock ul(mtx);   // 使用 lock guard 类来管理锁周期，unique_lock 为写锁
        dict[key] = value;
    }

    std::string lookup(std::string key)
    {
        std::shared_lock sl(mtx);   // shared_lock 为读锁
        return dict.at(key);
    }
}

```

### shared_container_base

一个线程安全的容的模板类，使用其接口调用容器接口时会按照需要自动上锁。    
> 由于过度包装，其使用极其复杂，此处不做过多介绍，
> 具体的使用方法在源码注释中有介绍

## secretary

主要包含 PrintCenter 和 ScanCenter 两个模块，以及一些和日志格式化、进度条格式化相关的工具

### PrintCenter

此类用于协调不同线程的、不同类型的控制台输出请求。    
此类将输出请求分成三类：滚动消息、底部消息和粘底消息。    
滚动消息即一般的控制台消息，新的行将会排列在旧的行下方，窗口满后旧行会依次向上滚动。    
底部消息通常为进度条、运行提示等，其将一直保持在所有消息的最下方，新的滚动消息将会在它上方生成。
作为优化，如果底部消息特别多，系统将只展示其中几条；如果一个底部消息长时间没有更新，系统会将他隐藏。    
粘底消息通常是输入提示词，它将会排列在底部消息下方，同样不受滚动消息的影响。
粘底消息不会受到超时条件的约束，代价是同一时间只能有一条粘底消息，新的粘底消息会将旧的覆盖。    
> 由于使用了 ANSI 转义串来控制光标移动，要求控制台必须支持 ANSI 转义，否则无法正常工作。    
> 此类采用懒汉单例模式设计。
> 此类初始化时会将 ``std::cout`` 重定向至自身，如果你不想使用此特性，请确保你的代码中没有调用其 
``get_instance()`` 函数，注意，本库其他类可能会使用此类，详见本文档。    
> 使用被重定向的``std::cout``来输出内容时，仅当缓冲区被刷新时，缓冲区内的内容才会被作为一整行输出。

每种消息都有对应的静态接口，详细用法参见源码。

使用示例：    
实现一个无限循环的程序，获取用户输入并原样输出为滚动消息，用户输入前将会有提示词。
```cpp
using namespace HYDRA15::Union;

int main()
{
    secretary::PrintCenter pc = secretary::PrintCenter::get_instance();
    pc.set_stick_btm("请输入内容");
    while(true)
    {
        std:;string str;
        std::get_line(std::cin, str);
        pc.println("你输入的内容为：", str);    // 支持依次在一行内输出多个内容
        // 或者
        // pc.printf("你输入的内容为：{}", str);  // 支持格式化字符串
        // 或者
        // pc << str;   // 支持老式的流插入运算符，注意此操作仍将所有内容作为一行输出
        // 或者
        // std::cout << str << std::endl; // `std::cout`已被重定向
    }
}

```

### ScanCenter

此类用于协调不同模块和线程的输入请求，同时提供非阻塞的异步输入和程序发送的伪输入的功能。    
此类将一次处理每一个输入请求，打印提示词、获取用于输入的内容、最后将内容通过std::future发送给请求线程。    
对于每个输入请求，用户都可以设置一个请求ID。其他线程可以替代用户操作，将输入内容发送至此类，此类将以
用户设定的ID作为标识将内容发送给等待中的线程。    
如果用户输入内容时并没有任何线程等待，并且开发者为此类设置了 ``assign`` 函数，输入的内容将会被发送至 
``assign``，此功能通常适用于交互式命令行执行；如果没有设置 ``assign`` 函数，
输入内容会以默认ID添加至输入队列，等待匹配的线程认领。    
> 使用此类输入时，一次只能获取整行的输入内容    
> 为了实现提示词打印的功能，此类引用了 ``PrintCenter``，这意味着启动此类时会自动重定向 ``std::cout`` 
> 到 ``PrintCenter``    
> 此类采用懒汉模式的单例设计，并且在启动时会自动重定向 ``std::cin`` 到本类。如果你不想使用此特性，请确保
> 在你的程序的任何地方都没有调用 ``get_instance()`` 函数。另外，本库的某些模块也引用了此类，你也需确保没有
> 引用它们。引用本类的模块的详细信息参见此文档。

### log & logger

log 是一个静态类，用于生成格式化的日志字符串。    
日志分为 info、warn、error、fatal、debug、trace 六个等级，分别用不同的颜色标识。日志内容包含 日期和时间、
等级、标题和内容四个部分，其中日期和时间在生成日志时由系统生成，等级由调用的接口决定，标题和内容由用户输入。    
> log使用 ansi 转义串来控制内容的颜色，这要求控制台必须支持 ansi 转义。如果不想使用此特性，可以使用
> ``std::string assistant::strip_ansi_color(const std::string& str)`` 函数来去除所有的颜色信息，
> 此函数被包含于 ``assistant`` 模块中。

logger 通常在模块内部为模块发布日志提供帮助，其在创建时存储传入的 标题 字段，并自动将此字段添加至每一个
日志中。你可以使用 ``assistant::logger lgr = UNION_CREATE_LOGGER()`` 来自动捕获创建 logger 时的函数名。
同时，logger还集成了格式化日志内容的功能。

log 创建日志时，默认只返回日志字符串。你可以修改其 ``std::function<void(const std::string&)>print``
成员变量，让其自动将日志输出至 ``print`` 。    
设置了 ``print`` 时，log 默认会在 DEBUG 环境下输出 debug、trace 等级的日志，你可以通过修改其 ``bool enableDebug``
成员变量自定义此行为。    

使用示例：    
用两种方式输出日志。
```cpp
using namespace HYDRA15::Union;

void my_modual_service(int a, int b)
{
    // 实用宏创建 logger 将自动捕获函数名 my_modual_service 作为标题
    secretary::logger lgr = UNION_CREATE_LOGGER();
    // 启用 DEBUG 输出
    secretary::log::enableDebug = true;
    // 设置自动输出
    secretary::log::print = [](const std::string& str){ std::cout<< str << std::endl; }

    lgr.debug("my_modual_service is running");  // 由于前面启用了 DEBUG ，此条会正常打印
    lgr.info("recived param: {:04X} : {:04X}", a, b);   // 格式化日志内容并输出。
}
```

## assistant

提供了一些实用工具。

### datetime

存储日期、时间的对象，并且可以用于格式化输出日期、时间字符串。
使用 ``assistant::datetime assistant::datetime::now()`` 创建一个记录了当前时间的 datetime 对象。    
使用 ``std::string date_time(std::string format, int timeZone)`` 
将对象 dt 中记录的时间格式化为字符串。其中第二个参数是整数表示的时区，默认为东八区。
或者使用 ``std::string now_date_time(std::string format, int timeZone)`` 合并执行上述两步。    
> 目前， datetime 使用 C 风格的时间处理逻辑，在将来的更新中，可能会更新为 C++ 风格的 std::chrono 时间处理逻辑。

### utilities

包含丰富的工具函数，详情请参阅源码。

## expressman

## referee

## commander



