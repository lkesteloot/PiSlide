
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
    slideDisplayTime(12), slideTransitionTime(2), maxPauseTime(60*60), maxBusTime(60*60), minRating(3),
    minDays(0), maxDays(0), windowWidth(0), windowHeight(0) {}

bool Config::readConfigFile(std::filesystem::path const &pathname) {
    try {
        auto config = toml::parse_file(pathname.string());

        if (auto value = config["root_dir"].value<std::string>()) {
            this->rootDir = *value;
        }

        if (auto value = config["slide_display_time"].value<float>()) {
            this->slideDisplayTime = *value;
        }

        if (auto value = config["slide_transition_time"].value<float>()) {
            this->slideTransitionTime = *value;
        }

        if (auto value = config["max_pause_time"].value<float>()) {
            this->maxPauseTime = *value;
        }

        if (auto value = config["min_rating"].value<int>()) {
            this->minRating = *value;
        }

        if (auto value = config["min_days"].value<int>()) {
            this->minDays = *value;
        }

        if (auto value = config["max_days"].value<int>()) {
            this->maxDays = *value;
        }

        if (auto value = config["window_width"].value<int>()) {
            this->windowWidth = *value;
        }

        if (auto value = config["window_height"].value<int>()) {
            this->windowHeight = *value;
        }

        if (auto value = config.at_path("511org.token").value<std::string>()) {
            this->bus511orgToken = *value;
        }

        if (auto value = config.at_path("511org.agency").value<std::string>()) {
            this->bus511orgAgency = *value;
        }

        if (auto value = config.at_path("511org.stop_code").value<std::string>()) {
            this->bus511orgStopCode = *value;
        }

        if (auto unwantedDirs = config["unwanted_dirs"].as_array()) {
            for (auto const &dir : *unwantedDirs) {
                if (auto s = dir.value<std::string>()) {
                    this->unwantedDirs.insert(*s);
                }
            }
        }
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

