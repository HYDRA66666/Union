#include "pch.h"
#include "archivist_interfaces.h"

namespace HYDRA15::Union::archivist
{
    std::string notype::info() const
    {
        throw exceptions::archivist::InterfaceExtensionFunctionNotImplemented();
    }

    size_t notype::class_size() const
    {
        throw exceptions::archivist::InterfaceExtensionFunctionNotImplemented();
    }

    std::list<archive> packable::pack(size_t maxFrameSize) const
    {
        if (maxFrameSize < sizeof(archive::header_t))
            throw exceptions::archivist::InterfaceInvalidFrameSize();

        size_t maxFrameDataSize = std::min(maxFrameSize - sizeof(archive::header_t), archive::maxFrameDataSize);    // 根据给出帧大小限定计算数据区大小限制
        std::list<archive> res;
        for (const auto& datas : packing())
        {
            archive format; // 每对象生成的数据包组拥有相同的部分头结构
            size_t remaining = datas.size();    // 剩余未打包的数据量
            if (remaining > maxFrameDataSize * std::numeric_limits<archive::uint>::max())
                throw exceptions::archivist::InterfaceDataTooLarge();

            // 设置头的通用部分
            assistant::memcpy(class_name_pack().data(), format.header.className, archive::maxClassNameSize);    // 类名
            format.header.frameTotal = remaining > 0 ? static_cast<archive::uint>((remaining - 1) / archive::maxFrameDataSize + 1) : static_cast<archive::uint>(0); // 总包数
            format.header.serialNo = serialNo_fetch_and_increase(); // 序列号

            archive::uint frameNo = 0;
            do
            {
                // 在list中创建新的帧
                res.push_back(format);  
                archive& f = res.back();
                // 设置头
                f.header.frameNo = frameNo; 
                f.header.dataLength = static_cast<archive::uint>(std::min(remaining, maxFrameDataSize));
                // 拷贝数据
                assistant::memcpy(datas.data() + frameNo * maxFrameDataSize, f.data, f.header.dataLength);
                // 收尾工作
                remaining -= f.header.dataLength;
                frameNo++;
            } while (remaining > 0);
        }

        return res;
    }

    std::any packable::unpack(const std::list<archive>& archs)
    {
        // 在所有帧都按顺序排列的情况下，算法复杂度为 O(n)
        // 帧不按顺序排序时，算法的复杂度最大可达 O(n^2)
        datablocks dbs;
        std::list<archive> archives = archs;    // 由于要对包列表进行操作，所以将参数拷贝
        while (!archives.empty())
        {
            datablock db;
            //  使用 frameTotal 和 serialNo 两个字段的匹配来识别对象
            uint16_t total = archives.front().header.frameTotal;    
            uint16_t serialNo = archives.front().header.serialNo;
            db.reserve(total * archive::maxFrameDataSize);

            for (uint16_t i = 0; i < total; i++)
            {
                auto pFrame = archives.end();
                for(auto f = archives.begin(); f!=archives.end();f++)
                    if (f->header.frameTotal == total && f->header.serialNo == serialNo && f->header.frameNo == i)  // 寻找两个字段匹配且包号为所需的包
                    {
                        pFrame = f;
                        break;
                    }
                if (pFrame == archives.end())   // 未找到匹配的包，表明数据不完整
                    throw exceptions::archivist::InterfaceIncompleteData();
                db.assign(pFrame->data, (pFrame->data) + (pFrame->header.dataLength));
                archives.erase(pFrame);
            }
            
            dbs.push_back(db);
        }

        return unpacking(dbs);
    }

    packable::objects unpack(const std::list<archive>& archs, std::function<packable::objects(const packable::datablocks&)> unpacking)
    {
        // 在所有帧都按顺序排列的情况下，算法复杂度为 O(n)
        // 帧不按顺序排序时，算法的复杂度最大可达 O(n^2)
        packable::datablocks dbs;
        std::list<archive> archives = archs;    // 由于要对包列表进行操作，所以将参数拷贝
        while (!archives.empty())
        {
            packable::datablock db;
            //  使用 frameTotal 和 serialNo 两个字段的匹配来识别对象
            uint16_t total = archives.front().header.frameTotal;
            uint16_t serialNo = archives.front().header.serialNo;
            db.reserve(total * archive::maxFrameDataSize);

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
                    throw exceptions::archivist::InterfaceIncompleteData();
                db.assign(pFrame->data, (pFrame->data) + (pFrame->header.dataLength));
                archives.erase(pFrame);
            }

            dbs.push_back(db);
        }

        return unpacking(dbs);
    }

    std::string extract_name(const archive& arch)
    {
        return std::string(arch.header.className, arch.maxClassNameSize);
    }

    size_t packable::obj_size() const
    {
        throw exceptions::archivist::InterfaceExtensionFunctionNotImplemented();
    }

    bool comparable::operator<(std::shared_ptr<comparable> other) const
    {
        if (other == nullptr)
            throw exceptions::archivist::InterfaceIllegalType();
        if (this->type() != other->type())
            return this->type().hash_code() < other->type().hash_code();
        return less_than(other);
    }

    bool comparable::operator>(std::shared_ptr<comparable> other) const
    {
        if (other == nullptr)
            throw exceptions::archivist::InterfaceIllegalType();
        if (this->type() != other->type())
            return this->type().hash_code() > other->type().hash_code();
        return greater_than(other);
    }

    bool hashable::operator==(std::shared_ptr<hashable> other) const
    {
        if (other == nullptr)
            throw exceptions::archivist::InterfaceIllegalType();
        if (this->type() != other->type())
            return false;
        return equal_to(other);
    }


}


