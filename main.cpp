/*************************************************************************
	> File Name: main.cpp
	> Author: 
	> Mail: 
	> Created Time: 2020年03月20日 星期五 21时53分47秒
 ************************************************************************/

#include<iostream>

#include "thread_pool.hpp"

using namespace std;


int main(int argc, char* argv[]) {
    std::cout << "ThreadPool test" << std::endl;
    int thread_num = std::thread::hardware_concurrency() - 1;
    ThreadPool *thread_pool = new ThreadPool(thread_num, thread_num);
    std::future<int> future_func = thread_pool->AddFunction([&]()->int{ 
            std::thread::id tid = std::this_thread::get_id();
            std::cout << "current thread id: " << tid << std::endl;
            std::cout << "this is a testing" << std::endl; 
            return 100; 
        });
    int ret = future_func.get();
    std::cout << ret << std::endl;
    std::cout << "finished thread pool test" << std::endl;

    int resource_id = thread_pool->GetNextResourceId();
    std::cout << "resource id: " << resource_id << std::endl;

    std::future<void> future_func2 = thread_pool->AddFunction([&] () {
        std::thread::id tid = std::this_thread::get_id();
        std::cout << "current thread id: " << tid << std::endl;
    });
    future_func2.get();

    return 0;
}
