
#pragma once

#include <string>
#include <filesystem>
#include <set>

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
     * Amount of time each slide is displayed (not including transitions), in seconds.
     */
    float slideDisplayTime;

    /**
     * Transition time, in seconds.
     */
    float slideTransitionTime;

    /**
     * Maximum amount of time the slideshow will be left paused before it
     * automatically resumes, in seconds:
     */
    float maxPauseTime;

    /**
     * Maximum amount of time the bus information will be shown before it
     * automatically turns off, in seconds:
     */
    float maxBusTime;

    /**
     * Minimum photo rating (1-5) to show.
     */
    int minRating;

    /**
     * Minimum age of photos to show, in days, or 0 for no limit.
     */
    int minDays;

    /**
     * Maximum age of photos to show, in days, or 0 for no limit.
     */
    int maxDays;

    /**
     * Width of window, or 0 for full screen.
     */
    int windowWidth;

    /**
     * Height of window, or 0 for full screen.
     */
    int windowHeight;

    /**
     * Token for the 511.org API.
     */
    std::string bus511orgToken;

    /**
     * The 511.org agency (e.g., "SF" for San Francisco MUNI) whose transit information
     * we want to fetch.
     */
    std::string bus511orgAgency;

    /**
     * The bus stop code whose transit information we want to fetch.
     */
    std::string bus511orgStopCode;

    /**
     * Directories to skip.
     */
    std::set<std::filesystem::path> unwantedDirs;

    /**
     * Directory parts to not display in the label.
     */
    std::set<std::string> badParts;

    /**
     * If a pathname part has this prefix (case-sensitive), the whole part
     * is ignored when displaying the label.
     */
    std::set<std::string> badFilePrefixes;

    /**
     * Set default values.
     */
    Config();

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

    /**
     * The total time that a slide takes up, in seconds. This is the display
     * time plus one transition time.
     */
    float slideTotalTime() const {
        return slideDisplayTime + slideTransitionTime;
    }
};

