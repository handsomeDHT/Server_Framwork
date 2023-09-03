//
// Created by 帅帅的涛 on 2023/9/3.
//
#include "daemon.h"
#include "src/log.h"
#include "src/config.h"
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace dht {

static dht::Logger::ptr g_logger = DHT_LOG_NAME("system");
static dht::ConfigVar<uint32_t>::ptr g_daemon_restart_interval
        = dht::Config::Lookup("daemon.restart_interval", (uint32_t)5, "daemon restart interval");

std::string ProcessInfo::toString() const {
    std::stringstream ss;
    ss << "[ProcessInfo parent_id=" << parent_id
       << " main_id=" << main_id
       << " parent_start_time=" << dht::Time2Str(parent_start_time)
       << " main_start_time=" << dht::Time2Str(main_start_time)
       << " restart_count=" << restart_count << "]";
    return ss.str();
}

static int real_start(int argc, char** argv,
                      std::function<int(int argc, char** argv)> main_cb) {
    return main_cb(argc, argv);
}

static int real_daemon(int argc, char** argv,
                       std::function<int(int argc, char** argv)> main_cb) {

    /**
     * 将当前进程变成守护进程
     * int daemon (int nochdir, int noclose);
     * @param[nochdir] 如果 nochdir 参数为非零值，它将防止 daemon 函数改变当前工作目录为根目录 ("/")。
     *                 如果为零，daemon 函数会将当前工作目录改变为根目录。
     * @param[noclose] 如果 noclose 参数为非零值，它将防止 daemon 函数关闭标准输入（stdin）、标准输出（stdout）和标准错误（stderr）。
     *                 如果为零，daemon 函数会关闭这些文件描述符，以确保它们不会在后续的操作中影响守护进程。
     */

    daemon(1, 0);
    // 设置父进程的信息，包括ID和启动时间
    ProcessInfoMgr::GetInstance()->parent_id = getpid();
    ProcessInfoMgr::GetInstance()->parent_start_time = time(0);
    while(true) {
        // 创建子进程
        pid_t pid = fork();
        if(pid == 0) {
            // 子进程执行以下代码
            ProcessInfoMgr::GetInstance()->main_id = getpid();
            ProcessInfoMgr::GetInstance()->main_start_time  = time(0);
            // 打印子进程的启动信息
            DHT_LOG_INFO(g_logger) << "process start pid=" << getpid();
            return real_start(argc, argv, main_cb);
        } else if(pid < 0) {    // fork() 失败，打印错误信息
            DHT_LOG_ERROR(g_logger) << "fork fail return=" << pid
                                      << " errno=" << errno << " errstr=" << strerror(errno);
            return -1;
        } else {
            // 父进程执行以下代码
            // 等待子进程退出，并获取其退出状态
            int status = 0;
            waitpid(pid, &status, 0);
            if(status) {
                // 子进程异常退出，打印错误信息
                DHT_LOG_ERROR(g_logger) << "child crash pid=" << pid
                                          << " status=" << status;
            } else {
                // 子进程正常退出，打印信息
                DHT_LOG_INFO(g_logger) << "child finished pid=" << pid;
                break;
            }
            // 增加重启计数，并等待一段时间后重新启动子进程
            ProcessInfoMgr::GetInstance()->restart_count += 1;
            sleep(g_daemon_restart_interval->getValue());
        }
    }
    return 0;
}

int start_daemon(int argc, char** argv
        , std::function<int(int argc, char** argv)> main_cb
        , bool is_daemon) {
    if(!is_daemon) {
        return real_start(argc, argv, main_cb);
    }
    return real_daemon(argc, argv, main_cb);
}

}
