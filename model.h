
#pragma once

#include <iostream>
#include <cstdint>
#include <string>
#include <filesystem>

/**
 * Model object for a person and what we know about them.
 */
struct Person {
    int32_t id;
    std::string emailAddress;
};

/**
  * Represents a photo that was taken. Multiple files on disk (PhotoFile) can
  * represent this photo.
  */
struct Photo {
    int32_t id;
    /**
     * SHA-1 of the last 1 kB of the file.
     */
    std::string hashBack;
    /**
     * Much to rotate the loaded image counter-clockwise in order to display it, in degrees.
     */
    int32_t rotation;
    /**
     * 1 to 5, where 1 means "I'd be okay deleting this file", 2 means "I don't want
     * to show this on this slide show program", 3 is the default, 4 means "This is
     * a great photo", and 5 means "On my deathbed I want a photo album with this photo."
     */
    int32_t rating;
    /**
     * Epoch date when the photo was taken. Defaults to the date of the file.
     */
    int64_t date;
    /**
     * Date as displayed to the user. Usually a friendly version of "date" but might
     * be more vague, like "Summer 1975".
     */
    std::string displayDate;
    /**
     * Label to display. Defaults to a cleaned up version of the pathname, but could be
     * edited by the user.
     */
    std::string label;

    /**
     * In-memory use only, not stored in database. Refers to the preferred photo file to
     * show for this photo.
     */
    std::filesystem::path pathname; // Relative to root path.
    std::filesystem::path absolutePathname;
};

std::ostream &operator<<(std::ostream &os, Photo const &photo);

/**
 * Represents a photo file on disk. Multiple of these (including minor changes in the header)
 * might represent the same Photo.
 */
struct PhotoFile {
    /**
     * Pathname relative to the root of the photo tree.
     */
    std::string pathname;
    /**
     * Hex of the SHA-1 of the entire file.
     */
    std::string hashAll;
    /**
     * Hex of the SHA-1 of the last 1 kB of the file.
     */
    std::string hashBack;
};

/**
 * Represents an email that was sent to a person about a photo.
 */
struct Email {
    int32_t id;
    int32_t personId;
    int64_t sentAt;
    int32_t photoId;
};
