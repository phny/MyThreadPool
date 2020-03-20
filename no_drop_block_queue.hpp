#ifndef COMMON_UTILS_NO_DROP_BLOCK_QUEUE_HPP_
#define COMMON_UTILS_NO_DROP_BLOCK_QUEUE_HPP_

#include <condition_variable>  // NOLINT
#include <mutex>               // NOLINT
#include <queue>

#include <iostream>


/// @brief 阻塞多列，支持多线程
// A usual work pattern looks like this: one or multiple producers push jobs
// into this queue, and one or multiple workers pops jobs from this queue. If
// nothing is in the queue but NoMoreJobs() is not called yet, the pop calls
// will wait. If NoMoreJobs() has been called, pop calls will return false,
// which serves as a message to the workers that they should exit.
template <typename T>
class NoDropBlockQueue {
 public:
  /// @brief 构造函数
  NoDropBlockQueue(int capacity) : no_more_jobs_(false), capacity_(capacity) {}
  /// @brief 构造函数
  NoDropBlockQueue(const NoDropBlockQueue& src) = delete;

  /// @brief 从队列中获取值
  /// Pops a value and writes it to the value pointer. If there is nothing in
  /// the queue, this will wait till a value is inserted to the queue. If there
  /// are no more jobs to pop, the function returns false. Otherwise, it returns
  /// true.
  bool Pop(T* value) {
    std::unique_lock<std::mutex> mutex_lock(mutex_);
    while (queue_.size() == 0 && !no_more_jobs_) {
      not_empty_.wait(mutex_lock);
    }
    if (queue_.size() == 0 && no_more_jobs_) {
      return false;
    }
    *value = queue_.front();
    queue_.pop();
    not_full_.notify_one();
    return true;
  }

  /// @brief 获取队列头的值，不修改队列
  bool Front(T* value) {
    std::unique_lock<std::mutex> mutex_lock(mutex_);
    while (queue_.size() == 0 && !no_more_jobs_) {
      not_empty_.wait(mutex_lock);
    }
    if (queue_.size() == 0 && no_more_jobs_) {
      return false;
    }
    *value = queue_.front();
    return true;
  }

  /// @brief 获取队列里面的元素数量
  int Size() {
    std::unique_lock<std::mutex> mutex_lock(mutex_);
    return queue_.size();
  }

  /// @brief 向队列中添加一个值
  void Push(const T& value) {
    {
      std::unique_lock<std::mutex> mutex_lock(mutex_);
      if (no_more_jobs_) {
        std::cout << "Cannot push to a closed queue." << std::endl;
        exit(-1);
      }
      // if (queue_.size() == capacity_) {
      //   queue_.pop();
      // }
      while (queue_.size() == capacity_) {
        not_full_.wait(mutex_lock);
      }
      queue_.push(value);
    }
    not_empty_.notify_one();
  }

  /// @brief 设置队列不再有新的任务进来
  // NoMoreJobs() marks the close of this queue. It also notifies all waiting
  // Pop() calls so that they either check out remaining jobs, or return false.
  // After NoMoreJobs() is called, this queue is considered closed - no more
  // Push() functions are allowed, and once existing items are all checked out
  // by the Pop() functions, any more Pop() function will immediately return
  // false with nothing set to the value.
  void NoMoreJobs() {
    {
      std::unique_lock<std::mutex> mutex_lock(mutex_);
      no_more_jobs_ = true;
    }
    not_empty_.notify_all();
    not_full_.notify_all();
  }

  /// @brief 重新启用队列
  void Reset() {
    {
      std::unique_lock<std::mutex> mutex_lock(mutex_);
      if (queue_.size() > 0) {
        std::cout << "Cannot reset a non-empty queue." << std::endl;
        exit(-1);
      }
      no_more_jobs_ = false;
    }
  }

 private:
  std::mutex mutex_;                   ///< 队列线程锁
  std::condition_variable not_empty_;  ///< 队列非空的信号量
  std::condition_variable not_full_;   ///< 队列未满的信号量
  std::queue<T> queue_;                ///< 存放元素的队列
  bool no_more_jobs_;                  ///< 标记是否不会再往队列添加元素
  int capacity_;                       ///< 队列的容量
};

#endif
