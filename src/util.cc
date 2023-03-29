//
// Created by 帅帅的涛 on 2023/3/29.
//

#include "util.h"

namespace dht{

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}
u_int32_t GetFiberId(){
    return 0;
}

}