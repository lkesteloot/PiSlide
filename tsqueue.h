
#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

/**
 * A thread-safe blocking queue.
 */
template <typename T>
class ThreadSafeQueue {
private:
    std::queue<T> mDataQueue;
    std::mutex mMutex;
    std::condition_variable mConditionVariable;

public:
    /**
     * Enqueues the data.
     */
    void enqueue(T data) {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mDataQueue.push(data);
        }
        mConditionVariable.notify_one();
    }

    /**
     * Blocks until the queue has data available, and returns it.
     */
    T dequeue() {
        std::unique_lock<std::mutex> lock(mMutex);
        while (mDataQueue.empty()) {
            mConditionVariable.wait(lock);
        }
        T data = mDataQueue.front();
        mDataQueue.pop();
        return data;
    }

    /**
     * Non-blocking way to dequeue data. Returns whether there was data.
     */
    bool try_dequeue(T& data) {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mDataQueue.empty()) {
            return false;
        }
        data = mDataQueue.front();
        mDataQueue.pop();
        return true;
    }

    /**
     * Non-blocking way to dequeue data.
     */
    std::optional<T> try_dequeue2() {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mDataQueue.empty()) {
            return std::optional<T>();
        }
        T data = mDataQueue.front();
        mDataQueue.pop();
        return std::make_optional(data);
    }

    /**
     * Empty the queue.
     */
    void clear() {
      std::lock_guard<std::mutex> lock(mMutex);
      mDataQueue = {};
    }

    /**
     * Whether the queue is empty.
     */
    bool empty() const {
      std::lock_guard<std::mutex> lock(mMutex);
      return mDataQueue.empty();
    }
};
