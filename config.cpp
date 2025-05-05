
#include <iostream>
#include <deque>

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

bool Config::readConfigFile(std::filesystem::path const &pathname) {
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

