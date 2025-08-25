
#pragma once

#include <memory>
#include <thread>

#include "config.h"
#include "tsqueue.h"

namespace httplib {
    class Server;
}

/**
 * A photo uploaded by the user via the web interface.
 */
struct WebUpload final {
    std::string filename;
    std::string contentType;
    std::string content;
};

/**
 * Object that represents the web server. Dispose of this object to shut
 * the server down.
 */
class WebServer final {
private:
    std::shared_ptr<httplib::Server> mServer;
    std::shared_ptr<std::thread> mThread;

    WebServer(const WebServer &) = delete;
    void operator=(const WebServer &) = delete;
    WebServer &operator=(WebServer &&) = delete;

public:
    WebServer(std::shared_ptr<httplib::Server> server, std::shared_ptr<std::thread> thread) :
        mServer(server), mThread(thread) {}
    ~WebServer();
};

/**
 * Start the web server for uploading photos.
 */
std::unique_ptr<WebServer> startWebServer(Config const &config, ThreadSafeQueue<WebUpload> &queue);

