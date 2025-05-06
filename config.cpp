
#include <iostream>
#include <deque>

#include "toml++/toml.hpp"

#include "config.h"

namespace {
    /**
     * Strips trailing slash from a path. Doesn't strip
     * the slash of the root directory (just "/").
     */
    std::string stripTrailingSlash(std::string const &pathToStrip) {
        std::string path = pathToStrip;

        while (path.size() > 1 && path.ends_with('/')) {
            path.erase(path.end() - 1);
        }

        return path;
    }
}

Config::Config() :
    slideDisplayTime(12), slideTransitionTime(2), maxPauseTime(60*60) {}

bool Config::readConfigFile(std::filesystem::path const &pathname) {
    try {
        auto config = toml::parse_file(pathname.string());

        if (auto rootDir = config["root_dir"].value<std::string>()) {
            this->rootDir = *rootDir;
        }

        if (auto slideDisplayTime = config["slide_display_time"].value<float>()) {
            this->slideDisplayTime = *slideDisplayTime;
        }

        if (auto slideTransitionTime = config["slide_transition_time"].value<float>()) {
            this->slideTransitionTime = *slideTransitionTime;
        }

        if (auto maxPauseTime = config["max_pause_time"].value<float>()) {
            this->maxPauseTime = *maxPauseTime;
        }

        /*
        // Get array of strings
        if (auto servers = config["servers"].as_array()) {
            std::cout << "Servers:\n";
            for (const auto& val : *servers) {
                if (auto s = val.value<std::string>())
                    std::cout << "  - " << *s << "\n";
            }
        }
        */

    } catch (toml::parse_error const &err) {
        std::cerr << "Problem with config file " << pathname << ":\n"
                << "  " << err.description() << '\n'
                << "  at " << err.source().begin << '\n';
        return false;
    }

    return true;
}

bool Config::parseCommandLine(int argc, char *argv[]) {
    // Skip program name.
    std::deque<std::string> args(&argv[1], &argv[argc]);

    // Get and pop the next arg to process.
    auto nextArg = [&args]() {
        std::string arg = args.front();
        args.pop_front();
        return arg;
    };

    while (!args.empty()) {
        std::string arg = nextArg();

        if (arg == "--root-dir") {
            if (args.empty()) {
                std::cerr << "Must specify directory for --root-dir" << '\n';
                return false;
            }
            rootDir = stripTrailingSlash(nextArg());
        } else {
            std::cerr << "Unknown flag: " << arg << '\n';
            return false;
        }
    }

    return true;
}

bool Config::isValid() const {
    if (rootDir.empty()) {
        std::cerr << "Must specify a rootDir" << '\n';
        return false;
    }


    return true;
}

