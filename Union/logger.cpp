#include "logger.h"

namespace HYDRA15::Union::secretary
{
    logger::logger(const std::string& t) :title(t) {}

//#define log(type) std::string logger::type(const std::string& content) { return log::type(title, content); }
//    
//    log(info);
//    log(warn);
//    log(error);
//    log(fatal);
//    log(debug);
//    log(trace);
//
//#undef log
}
