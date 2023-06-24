//
// Created by 帅帅的涛 on 2023/3/29.
//

#include <execinfo.h>
#include "log.h"
#include "fiber.h"
#include "util.h"

namespace dht{

static dht::Logger::ptr g_logger = DHT_LOG_NAME("system");

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

u_int32_t GetFiberId(){
    return dht::Fiber::GetFiberId();
}

void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    void** array = (void**)malloc((sizeof(void*) * size));
    size_t s = ::backtrace(array, size);

    char** strings = backtrace_symbols(array, s);
    if(strings == NULL) {
        DHT_LOG_ERROR(g_logger) << "backtrace_synbols error";
        return;
    }

    for(size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
}

std::string BacktraceToString(int size, int skip, const std::string& prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for(size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}

}