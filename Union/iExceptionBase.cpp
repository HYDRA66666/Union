#include "pch.h"
#include "iExceptionBase.h"


namespace HYDRA15::Union::referee
{
	iExceptionBase::iExceptionBase(const std::string& desp, const iException_code& id, const iException_code& code) noexcept
        :description(desp), libID(id), exptCode(code)
#ifdef LIB_IEXPT_STACKTRACE_ENABLE
		, stackTrace(std::stacktrace::current())
#endif // LIB_IEXPT_STACKTRACE_ENABLE
	{

	}

	const char* iExceptionBase::what() const noexcept
	{
        if (whatStr.empty())
            whatStr = std::format(
                baseWhatStrFormat,
                description,
                libID,
                exptCode
            );

		return whatStr.c_str();
	}

#ifdef LIB_IEXPT_STACKTRACE_ENABLE
    const char* iExceptionBase::stack_trace() const
    {
        if (stackTraceStr.empty())
            stackTraceStr = std::format(
                baseStackTraceFormat,
                stackTrace
            );

        return stackTraceStr.c_str();
    }
#endif // LIB_IEXPT_STACKTRACE_ENABLE

}
