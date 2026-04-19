#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <comm_types.hpp>

namespace comm {

// brief: SendScheduler class manages scheduling of non-blocking and periodic sends for a given communication type.
//        It runs a worker thread that executes scheduled send tasks at the appropriate times, handles task cancellation, 
//        and tracks the result of the last send operation.
template <typename CommunicationT>
class SendScheduler
{
public:
    using TaskId = std::uint64_t;

    // Constructor takes a reference to the communication object to use for sending data.
    explicit SendScheduler(CommunicationT& communication)
        : m_communication(communication),
          m_stop(false),
          m_nextTaskId(1),
          m_nextSequence(0),
          m_lastResult(OperationResult{OutputType::ok, CommunicationError::none}) {}

    // Destructor ensures the worker thread is stopped and resources are cleaned up.
    ~SendScheduler()
    {
      stop();
    }

    // Delete copy constructor to prevent copying of the scheduler.
    SendScheduler(const SendScheduler&) = delete;

    // Delete assignment operator to prevent copying of the scheduler.
    SendScheduler& operator=(const SendScheduler&) = delete;

    // Start the worker thread for handling scheduled tasks.
    OperationResult start()
    {
      std::lock_guard<std::mutex> lock(m_mutex);  
      if (m_worker.joinable())
      {
        return OperationResult{OutputType::ok,
                               CommunicationError::none};
      }  
      m_stop = false;
      m_worker = std::thread(&SendScheduler::workerLoop, this);
      return OperationResult{OutputType::ok, CommunicationError::none};
    }

    OperationResult stop()
    {
      {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
      }
      m_cv.notify_all();  
      if (m_worker.joinable())
      {
        m_worker.join();
      }
      return OperationResult{OutputType::ok, CommunicationError::none};
    }

    // Schedule a send operation with a specified delay. The delay is in milliseconds. 
    // Returns an OperationResult indicating success or failure of scheduling.
    OperationResult sendWithDelay(const std::uint8_t* data,
                                  std::size_t size,
                                  std::uint32_t delayMs)
    {
      TaskId ignored = 0;
      return schedule(data, size, delayMs, 0, false, true, ignored);
    }

    // Send data periodically with a specified interval. The initial delay and interval are in milliseconds.
    // Returns an OperationResult indicating success or failure of scheduling, and outputs a TaskId for managing the periodic task.
    OperationResult sendPeriodically(const std::uint8_t* data,
                                     std::size_t size,
                                     std::uint32_t initialDelayMs,
                                     std::uint32_t periodMs,
                                     TaskId& outTaskId)
    {
      return schedule(data, size, initialDelayMs, periodMs, true, false, outTaskId);
    }

    // Cancel a scheduled task by its TaskId. Returns an OperationResult indicating success or failure.
    OperationResult cancel(TaskId taskId)
    {
      std::lock_guard<std::mutex> lock(m_mutex);  
      auto it = m_tasks.find(taskId);
      if (it == m_tasks.end())
      {
        return OperationResult{OutputType::error,
                               CommunicationError::schedulerTaskNotFound};
      }  
      it->second.canceled = true;
      m_cv.notify_all();
      return OperationResult{OutputType::ok, CommunicationError::none};
    }

    // Cancel all scheduled tasks. Returns an OperationResult indicating success.
    OperationResult cancelAll()
    {
      std::lock_guard<std::mutex> lock(m_mutex);  
      for (auto& kv : m_tasks)
      {
        kv.second.canceled = true;
      }  
      m_cv.notify_all();
      return OperationResult{OutputType::ok, CommunicationError::none};
    }

    // Fetch the result of the last send operation. This is useful for checking the outcome of non-blocking and periodic sends.
    OperationResult lastResult() const
    {
      std::lock_guard<std::mutex> lock(m_resultMutex);
      return m_lastResult;
    }

private:
    // Internal structures for managing scheduled tasks and their metadata.
    struct TaskInfo
    {
      bool periodic = false;
      bool canceled = false;
      bool nonBlocking = false;
      std::uint32_t periodMs = 0;
    };

    // Internal structure representing a scheduled send task, including its scheduled time, payload, and identifiers for ordering and cancellation.
    struct Item
    {
      std::chrono::steady_clock::time_point sendAt;
      std::vector<std::uint8_t> payload;
      TaskId taskId = 0;
      std::uint64_t sequence = 0;
    };

    // Comparator for ordering scheduled tasks in the priority queue.
    struct Compare
    {
      bool operator()(const Item& a, const Item& b) const
      {
          if (a.sendAt != b.sendAt) {
              return a.sendAt > b.sendAt;
          }
          return a.sequence > b.sequence;
      }
    };

    // Schedule a send operation with specified parameters. 
    // This is a helper function used by both sendWithDelay and sendPeriodically to add tasks to the scheduler.
    OperationResult schedule(const std::uint8_t* data,
                             std::size_t size,
                             std::uint32_t delayMs,
                             std::uint32_t periodMs,
                             bool periodic,
                             bool nonBlocking,
                             TaskId& outTaskId)
    {
      {
        std::lock_guard<std::mutex> lock(m_mutex);  
        if (m_stop)
        {
            return OperationResult{OutputType::ok,
                                   CommunicationError::none};
        }  
        TaskId taskId = m_nextTaskId++;  
        Item item;
        item.sendAt = std::chrono::steady_clock::now() +
                      std::chrono::milliseconds(delayMs);
        item.payload.assign(data, data + size);
        item.taskId = taskId;
        item.sequence = m_nextSequence++;  
        m_tasks[taskId] = TaskInfo{periodic, false, nonBlocking, periodMs};
        m_queue.push(std::move(item));
        outTaskId = taskId;
      }  
      m_cv.notify_all();  
      return OperationResult{OutputType::ok,
                             CommunicationError::none};
    }

    // Worker loop that runs in a separate thread to manage the execution of scheduled send tasks. 
    // It waits for tasks to become due, executes them, and handles rescheduling for periodic tasks.
    void workerLoop()
    {
      std::unique_lock<std::mutex> lock(m_mutex);
  
      for (;;) {
          if (m_stop)
          {
            return;
          }
  
          if (m_queue.empty())
          {
            m_cv.wait(lock, [this]() {
                return m_stop || !m_queue.empty();
            });
            continue;
          }
  
          Item item = m_queue.top();
  
          if (m_tasks.find(item.taskId) == m_tasks.end() || m_tasks[item.taskId].canceled)
          {
            m_queue.pop();
            m_tasks.erase(item.taskId);
            continue;
          }
  
          if (std::chrono::steady_clock::now() < item.sendAt)
          {
            m_cv.wait_until(lock, item.sendAt);
            continue;
          }
  
          m_queue.pop();
          TaskInfo info = m_tasks[item.taskId];
  
          lock.unlock();
          OperationResult result = info.nonBlocking
              ? m_communication.sendNonBlocking(item.payload.data(), item.payload.size())
              : m_communication.send(item.payload.data(), item.payload.size());
          {
            std::lock_guard<std::mutex> resultLock(m_resultMutex);
            m_lastResult = result;
          }
          lock.lock();
  
          auto it = m_tasks.find(item.taskId);
          if (it == m_tasks.end())
          {
            continue;
          }
  
          if (it->second.canceled || m_stop)
          {
            m_tasks.erase(it);
            continue;
          }
  
          if (result.error == CommunicationError::connectionFailed)
          {
            m_tasks.erase(it);
            continue;
          }
  
          if (info.periodic)
          {
            Item nextItem;
            nextItem.sendAt = std::chrono::steady_clock::now() +
                                std::chrono::milliseconds(info.periodMs);
            nextItem.payload = item.payload;
            nextItem.taskId = item.taskId;
            nextItem.sequence = m_nextSequence++;
            m_queue.push(nextItem);
            m_cv.notify_all();
          } 
          else
          {
            m_tasks.erase(it);
          }
      }
    }

private:
    CommunicationT& m_communication; // A reference to the communication object used for sending data.

    // Queue of scheduled tasks ordered by their scheduled send time and sequence number for FIFO ordering of tasks with the same send time.
    std::priority_queue<Item, std::vector<Item>, Compare> m_queue;

    // Tasks map for quick lookup of task information (e.g., periodicity, cancellation status) by TaskId.
    std::map<TaskId, TaskInfo> m_tasks;

    // Mutex and condition variable for synchronizing access to the task queue and managing the worker thread's wait/signal mechanism.
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;

    // Worker thread that executes scheduled tasks and a flag to signal it to stop.
    std::thread m_worker;
    bool m_stop;

    // TaskId generator for assigning unique identifiers to scheduled tasks, and a sequence number for ordering tasks with the same scheduled time.
    TaskId m_nextTaskId;
    std::uint64_t m_nextSequence;

    // Mutex and variable for protecting access to the last operation result.
    mutable std::mutex m_resultMutex;
    OperationResult m_lastResult;
};

} // namespace comm
