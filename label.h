
#pragma once

#include <string>
#include <filesystem>

#include "config.h"

/**
 * Converts a pathname to a nice string. For example "Vacation/Ibiza/Funny_moment.jpg"
 * becomes "Vacation, Ibiza, Funny moment".
 */
std::string pathnameToLabel(Config const config, std::filesystem::path const &pathname);

