//
// Created by 帅帅的涛 on 2023/9/5.
//
#include "src/application.h"

int main(int argc, char** argv) {
    dht::Application app;
    if(app.init(argc, argv)) {
        return app.run();
    }
    return 0;
}