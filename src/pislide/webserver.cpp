
#include <httplib.h>
#include <spdlog/spdlog.h>

#include "webserver.h"

namespace {
    /**
     * Maps an HTTP status to a log level.
     */
    spdlog::level::level_enum logLevelForHttpStatus(int status) {
        if (status >= 500) {
            return spdlog::level::err;
        } else if (status >= 400) {
            return spdlog::level::warn;
        } else {
            return spdlog::level::info;
        }
    }
}

WebServer::~WebServer() {
    // Shut down the web server.
    mServer->stop();

    // Wait for its loop to exit.
    mThread->join();
}

std::unique_ptr<WebServer> startWebServer(Config const &config, ThreadSafeQueue<WebUpload> &queue) {
    if (config.webSubdir.empty()) {
        spdlog::info("Web serving is disabled in config");
        return std::unique_ptr<WebServer>();
    }

    std::shared_ptr<httplib::Server> server = std::make_shared<httplib::Server>();

    server->set_logger([](httplib::Request const &req, httplib::Response const &res) {
        spdlog::log(logLevelForHttpStatus(res.status), "Web server: {} {} ({})",
                req.method, req.path, res.status);
    });

    auto success = server->set_mount_point("/", "static");
    if (!success) {
        spdlog::error("The directory for static web files does not exist");
        return std::unique_ptr<WebServer>();
    }

    server->Post("/", [&queue, &config](httplib::Request const &req, httplib::Response &res) {
        // Access uploaded files.
        auto const &files = req.form.get_files("image");
        for (auto const &file : files) {
            spdlog::info("Uploaded file: {} ({}) {} bytes",
                    file.filename, file.content_type, file.content.size());

            // TODO delete this, it's just to see what Cloudflair sends us.
            for (auto const &header : file.headers) {
                spdlog::info("Header: {} = {}", header.first, header.second);
            }

            queue.enqueue(WebUpload {
                .filename = file.filename,
                .contentType = file.content_type,
                .content = file.content,
            });
        }

        // Redirect back home with a message.
        res.set_redirect("/?uploaded=1", httplib::StatusCode::SeeOther_303);
    });

    auto webThread = std::make_shared<std::thread>([server, &config]() {
        spdlog::info("Starting the web server at {}:{}", config.webHostname, config.webPort);
        server->listen(config.webHostname, config.webPort);
        spdlog::info("Shut down the web server");
    });

    return std::make_unique<WebServer>(server, webThread);
}
