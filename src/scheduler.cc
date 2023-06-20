//
// Created by 帅帅的涛 on 2023/6/20.
//

#include "scheduler.h"


namespace dht{
static dht::Logger::ptr g_logger = DHT_LOG_NAME("system");

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name) {

}

Scheduler::~Scheduler() {

}

Scheduler *Scheduler::GetThis() {

}

Fiber *Scheduler::GetMainFiber() {
    return nullptr;
}

void Scheduler::start() {

}

void Scheduler::stop() {

}


}