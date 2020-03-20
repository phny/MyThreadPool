#ifndef COMMON_UTILS_THREAD_POOL_HPP_
#define COMMON_UTILS_THREAD_POOL_HPP_

#include <cstdint>
#include <future>
#include <thread>
#include <vector>

#include "no_drop_block_queue.hpp"


typedef std::function<void()> Function;

/// @brief 线程池，方便进行多线程处理
class ThreadPool {
 public:
  /// @brief 构造线程池
  ThreadPool(const uint32_t thread_num, const uint32_t queue_capacity) {
    queue_ = new NoDropBlockQueue<Function>(queue_capacity);
    resource_queue_ = new NoDropBlockQueue<uint32_t>(thread_num);
    threads_.resize(thread_num);
    for (uint32_t thread_ind = 0; thread_ind < thread_num; thread_ind++) {
      resource_queue_->Push(thread_ind);
      threads_[thread_ind] = new std::thread([this] {
        Function function;
        while (this->queue_->Pop(&function)) {
          // 执行函数
          function();
        }
      });
    }
  }

  ~ThreadPool() {
    Join();
    delete queue_;
    delete resource_queue_;
  }

  /// @brief 获取下一个可用的资源号
  uint32_t GetNextResourceId() {
    uint32_t resource_ind;
    if (resource_queue_->Front(&resource_ind)) {
      return resource_ind;
    } else {
      std::cout << "[Error] in ThreadPool::GetNextResourceId "
                << "Cannot get next resource id" << std::endl;
      exit(-1);
    }
  }

  /// @brief 添加一个要运行的函数到线程池中
  template <typename Func, typename... Args>
  auto AddFunction(Func &&f, Args &&... args) -> std::future<typename std::result_of<Func(Args...)>::type> {
    // 获取资源号
    uint32_t resource_ind;
    this->resource_queue_->Pop(&resource_ind);

    // 获取该函数的返回值类型
    using return_type = typename std::result_of<Func(Args...)>::type;
    // 生成该函数的运行对象
    auto task = std::make_shared<std::packaged_task<return_type()> >(
        std::bind(std::forward<Func>(f), std::forward<Args>(args)...));
    // 获取该运行对象的结果
    std::future<return_type> result = task->get_future();

    // 将该运行对象添加到运行队列
    queue_->Push([this, task, resource_ind] {
      (*task)();
      // 函数运行完毕才能释放资源号
      this->resource_queue_->Push(resource_ind);
    });

    // 直接返回，外部可以根据返回值 result.get() 来确保函数运行结束
    return result;
  }

  /// @brief 等待线程池的所有操作结束
  void Join() {
    queue_->NoMoreJobs();
    for (auto &thread_ptr : threads_) {
      // 如果线程对象存在则释放
      if (thread_ptr) {
        // 先停止线程
        thread_ptr->join();
        // 删除线程
        delete thread_ptr;
        // 将线程指针置空，防止多次释放线程
        thread_ptr = NULL;
      }
    }
  }

 private:
  std::vector<std::thread *> threads_;  ///< 线程对象
  /// @brief 要运行的函数及其对应的返回值队列
  NoDropBlockQueue<Function> *queue_;
  /// @brief 资源管理队列，用来记录当前哪个资源是空闲状态
  NoDropBlockQueue<uint32_t> *resource_queue_;
};

#endif
