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
        virtual std::string info() const
        {
            throw exceptions::expressman::InterfaceExtensionFunctionNotImplemented();
        }

        virtual size_t class_size() const    // 获取类本身的大小，不包含对象管理的外部数据，基本上就是 size_of(class)
        {
            throw exceptions::expressman::InterfaceExtensionFunctionNotImplemented();
        }
    };

    /***************************** 数据传输相关 *****************************/
// 交换数据用的帧格式
// 一帧大小为 4KB ，其中 96B 头，4000B数据
// 数据区集中排布在末尾，因此可以放弃传输尾部没有有效数据的部分
    struct packet
    {
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
        using datablock = std::vector<byte>;
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

        std::list<packet> pack(size_t maxFrameSize = sizeof(packet)) const
        {
            if (maxFrameSize < sizeof(packet::header_t))
                throw exceptions::expressman::InterfaceInvalidFrameSize();

            size_t maxFrameDataSize = std::min(maxFrameSize - sizeof(packet::header_t), packet::maxFrameDataSize);    // 根据给出帧大小限定计算数据区大小限制
            std::list<packet> res;
            for (const auto& datas : packing())
            {
                packet format; // 每对象生成的数据包组拥有相同的部分头结构
                size_t remaining = datas.size();    // 剩余未打包的数据量
                if (remaining > maxFrameDataSize * std::numeric_limits<packet::uint>::max())
                    throw exceptions::expressman::InterfaceDataTooLarge();

                // 设置头的通用部分
                assistant::memcpy(class_name_pack().data(), format.header.className, packet::maxClassNameSize);    // 类名
                format.header.frameTotal = remaining > 0 ? static_cast<packet::uint>((remaining - 1) / packet::maxFrameDataSize + 1) : static_cast<packet::uint>(0); // 总包数
                format.header.serialNo = serialNo_fetch_and_increase(); // 序列号

                packet::uint frameNo = 0;
                do
                {
                    // 在list中创建新的帧
                    res.push_back(format);
                    packet& f = res.back();
                    // 设置头
                    f.header.frameNo = frameNo;
                    f.header.dataLength = static_cast<packet::uint>(std::min(remaining, maxFrameDataSize));
                    // 拷贝数据
                    assistant::memcpy(datas.data() + frameNo * maxFrameDataSize, f.data, f.header.dataLength);
                    // 收尾工作
                    remaining -= f.header.dataLength;
                    frameNo++;
                } while (remaining > 0);
            }

            return res;
        }

        std::any unpack(const std::list<packet>& archs)
        {
            // 在所有帧都按顺序排列的情况下，算法复杂度为 O(n)
        // 帧不按顺序排序时，算法的复杂度最大可达 O(n^2)
            datablocks dbs;
            std::list<packet> archives = archs;    // 由于要对包列表进行操作，所以将参数拷贝
            while (!archives.empty())
            {
                datablock db;
                //  使用 frameTotal 和 serialNo 两个字段的匹配来识别对象
                uint16_t total = archives.front().header.frameTotal;
                uint16_t serialNo = archives.front().header.serialNo;
                db.reserve(total * packet::maxFrameDataSize);

                for (uint16_t i = 0; i < total; i++)
                {
                    auto pFrame = archives.end();
                    for (auto f = archives.begin(); f != archives.end(); f++)
                        if (f->header.frameTotal == total && f->header.serialNo == serialNo && f->header.frameNo == i)  // 寻找两个字段匹配且包号为所需的包
                        {
                            pFrame = f;
                            break;
                        }
                    if (pFrame == archives.end())   // 未找到匹配的包，表明数据不完整
                        throw exceptions::expressman::InterfaceIncompleteData();
                    db.assign(pFrame->data, (pFrame->data) + (pFrame->header.dataLength));
                    archives.erase(pFrame);
                }

                dbs.push_back(db);
            }

            return unpacking(dbs);
        }

        // 扩展接口
        virtual size_t obj_size() const    // 返回对象必需数据的总大小，为对象大小和被对象管理的数据大小之和
        {
            throw exceptions::expressman::InterfaceExtensionFunctionNotImplemented();
        }
    };

    packable::objects unpack(const std::list<packet>& archs, std::function<packable::objects(const packable::datablocks&)> unpacking)
    {
        // 在所有帧都按顺序排列的情况下，算法复杂度为 O(n)
        // 帧不按顺序排序时，算法的复杂度最大可达 O(n^2)
        packable::datablocks dbs;
        std::list<packet> archives = archs;    // 由于要对包列表进行操作，所以将参数拷贝
        while (!archives.empty())
        {
            packable::datablock db;
            //  使用 frameTotal 和 serialNo 两个字段的匹配来识别对象
            uint16_t total = archives.front().header.frameTotal;
            uint16_t serialNo = archives.front().header.serialNo;
            db.reserve(total * packet::maxFrameDataSize);

            for (uint16_t i = 0; i < total; i++)
            {
                auto pFrame = archives.end();
                for (auto f = archives.begin(); f != archives.end(); f++)
                    if (f->header.frameTotal == total && f->header.serialNo == serialNo && f->header.frameNo == i)  // 寻找两个字段匹配且包号为所需的包
                    {
                        pFrame = f;
                        break;
                    }
                if (pFrame == archives.end())   // 未找到匹配的包，表明数据不完整
                    throw exceptions::expressman::InterfaceIncompleteData();
                db.assign(pFrame->data, (pFrame->data) + (pFrame->header.dataLength));
                archives.erase(pFrame);
            }

            dbs.push_back(db);
        }

        return unpacking(dbs);
    }

    std::string extract_name(const packet& arch) { return std::string(arch.header.className, arch.maxClassNameSize); }


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