/** Copyright 2020 Alibaba Group Holding Limited.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef GRAPE_UTILS_THREAD_POOL_H_
#define GRAPE_UTILS_THREAD_POOL_H_

#include <bits/stdc++.h>

class ThreadPool {
 public:
  ThreadPool(ThreadPool const&) = delete;
  ThreadPool& operator=(ThreadPool const&) = delete;
  ThreadPool() {}

  void InitThreadPool(uint32_t max_thread_num) {
    max_thread_num_ = max_thread_num;
    current_thread_num_ =
        max_thread_num_;  // default use all threads to parallel
    tasks_.resize(max_thread_num_);
    for (uint32_t i = 0; i < max_thread_num_; ++i) {
      once_run_.push_back(0);

      threads_.push_back(std::thread(
          [this](uint32_t tid) {
            while (1) {
              std::unique_lock<std::mutex> ul(mtx_);
#ifdef DEBUG
              std::cout << "thread " << tid << " before wait." << std::endl;
#endif
              cv_worker_.wait(ul, [this, &tid]() { return once_run_[tid]; });
              if (terminated_)
                return;
              mtx_.unlock();
              (tasks_[tid])();
              mtx_.lock();
              once_run_[tid] = 0;
#ifdef DEBUG
              std::cout << "thread " << tid << " finished a task." << std::endl;
#endif
              cv_worker_.notify_all();
            }
#ifdef DEBUG
            std::cout << "thread " << tid << " terminated." << std::endl;
#endif
          },
          i));
    }
  }

  void StartAllThreads() {
    std::unique_lock<std::mutex> ul(mtx_);
    for (auto& i : once_run_)
      i = 1;
    ul.unlock();
    cv_worker_.notify_all();
#ifdef DEBUG
    std::cout << "notify_all" << std::endl;
#endif
  }

  void Terminate() {
    std::unique_lock<std::mutex> ul(mtx_);
    terminated_ = 1;
#ifdef DEBUG
    std::cout << "terminate" << std::endl;
#endif
    ul.unlock();
    StartAllThreads();
    for (uint32_t i = 0; i < current_thread_num_; ++i)
      threads_[i].join();
  }

  void WaitEnd() {
    std::unique_lock<std::mutex> ul(mtx_);
    cv_worker_.wait(ul, [this]() {
      uint32_t running = 0;
      for (auto i : once_run_)
        running += i;
      return running == 0;
    });
#ifdef DEBUG
    std::cout << "all tasks end" << std::endl;
#endif
  }

  inline uint32_t GetCurrentThreadNum() const { return current_thread_num_; }

  ~ThreadPool() { Terminate(); }

  void SetTask(uint32_t tid, const std::function<void()>&& f) {
    std::unique_lock<std::mutex> ul(mtx_);
    tasks_[tid] = std::move(f);
  }

 private:
  uint32_t max_thread_num_{1};
  uint32_t current_thread_num_{1};
  std::vector<std::function<void()>> tasks_;
  std::mutex mtx_;
  std::condition_variable cv_worker_;
  std::condition_variable cv_manager_;
  int terminated_{0};
  std::vector<int> once_run_;
  std::vector<std::thread> threads_;
};

#endif  // GRAPE_UTILS_THREAD_POOL_H_
