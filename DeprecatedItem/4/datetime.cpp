#include "pch.h"
#include "datetime.h"

namespace HYDRA15::Union::assistant
{
    datetime::datetime()
        :stamp(std::time(NULL))
    { }

    datetime::datetime(time_t t)
        :stamp(t)
    { }

    std::string datetime::date_time(std::string format, int timeZone) const
    {
        if(timeZone < -12 || timeZone > 14)
            throw exceptions::assistant::DateTimeInvalidTimeZone();

        time_t localStamp = stamp + timeZone * 3600;
        tm local;
        gmtime_s(&local, &localStamp);
        std::string str;
        str.resize(format.size() * 2 + 20, '\0');
        size_t len = strftime(str.data(), str.size(), format.data(), &local);
        str.resize(len);
        return str;
    }

    datetime datetime::now()
    {
        return datetime();
    }

    std::string datetime::now_date_time(std::string format, int timeZone)
    {
        return datetime().date_time(format, timeZone);
    }
}