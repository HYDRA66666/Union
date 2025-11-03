#pragma once
#include "framework.h"
#include "pch.h"

#include "concepts.h"
#include "expressman_exception.h"
#include "utilities.h"

namespace HYDRA15::Union::expressman
{
    // 用于类型擦除的基础接口
    class notype
    {
    public:
        virtual ~notype() = default;

        virtual std::shared_ptr<notype> clone() const = 0;    // 类型擦除状态下的拷贝构造
        virtual constexpr const std::type_info& type() const noexcept = 0;    // 返回类型信息，基本上就是 typeid(class)

        // 扩展接口
        virtual std::string info() const;
        virtual size_t class_size() const;    // 获取类本身的大小，不包含对象管理的外部数据，基本上就是 size_of(class)
    };

    /***************************** 数据传输相关 *****************************/
// 交换数据用的帧格式
// 一帧大小为 4KB ，其中 96B 头，4000B数据
// 数据区集中排布在末尾，因此可以放弃传输尾部没有有效数据的部分
    struct packet
    {
        using byte = uint8_t;
        using uint = uint16_t;
        static constexpr size_t maxClassNameSize = 88;
        static constexpr size_t maxFrameDataSize = 4000;

        struct header_t
        {
            uint frameNo = 0;
            uint frameTotal = 0;
            uint serialNo = 0;
            uint dataLength = 0;
            char className[maxClassNameSize] = {};
        }header;
        byte data[maxFrameDataSize] = {};
    };

    // 可将对象内部的全部数据打包成标准的帧，用于数据交换和数据持久化
    class packable
    {
    public:
        using datablock = std::vector<packet::byte>;
        using datablocks = std::list<datablock>;
        using class_name_array = std::array<char, packet::maxClassNameSize>;
        using objects = std::list<std::shared_ptr<packable>>;
    protected:
        // 用户仅需实现 将数据按照自己的设计打包成连续的二进制数据
        // 将内存数据打包成帧的工作将交由系统完成
        virtual datablocks packing() const = 0;
        virtual std::any unpacking(const datablocks&) = 0;
        virtual constexpr class_name_array class_name_pack() const = 0;    // 返回的类名称将填入className字段，作为类的唯一标识符，在重构对象时使用
        virtual packet::uint serialNo_fetch_and_increase() const = 0;     // 用于标记对象实例的序列号，对于类而言应当是全局静态的

    public:
        virtual ~packable() = default;

        std::list<packet> pack(size_t maxFrameSize = sizeof(packet)) const;
        std::any unpack(const std::list<packet>& archives);

        // 扩展接口
        virtual size_t obj_size() const;    // 返回对象必需数据的总大小，为对象大小和被对象管理的数据大小之和
    };
    packable::objects unpack(const std::list<packet>& archives, std::function<packable::objects(const packable::datablocks&)> constructor);
    std::string extract_name(const packet& arch);

    // 远程数据传输的代理
    // 用于隐藏各种传输方式的底层细节
    class agent
    {
    public:
        virtual bool send(const std::list<packet>& archives) const = 0;
        virtual std::list<packet> recv() const = 0;
        virtual std::list<packet> try_recv() const = 0;

        virtual ~agent() = default;
    };


    /***************************** 数据传递相关 *****************************/
    // 可被投递的接口
    // 模板参数为地址类型
    template<framework::hash_key A>
    class postable : virtual public notype
    {
    public:
        virtual A origin() const { return std::string(); }  // 指示源地址
        virtual A destination() const = 0;   // 指示目标地址

        // 可选：指定路由路径时使用
        // 使得指示目的地的指针在路由路径列表中向后移动一位，如果成功返回true，
        // 否则，如不存在路由路径或者已经达到最终目的地，返回false
        virtual bool next_route() const { return false; }

        virtual ~postable() = default;
    };


    // 实现此接口的类可以接收 postable 或其子类
    // 用于在多级路由中抹除层级差异
    // 模板参数为地址类型
    template<framework::hash_key A>
    class collector
    {
    public:
        virtual unsigned int post(const std::shared_ptr<const postable<A>>& pkg) = 0;    

        virtual ~collector() = default;
    };

    
}