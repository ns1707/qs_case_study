#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>
#include <atomic>

using namespace std::chrono_literals;

/// @brief  Starts a thread that runs the given task repeatedly until it returns true or the max timeout is reached.
/// @param[in,out] stop_requested A reference to an atomic boolean that can be used to request the thread to stop.
/// @param[in] task A function that represents the task to be executed. It should return true when the task is complete.
/// @param[in] maxTimeout The maximum duration in seconds for which the thread should run the task.
/// @return A std::thread object representing the started thread.
inline std::thread startRun(std::atomic<bool>& stop_requested,
                            std::function<bool(void)> task,
                            std::chrono::seconds maxTimeout)
{
  return std::thread([&stop_requested, task = std::move(task), maxTimeout]() {
    auto start_time = std::chrono::high_resolution_clock::now();

    while (!stop_requested)
    {
      bool task_completed = task();

      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::high_resolution_clock::now() - start_time);

      if (task_completed || elapsed >= maxTimeout)
      {
        stop_requested = true;
        break;
      }
    }
  });
}

inline void execTaskThreads() {
  std::atomic<bool> stop_requested{false};
  int c1{0}, c2{0};

  auto t1 = startRun(stop_requested, 
                     [&c1]() {
                      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                      c1++;
                      return false;
                    },
                    10s);

  auto t2 = startRun(stop_requested,
                     [&c2]() {
                      if (c2 < 5) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        c2++;
                        return false;
                      }
                      return true;
                    },
                    10s);

  if (t1.joinable()) t1.join();
  if (t2.joinable()) t2.join();

  std::cout << "Worker threads have finished execution." << std::endl;
  std::cout << "c1: " << c1 << ", c2: " << c2 << std::endl;
}
