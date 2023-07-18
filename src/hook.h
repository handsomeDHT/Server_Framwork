//
// Created by 帅帅的涛 on 2023/7/18.
//

#ifndef SERVER_FRAMWORK_HOOK_H
#define SERVER_FRAMWORK_HOOK_H

#include <unistd.h>

namespace dht{
    bool is_hook_enable();
    void set_hook_enable(bool flag);
}

extern "C"{
    typedef unsigned int (*sleep_fun)(unsigned int seconds);
    extern sleep_fun sleep_f;

    typedef int (*usleep_fun)(useconds_t usec);
    extern usleep_fun usleep_f;
}


#endif //SERVER_FRAMWORK_HOOK_H
