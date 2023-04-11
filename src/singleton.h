//
// Created by 帅帅的涛 on 2023/3/30.
//

#ifndef SERVER_FRAMWORK_SINGLETON_H
#define SERVER_FRAMWORK_SINGLETON_H

#include <memory>

namespace dht{

template<class T, class X = void, int N = 0>
class Singleton{
public:
    static T* GetInstance(){
        static T v;
        return &v;
    }
};

template<class T, class X = void, int N = 0>
class SingletonPtr{
public:
    static std::shared_ptr<T> GetInstance(){
        static std::shared_ptr<T> v(new T);
        return v;
    }
};
}

#endif //SERVER_FRAMWORK_SINGLETON_H
