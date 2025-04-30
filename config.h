
#pragma once

#include <string>

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
    std::string rootDir;

    /**
     * Loads the config file from disk. Returns whether successful.
     */
    bool readConfigFile(std::string const &pathname);

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

