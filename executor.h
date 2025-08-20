
#pragma once

#include <iostream>
#include <thread>
#include <functional>
#include <memory>

#include "tsqueue.h"

/**
 * Passes tasks to a set of threads and makes the results available.
 */
template <typename REQUEST, typename RESPONSE>
class Executor final {
    // The function to call for each request.
    std::function<RESPONSE(REQUEST const &)> mRun;
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
                std::cerr << "Executor run function threw exception: " << e.what() << '\n';
            } catch (...) {
                std::cerr << "Executor run function threw exception" << '\n';
            }
        }
    }

public:
    Executor(int threadCount, std::function<RESPONSE(REQUEST const &)> run): mRun(std::move(run)) {
        for (int i = 0; i < threadCount; i++) {
            mThreads.emplace_back(std::make_unique<std::thread>(&Executor::loop, this));
        }
    }

    ~Executor() {
        // This "clear" assumes that the task has no permanent value (e.g., modifying
        // database, saving a file, sending an email). Might want to have a flag for
        // whether to do this.
        mRequestQueue.clear();

        // Enqueue requests to terminate the thread.
        for (unsigned int i = 0; i < mThreads.size(); i++) {
            mRequestQueue.enqueue(std::optional<REQUEST>());
        }

        // Wait for all of them to finish.
        for (auto &t : mThreads) {
            t->join();
        }
    }

    // Can't copy.
    Executor(const Executor &) = delete;
    Executor &operator=(const Executor &) = delete;

    /**
     * Submit the request for processing.
     */
    void ask(REQUEST const &request) {
        mRequestQueue.enqueue(std::make_optional(request));
    }

    /**
     * Clear the request queue. Call this before ask() if you want at most
     * one queued request at a time.
     */
    void clearRequestQueue() {
        mRequestQueue.clear();
    }

    /**
     * Get a response, if any.
     */
    std::optional<RESPONSE> get() {
        return mResponseQueue.try_dequeue();
    }

    /**
     * Drain the response queue and return the most recent response, if any.
     */
    std::optional<RESPONSE> getMostRecent() {
        std::optional<RESPONSE> response;

        while (true) {
            std::optional<RESPONSE> tryResponse = get();
            if (tryResponse.has_value()) {
                response = tryResponse;
            } else {
                break;
            }
        }

        return response;
    }
};
