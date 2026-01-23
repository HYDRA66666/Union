#pragma once
#include "pch.h"
#include "framework.h"

#include "lib_exceptions.h"

#include "utilities.h"

namespace HYDRA15::Union::expressman
{
    /**************************** 基 础 ****************************/
    // 用户传递信息、打包数据的包裹结构
    // 被设计为平凡类型，并且所有有效数据集中在内存前部
    // 标准 parcel 不携带数据，用于消息传递及接口传参。需要携带数据的可使用扩展 parcel
    struct parcel
    {
        static constexpr size_t maxInfoSize = 116;

        uint16_t frameNo = 0;           // 帧号
        uint16_t frameCount = 0;        // 总帧数
        uint32_t id = 0;                // 对象id
        uint16_t serialNo = 0;          // 对象序列号
        uint16_t dataLength = 0;        // 数据长度，如果不是 parcel_ext 类型则此字段置 0
        char info[maxInfoSize] = {};

        static std::unique_ptr<parcel, std::function<void(parcel*)>> make_unique()
        {
            auto deleter = [](parcel* p) {delete p; };
            return std::unique_ptr<parcel, std::function<void(parcel*)>>{new parcel{}, deleter};
        }

    private:
        parcel() = default;
    };

    template<size_t extDatSize>
    struct parcel_ext
    {
        byte data[extDatSize] = {};

        static std::unique_ptr<parcel, std::function<void(parcel*)>> make_unique()
        {
            auto deleter = [](parcel* p) {delete static_cast<parcel_ext<extDatSize>*>(p); };
            return std::unique_ptr<parcel, std::function<void(parcel*)>>{new parcel_ext<extDatSize>{}, deleter};
        }

    private:
        parcel_ext() = default;
    };

    using parcel_ptr = std::unique_ptr<parcel, std::function<void(parcel*)>>;

    static_assert(std::is_trivially_copyable_v<parcel>, "trivially cpoy for HYDRA15::Union::expressman::parcel not supported.");
    static_assert(std::is_standard_layout_v<parcel>, "standard layout for HYDRA15::Union::expressman::parcel not supported.");
    static_assert(sizeof(parcel) == 128, "size check for HYDRA15::Union::expressman::parcel failed.");


    /**************************** 信 件 ****************************/

    // 实现此接口的类可以在类型擦除状态下被克隆
    // 某些多播场景下可能需要使用此接口
    class clonable
    {
    public:
        virtual ~clonable() = default;

        virtual std::unique_ptr<clonable> clone() const = 0;            // 基于此对象复制构造新对象
        virtual constexpr const std::type_info& type() const = 0;       // 返回此对象类型信息

        // 扩展：返回此对象的描述字符串
        virtual std::string info() const { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::expressman::clonable", "", "info"); }
    };

    // 实现此接口的类可以被打包和解包
    // 需要存储、传输对象需要实现此接口
    // 子类只需实现对象到裸字节数据的转换接口即可，打包算法由系统实现。
    class packable
    {
    public:
        virtual ~packable() = default;

        virtual uint16_t serial_no() const = 0;    // 返回此对象的序列号，用于区分同一类型的不同对象

        template<size_t maxParcelDataSize>
        std::deque<parcel_ptr> pack(uint32_t id)
        {
            std::vector<byte> data = packing();

            // 检查合法：data 中的数据是否能全部放入多个帧中
            if (data.size() > maxParcelDataSize * std::numeric_limits<uint16_t>::max())
                throw exceptions::common("Object is too large to pack");

            // 分包封装并返回
            std::deque<parcel_ptr> res;
            uint16_t totalFrames = static_cast<uint16_t>((data.size() + maxParcelDataSize - 1) / maxParcelDataSize);
            uint16_t frameNo = 0;
            uint16_t serialNo = serial_no();
            while (!data.empty())
            {
                size_t chunkSize = std::min(data.size(), maxParcelDataSize);
                auto p = parcel_ext<maxParcelDataSize>::make_unique();
                p->frameNo = frameNo++;
                p->frameCount = totalFrames;
                p->id = id;
                p->serialNo = serialNo;
                p->dataLength = static_cast<uint16_t>(chunkSize);
                assistant::memcpy(data.data() + frameNo * maxParcelDataSize, p->data, chunkSize);
                res.push_back(std::move(p));
            }

            return res;
        }

        template<size_t maxParcelDataSize>
        void unpack(std::deque<parcel_ptr> data)
        {
            // 检查 data 中的数据是否完整，需要全部帧的 id 和 serialNo 相同 且 帧号没有空缺
            size_t dataSize = 0;
            {
                exceptions::common expt("Incomplete parcel frames for unpacking");

                if (data.empty()) return;

                uint16_t frameCount = data.front()->frameCount;
                uint32_t id = data.front()->id;
                uint16_t serialNo = data.front()->serialNo;
                std::vector<bool> frameReceived(frameCount, false);

                for (auto it = data.begin(); it != data.end();)
                {
                    if ((*it)->id != id || (*it)->serialNo != serialNo || (*it)->frameCount != frameCount) throw expt;
                    if ((*it)->frameNo > frameCount) throw expt;
                    if (frameReceived[(*it)->frameNo]) 
                    {
                        it = data.erase(it); 
                        continue;
                    }
                    frameReceived[(*it)->frameNo] = true;
                    dataSize += (*it)->dataLength;
                    it++;
                }

                if (data.size() != frameCount) throw expt;
            }

            // 按照帧号排序
            std::sort(data.begin(), data.end(), [](const parcel_ptr& a, const parcel_ptr& b) {
                return a->frameNo < b->frameNo;
                });

            // 解包并还原对象
            std::vector<byte> res(dataSize, 0);
            size_t offset = 0;
            for (const auto& p : data)
            {
                assistant::memcpy(p->data, res.data() + offset, p->dataLength);
                offset += p->dataLength;
            }

            unpacking(res);
        }

    protected:
        virtual std::vector<byte> packing() = 0;

        virtual void unpacking(const std::vector<byte>& data);

    };

    // 实现此接口的类可以按照地址信息被路由、传递
    // 其模板参数为地址类型
    template<typename A>
    class postable
    {
    public:
        virtual ~postable() = default;

        virtual const A& destination() const = 0;   // 返回此对象需要到达的目的地，或者指定路由路径时需要到达的下一跳

        // 扩展：返回此对象的事发地地址
        virtual const A& origin() const { throw exceptions::common::UnsupportedInterface("HYDRA15::Union::expressman::clonable", "", "info"); }
        // 扩展：在指定路由路径时使用，更新下一跳地址，并返回更新后的结果。
        virtual const A& move_forward() const { return destination(); }
    };


    /**************************** 站 点 ****************************/

    // 支持接收信件的站点
    // 信件传递链路中的基础结构，可能是 路由组件 存储组件 远程传输组件 收信站点 等等
    // 模板参数为地址类型
    template<typename A>
    class receiver
    {
    public:
        virtual ~receiver() = default;

        virtual void post(std::unique_ptr<postable<A>> mail) = 0;  // 投递信件，如果任何一个节点投递失败则抛出异常。应当是同步操作，需要异步的由调用者负责
        virtual bool is_open() const = 0;                       // 检查此站点是否打开，可以接收信件
    };

    // 支持广播信件的站点
    // 在 receptive 基础上增加订阅注册相关功能
    template<typename A>
    class dispatcher :public virtual receiver<A>
    {
    public:
        virtual ~dispatcher() = default;

        virtual void subscribe(const A& address, const std::function<void(std::unique_ptr<postable<A>>&)> listener) = 0;   // 在指定地址订阅消息并注册监听函数
        virtual void unsubscribe(const A& address) = 0;                                                                 // 取消订阅
    };

    // 用于将数据包还原为对象的解包器类
    class unpacker
    {
    public:
        virtual ~unpacker() = default;

        virtual std::unique_ptr<packable> build_object(std::deque<parcel_ptr> data) = 0;
        virtual void register_builder(const std::string& className, const std::function<std::unique_ptr<packable>(std::deque<parcel_ptr>)>& builder) = 0;
    };

    // 将数据包发送或存储 或 其反向操作的中转站
    // 在创建时绑定发送或存储的目标对象，接口中不再体现此信息
    class relay
    {
    public:
        virtual ~relay() = default;

        virtual bool send(const std::deque<parcel_ptr>& data) = 0;  // 发送操作，返回是否发送成功
        virtual std::deque<parcel_ptr> pick() = 0;                  // 接收操作，返回缓冲区中的数据，如果缓冲区中没有数据则返回空 deque
    };
}