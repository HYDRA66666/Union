//#include "pch.h"
//#include "Union/logger.h"
//#include "Union/PrintCenter.h"
//#include "Union/ThreadLake.h"
//#include "Union/fstreams.h"
//#include "Union/sfstream.h"
//#include "Union/byteswap.h"
//
//using namespace HYDRA15::Union;
//
//secretary::PrintCenter& pc = secretary::PrintCenter::get_instance();
//
//
//
//int main()
//{
//    //archivist::sfstream sfs = archivist::sfstream::make("demo_archive.sfa", archivist::sfstream::segment_size_level::I);
//    //std::vector<int> dataToWrite(2048, 0);
//    //for (size_t i = 0; i < dataToWrite.size(); i++)
//    //    dataToWrite[i] = static_cast<int>(i);
//    //sfs.write("demo_section", 0, dataToWrite);
//    //sfs.set_sec_comment("demo_section", "This is a demo section.");
//
//    //auto psfs = archivist::sfstream::make_unique("demo_archive.sfa");
//    //auto res = psfs->read<int>("demo_section", 0, 2048);
//    //for (auto& i : res)pc.println(i);
//
//    //std::deque<int> dq{ 1,2,3,4,5 };
//
//    //assistant::byteswap::to_big_endian_range(dq);
//
//    //for(const auto& i : dq)
//    //    pc.printf("{:08X}\n", i);
//
//    //char v[16] = "ArchivistSingle";
//    //std::string_view e{ "ArchivistSingle\0" };
//    //std::string c{ v,16 };
//    //pc.println(c == std::string(e.data(),16));
//}

#include "pch.h"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <string>
#include <vector>
#include <sstream>
#include <random>
#include <algorithm>

#include "Union/sfstream.h"
#include "Union/single_loader.h"
#include "Union/archivist_interfaces.h"

using namespace HYDRA15::Union::archivist;

static std::string field_to_string(const field& f)
{
    std::ostringstream oss;
    struct Visitor {
        std::ostringstream& oss;
        void operator()(const NOTHING&) const { oss << "<NULL>"; }
        void operator()(INT v) const { oss << v; }
        void operator()(FLOAT v) const { oss << v; }
        void operator()(const INTS& v) const {
            oss << "[";
            for (size_t i = 0; i < v.size(); ++i) {
                if (i) oss << ", ";
                oss << v[i];
            }
            oss << "]";
        }
        void operator()(const FLOATS& v) const {
            oss << "[";
            for (size_t i = 0; i < v.size(); ++i) {
                if (i) oss << ", ";
                oss << v[i];
            }
            oss << "]";
        }
        void operator()(const BYTES& v) const {
            oss << "0x";
            std::ios oldState(nullptr);
            oldState.copyfmt(oss);
            oss << std::hex << std::setfill('0');
            for (size_t i = 0; i < v.size(); ++i) {
                oss << std::setw(2) << static_cast<int>(v[i]);
            }
            oss.copyfmt(oldState);
        }
    };
    std::visit(Visitor{ oss }, f);
    return oss.str();
}

static bool equal_fields(const field& a, const field& b)
{
    if (a.index() != b.index()) return false;
    switch (a.index()) {
    case 0:
        return true; // NOTHING / monostate
    case 1:
        return std::get<INT>(a) == std::get<INT>(b);
    case 2:
        return std::get<FLOAT>(a) == std::get<FLOAT>(b);
    case 3:
        return std::get<INTS>(a) == std::get<INTS>(b);
    case 4:
        return std::get<FLOATS>(a) == std::get<FLOATS>(b);
    case 5:
        return std::get<BYTES>(a) == std::get<BYTES>(b);
    default:
        return false;
    }
}

int main()
{
    try {
        const std::filesystem::path dbPath = "test_single_loader.sfa";
        const size_t ROWS = 100;

        // 1) 准备字段表：6 种类型
        field_specs ftab;
        field_spec fs;
        fs.name = "col_nothing"; fs.comment = "nothing"; fs.type = field_spec::field_type::NOTHING; ftab.push_back(fs);
        fs.name = "col_int";     fs.comment = "int64";   fs.type = field_spec::field_type::INT;     ftab.push_back(fs);
        fs.name = "col_float";   fs.comment = "double";   fs.type = field_spec::field_type::FLOAT;   ftab.push_back(fs);
        fs.name = "col_ints";    fs.comment = "ints";     fs.type = field_spec::field_type::INTS;    ftab.push_back(fs);
        fs.name = "col_floats";  fs.comment = "floats";   fs.type = field_spec::field_type::FLOATS;  ftab.push_back(fs);
        fs.name = "col_bytes";   fs.comment = "bytes";    fs.type = field_spec::field_type::BYTES;   ftab.push_back(fs);

        // 如果文件已存在，删掉以保证 create 成功
        if (std::filesystem::exists(dbPath)) {
            std::filesystem::remove(dbPath);
        }

        // 原始数据与索引备份，用于后续比对
        std::vector<std::vector<field>> originalRows;
        originalRows.reserve(ROWS);
        std::vector<ID> originalIndex;
        originalIndex.reserve(ROWS);

        // 构造 100 条不同的数据并记忆
        for (size_t i = 0; i < ROWS; ++i) {
            std::vector<field> row;
            row.reserve(ftab.size());
            // NOTHING
            row.push_back(NOTHING{});
            // INT: 不同的整数
            row.push_back(static_cast<INT>(static_cast<long long>(i) * 100 + 7));
            // FLOAT: 不同的小数
            row.push_back(static_cast<FLOAT>(i * 0.25 + 3.14));
            // INTS: 长度随 i 变化
            INTS vints;
            for (size_t k = 0; k < (i % 5) + 1; ++k) vints.push_back(static_cast<INT>(i + static_cast<int>(k)));
            row.push_back(vints);
            // FLOATS: 长度随 i 变化
            FLOATS vfloats;
            for (size_t k = 0; k < (i % 3) + 1; ++k) vfloats.push_back(static_cast<FLOAT>(i * 0.1 + k * 0.5));
            row.push_back(vfloats);
            // BYTES: 不同字节序列，长度随 i%6 + 1
            BYTES bytes;
            size_t blen = (i % 6) + 1;
            for (size_t b = 0; b < blen; ++b) bytes.push_back(static_cast<BYTE>((i + b) & 0xFF));
            row.push_back(bytes);

            originalRows.push_back(row);

            // 原始索引：使用伪随机但可复现的 sequence（例如线性同余）
            // 保证索引落在 [0, ROWS-1]
            uint64_t idxval = (static_cast<uint64_t>(i) * 1103515245 + 12345) % ROWS;
            originalIndex.push_back(static_cast<ID>(idxval));
        }

        // 2) 创建新的 single_loader 文件并写入 100 条不同数据
        {
            std::cout << "创建并写入文件: " << dbPath.string() << " (" << ROWS << " 行)" << std::endl;
            single_loader sl = single_loader::make(dbPath, ftab);

            page pg{};
            pg.no = 0;
            pg.start = 0;
            pg.count = ROWS;
            // 标记所有行为已修改（写入）
            for (ID i = 0; i < static_cast<ID>(ROWS); ++i) pg.modified.insert(i);

            pg.data.resize(pg.count * ftab.size());

            // 填充 page.data
            for (size_t r = 0; r < ROWS; ++r) {
                for (size_t c = 0; c < ftab.size(); ++c) {
                    pg.data[r * ftab.size() + c] = originalRows[r][c];
                }
            }

            sl.rows(pg);

            // 写入索引 idx_test，使用 originalIndex
            index idx{};
            idx.name = "idx_test";
            idx.comment = "test index for comparison";
            idx.fields.push_back(ftab[1]); // 基于 col_int 示例
            idx.data = originalIndex;

            sl.index_tab(idx);

            // 保存并关闭（析构）
            sl.flush();
            std::cout << "写入完成并已保存。" << std::endl;
        }

        // 3) 重新打开文件，读取全部数据并打印，随后比较读取的数据与写入的是否一致
        {
            std::cout << "重新打开文件并读取数据..." << std::endl;
            single_loader sl = single_loader::make(dbPath);

            ID total = sl.tab_size();
            ID psize = sl.page_size();
            std::cout << "表行数: " << total << ", 页大小: " << psize << std::endl;

            // 读取所有数据并存入 readRows
            std::vector<std::vector<field>> readRows;
            readRows.reserve(total);

            ID pages = (psize == 0) ? 1 : static_cast<ID>((total + psize - 1) / psize);
            for (ID pid = 0; pid < pages; ++pid) {
                page pg = sl.rows(pid);
                for (ID r = 0; r < pg.count; ++r) {
                    std::vector<field> row;
                    row.reserve(ftab.size());
                    for (size_t fi = 0; fi < ftab.size(); ++fi) {
                        row.push_back(pg.data[r * ftab.size() + fi]);
                    }
                    readRows.push_back(row);
                }
            }

            // 打印表头
            std::cout << std::left << std::setw(6) << "Row";
            for (size_t i = 0; i < ftab.size(); ++i) {
                std::cout << " | " << std::left << std::setw(18) << ftab[i].name;
            }
            std::cout << std::endl;
            // 分隔线
            std::cout << std::string(6 + (3 + 18) * ftab.size(), '-') << std::endl;

            // 打印所有行（100 行不会太多）
            for (size_t r = 0; r < readRows.size(); ++r) {
                std::cout << std::left << std::setw(6) << r;
                for (size_t c = 0; c < readRows[r].size(); ++c) {
                    std::string s = field_to_string(readRows[r][c]);
                    if (s.size() > 18) s = s.substr(0, 15) + "...";
                    std::cout << " | " << std::left << std::setw(18) << s;
                }
                std::cout << std::endl;
            }

            // 读取并打印索引
            index ridx = sl.index_tab("idx_test");
            std::cout << "索引 '" << ridx.name << "' count=" << ridx.data.size() << std::endl;

            // 比较读取内容与写入内容
            bool data_ok = true;
            if (readRows.size() != originalRows.size()) {
                data_ok = false;
                std::cout << "行数不匹配：写入 " << originalRows.size() << "，读取 " << readRows.size() << std::endl;
            }
            else {
                for (size_t r = 0; r < originalRows.size() && data_ok; ++r) {
                    for (size_t c = 0; c < originalRows[r].size(); ++c) {
                        if (!equal_fields(originalRows[r][c], readRows[r][c])) {
                            data_ok = false;
                            std::cout << "第 " << r << " 行，第 " << c << " 列不匹配: 写入='"
                                << field_to_string(originalRows[r][c]) << "' 读取='"
                                << field_to_string(readRows[r][c]) << "'" << std::endl;
                            break;
                        }
                    }
                }
            }

            bool idx_ok = (ridx.data == originalIndex);
            if (!idx_ok) {
                std::cout << "索引数据不匹配（部分展示）：" << std::endl;
                size_t show = std::min<size_t>(10, ridx.data.size());
                for (size_t i = 0; i < show; ++i) {
                    std::cout << " idx[" << i << "] read=" << ridx.data[i] << " write=" << originalIndex[i] << std::endl;
                }
            }

            std::cout << "数据比对结果: " << (data_ok ? "一致" : "不一致") << std::endl;
            std::cout << "索引比对结果: " << (idx_ok ? "一致" : "不一致") << std::endl;
        }

        std::cout << "测试完成。" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}