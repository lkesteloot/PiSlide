
#pragma once

#include <thread>
#include <functional>
#include <memory>

#include "tsqueue.h"

/**
 * Passes tasks to a set of threads and makes the results available.
 */
template <typename REQUEST, typename RESPONSE>
class Executor {
    // The function to call for each request.
    std::function<RESPONSE(REQUEST)> mRun;
    // Threads in the pool.
    std::vector<std::unique_ptr<std::thread>> mThreads;
    // Requests, or (if optional has no data), threads should terminate.
    ThreadSafeQueue<std::optional<REQUEST>> mRequestQueue;
    // Responses.
    ThreadSafeQueue<RESPONSE> mResponseQueue;

    /**
     * Top-level thread function.
     */
    void loop() {
        while (true) {
            std::optional<REQUEST> request = mRequestQueue.dequeue();
            if (!request.has_value()) {
                // Terminate thread.
                return;
            }
            try {
                RESPONSE response = mRun(*request);
                mResponseQueue.enqueue(response);
            } catch (std::exception const &e) {
                std::cerr << "Executor run function threw exception: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Executor run function threw exception" << std::endl;
            }
        }
    }

public:
    Executor(int threadCount, std::function<RESPONSE(REQUEST)> run): mRun(run) {
        for (int i = 0; i < threadCount; i++) {
            mThreads.emplace_back(std::make_unique<std::thread>(&Executor::loop, this));
        }
    }

    virtual ~Executor() {
        // This "clear" assumes that the task has no permanent value (e.g., modifying
        // database, saving a file, sending an email). Might want to have a flag for
        // whether to do this.
        mRequestQueue.clear();

        // Enqueue requests to terminate the thread.
        for (int i = 0; i < mThreads.size(); i++) {
            mRequestQueue.enqueue(std::optional<REQUEST>());
        }

        // Wait for all of them to finish.
        for (auto &t : mThreads) {
            t->join();
        }
    }

    /**
     * Submit the request for processing.
     */
    void ask(REQUEST request) {
        mRequestQueue.enqueue(std::make_optional(request));
    }

    /**
     * Get any available response, if any.
     */
    std::optional<RESPONSE> get() {
        return mResponseQueue.try_dequeue2();
    }
};
