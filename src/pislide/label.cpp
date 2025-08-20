
#include <cctype>
#include <algorithm>
#include <regex>

#include "label.h"
#include "util.h"

namespace {
    std::regex DATE_RE("\\d\\d\\d\\d-\\d\\d-\\d\\d\\s*(.*)\\s*");
    std::regex TIME_RE("\\d\\d\\.\\d\\d\\.\\d\\d\\s*(.*)\\s*");

    /**
     * Convert a character to upper case.
     */
    char toUpper(char ch) {
        // I love C++.
        return static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }

    /**
     * Make the first letter upper case, leave the rest untouched.
     */
    std::string capitalize(std::string s) {
        if (!s.empty()) {
            s[0] = toUpper(s[0]);
        }

        return s;
    }

    /**
     * Whether the character is in base "base".
     */
    bool charInBase(char ch, int base) {
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            int value = ch - '0';
            return value >= 0 && value < base;
        }
        if (std::isalpha(static_cast<unsigned char>(ch))) {
            int value = toUpper(ch) - 'A' + 10;
            return value >= 10 && value < base;
        }
        return false;
    }

    /**
     * Whether the string is entirely made up of digits of base "base".
     */
    bool isEntirelyNumber(std::string const &s, int base) {
        for (char ch : s) {
            if (!charInBase(ch, base)) {
                return false;
            }
        }

        return true;
    }

    /**
     * Guess whether the string is a 4-digit year.
     */
    bool isYear(std::string const &s) {
        return s.size() == 4 && isEntirelyNumber(s, 10) && (s.starts_with("19") or s.starts_with("20"));
    }

    /**
     * Convert a part of a pathname (a segment between slashes) to a nice string.
     */
    std::string cleanPart(std::string part) {
        // Replace underscores with spaces.
        std::replace(part.begin(), part.end(), '_', ' ');


        // Strip initial date.
        std::smatch match;
        if (std::regex_match(part, match, DATE_RE)) {
            part = match[1];
        }

        // Strip initial time. Usually after DATE_RE for Dropbox camera uploads.
        if (std::regex_match(part, match, TIME_RE)) {
            part = match[1];
        }

        // Capitalize.
        part = capitalize(part);

        return part;
    }

    /**
     * Whether to keep this part of the pathname in the nice string.
     */
    bool keepPart(Config const &config, std::string const &part) {
        // Empty?
        if (part.empty()) {
            return false;
        }

        // Bad prefix?
        for (std::string const &prefix : config.badFilePrefixes) {
            if (part.starts_with(prefix)) {
                return false;
            }
        }

        // Bad altogether?
        if (config.badParts.contains(part)) {
            return false;
        }

        // All numbers (and not a year)?
        if (isEntirelyNumber(part, 10) && !isYear(part)) {
            return false;
        }
        if (part[0] == 'P' && isEntirelyNumber(part.substr(1), 10)) {
            return false;
        }

        // Twilio image?
        if (part.starts_with("ME") && part.size() == 34 && isEntirelyNumber(part.substr(2), 16)) {
            return false;
        }

        return true;
    }
}

std::string pathnameToLabel(Config const config, std::filesystem::path const &pathname) {
    std::vector<std::string> parts = split(pathname.string(), '/');

    // Strip extension.
    parts[parts.size() - 1] = stripExtension(parts[parts.size() - 1]);

    // Remove and clean up sections.
    std::string label;
    for (std::string const &part : parts) {
        if (keepPart(config, part)) {
            std::string cleanedPart = cleanPart(part);
            if (!cleanedPart.empty()) {
                if (!label.empty()) {
                    label += ", ";
                }
                label += cleanedPart;
            }
        }
    }

    return label;
}
