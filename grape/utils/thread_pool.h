#include <bits/stdc++.h>

class SimpleThreadPool {
 public:
  SimpleThreadPool() : thread_num_(1) {}

  void InitSimpleThreadPool() {
    terminated_ = 0;
    for (uint32_t i = 0; i < thread_num_; ++i) {
      once_run_.push_back(0);
      tasks_.push_back(nullptr);

      threads_.push_back(std::thread(
          [this](uint32_t tid) {
            while (1) {
              std::unique_lock<std::mutex> ul(m);
#ifdef DEBUG
              std::cout << "thread " << tid << " before wait." << std::endl;
#endif
              cv.wait(ul, [this, &tid]() { return once_run_[tid]; });
              if (terminated_) {
                return;
              }
              m.unlock();
              tasks_[tid]();
              m.lock();
              tasks_[tid] = nullptr;
              once_run_[tid] = 0;
              cv.notify_all();
            }
#ifdef DEBUG
            std::cout << "thread " << tid << " terminated." << std::endl;
#endif
          },
          i));
    }
  }

  void StartAllThreads() {
    std::unique_lock<std::mutex> ul(m);
    for (auto& i : once_run_)
      i = 1;
    ul.unlock();
    cv.notify_all();
#ifdef DEBUG
    std::cout << "notify_all" << std::endl;
#endif
  }

  void Terminate() {
    std::unique_lock<std::mutex> ul(m);
    terminated_ = 1;
#ifdef DEBUG
    std::cout << "terminate" << std::endl;
#endif
    ul.unlock();
    StartAllThreads();
    for (uint32_t i = 0; i < thread_num_; ++i)
      threads_[i].join();
  }

  void WaitEnd() {
    std::unique_lock<std::mutex> ul(m);
    cv.wait(ul, [this]() {
      uint32_t running = 0;
      for (auto i : once_run_)
        running += i;
      return running == 0;
    });
#ifdef DEBUG
    std::cout << "all tasks end" << std::endl;
#endif
  }

  ~SimpleThreadPool() { Terminate(); }

  uint32_t thread_num_;
  std::vector<std::function<void()>> tasks_;

 protected:
  std::mutex m;
  std::condition_variable cv;
  int terminated_;
  std::vector<int> once_run_;
  std::vector<std::thread> threads_;
};