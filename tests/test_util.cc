//
// Created by 帅帅的涛 on 2023/6/7.
//

#include "src/dht.h"
#include <assert.h>

dht::Logger::ptr g_logger = DHT_LOG_ROOT();

void test_assert () {
    DHT_LOG_INFO(g_logger) << dht::BacktraceToString(10, 2, "    ");
    //DHT_ASSERT(false);
    DHT_ASSERT2(0 == 1, "abcde xx");
};

int main(int argc, char** argv){
    test_assert();
    return 0;
}

