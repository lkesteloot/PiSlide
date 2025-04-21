
#pragma once

#include <string>
#include <memory>
#include <sqlite3.h>

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
     * In degrees. TODO which direction?
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
    std::string pathname;
};

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
     * SHA-1 of the entire file.
     */
    std::string hashAll;
    /**
     * SHA-1 of the last 1 kB of the file.
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

/**
 * A prepared SQL statement.
 */
class PreparedStatement {
    sqlite3_stmt *mStmt;

public:
    PreparedStatement(sqlite3_stmt *stmt) : mStmt(stmt) {}
    virtual ~PreparedStatement() {
        sqlite3_finalize(mStmt);
    }

    // Can't copy, might prematurely finalize statement.
    PreparedStatement(const PreparedStatement &) = delete;
    PreparedStatement &operator=(const PreparedStatement &) = delete;

    /**
     * Step to the next row, returning whether there are any rows left.
     * Throws on error.
     */
    bool step() const;

    /**
     * Get the integer at the specified column index.
     */
    int32_t getInt(int col) const {
        return sqlite3_column_int(mStmt, col);
    }

    /**
     * Get the long at the specified column index.
     */
    int64_t getLong(int col) const {
        return sqlite3_column_int64(mStmt, col);
    }

    /**
     * Gets the string at the specified column index.
     */
    std::string getString(int col) const {
        const unsigned char *s = sqlite3_column_text(mStmt, col);
        return s == nullptr ? "" : reinterpret_cast<const char *>(s);
    }
};

/**
 * Connection to the database.
 */
class Database {
    sqlite3 *mDb;

    /**
     * Returns a prepared statement of the SQL query. Throws on error.
     */
    std::unique_ptr<PreparedStatement> prepare(std::string const &sql) const;

public:
    Database();
    virtual ~Database();

    // Can't copy, might prematurely close connection.
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    void printPersons() const;
    std::vector<Photo> getAllPhotos() const;
    std::vector<PhotoFile> getAllPhotoFiles() const;
};
