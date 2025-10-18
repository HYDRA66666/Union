#pragma once
#include "framework.h"
#include "pch.h"

#include "archivist_exception.h"
#include "utility.h"

namespace HYDRA15::Union::archivist
{
    /***************************** 类型擦除相关 *****************************/
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

    // 可作为比较键，通常用于 std::map
    class comparable : virtual public notype
    {
    protected:
        // 用户仅需实现相同类型的比较逻辑
        virtual bool less_than(std::shared_ptr<comparable> other) const = 0;
        virtual bool greater_than(std::shared_ptr<comparable> other) const = 0;

    public:
        virtual ~comparable() = default;

        // 如果是相同类型，交由用于定义的逻辑比较。如果类型不同，比较 typeid 的 hash 值
        bool operator<(std::shared_ptr<comparable> other) const;
        bool operator>(std::shared_ptr<comparable> other) const;
    };

    // 可作为哈希键，通常用于 std::unordered_map
    class hashable : virtual public notype
    {
    protected:
        virtual ~hashable() = default;

        // 用户仅需实现相同类型的比较逻辑
        virtual bool equal_to(std::shared_ptr<hashable> other) const = 0;

    public:
        // 如果是相同类型，交由用于定义的逻辑比较。如果类型不同，返回false
        bool operator==(std::shared_ptr<hashable> other) const;
        virtual size_t hash() const = 0;
    };

    // 可作为索引，支持排序和快速查找
    class indexable :virtual public hashable, virtual public comparable
    {

    };

    /***************************** 数据交换相关 *****************************/
    // 交换数据用的帧格式
    // 一帧大小为 4KB ，其中 96B 头，4000B数据
    // 数据区集中排布在末尾，因此可以放弃传输尾部没有有效数据的部分
    struct archive
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
        using datablock = std::vector<archive::byte>;
        using datablocks = std::list<datablock>;
        using class_name_array = std::array<char, archive::maxClassNameSize>;
        using objects = std::list<std::shared_ptr<packable>>;
    protected:
        // 用户仅需实现 将数据按照自己的设计打包成连续的二进制数据
        // 将内存数据打包成帧的工作将交由系统完成
        virtual datablocks packing() const = 0;
        virtual std::any unpacking(const datablocks&) = 0;
        virtual constexpr class_name_array class_name_pack() const = 0;    // 返回的类名称将填入className字段，作为类的唯一标识符，在重构对象时使用
        virtual archive::uint serialNo_fetch_and_increase() const = 0;     // 用于标记对象实例的序列号，对于类而言应当是全局静态的

    public:
        virtual ~packable() = default;

        std::list<archive> pack(size_t maxFrameSize = sizeof(archive)) const;
        std::any unpack(const std::list<archive>& archives);

        // 扩展接口
        virtual size_t obj_size() const;    // 返回对象必需数据的总大小，为对象大小和被对象管理的数据大小之和
    };
    packable::objects unpack(const std::list<archive>& archives, std::function<packable::objects(const packable::datablocks&)> constructor);
    std::string extract_name(const archive& arch);

    // 远程数据传输的代理
    // 用于隐藏各种传输方式的底层细节
    class agent
    {
    public:
        virtual bool send(const std::list<archive>& archives) const = 0;
        virtual std::list<archive> recv() const = 0;
        virtual std::list<archive> try_recv() const = 0;

        virtual ~agent() = default;
    };

}

// 对 std 默认 hash 算法提供支持
namespace std
{
    using namespace HYDRA15::Union::archivist;
    template<>
    struct hash<hashable>
    {
        size_t operator()(const hashable& k) const
        {
            return k.hash();
        }
    };
}
