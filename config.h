
#pragma once

#include <string>
#include <filesystem>

/**
 * Holds all configurations. This includes:
 *
 * - Hard-coded constants (like margins) that the user can't even modify.
 * - Parameters from the config file.
 * - Parameters from the command line.
 */
struct Config final {
    /**
     * Root directory of photo files. Must not end with slash.
     */
    std::filesystem::path rootDir;

    /**
     * Loads the config file from disk. Returns whether successful.
     */
    bool readConfigFile(std::filesystem::path const &pathname);

    /**
     * Parses the command-line arguments. Returns whether successful.
     */
    bool parseCommandLine(int argc, char *argv[]);

    /**
     * Checks whether the configuration is valid. If not, prints an
     * error and returns false.
     */
    bool isValid() const;
};

