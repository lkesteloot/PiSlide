
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <sqlite3.h>

#include "model.h"

/**
 * A prepared SQL statement.
 */
class PreparedStatement final {
    sqlite3_stmt *mStmt;

public:
    explicit PreparedStatement(sqlite3_stmt *stmt) : mStmt(stmt) {}
    ~PreparedStatement() {
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
     * Get the integer at the specified column index (0-based).
     */
    int32_t getInt(int col) const {
        return sqlite3_column_int(mStmt, col);
    }

    /**
     * Get the long at the specified column index (0-based).
     */
    int64_t getLong(int col) const {
        return sqlite3_column_int64(mStmt, col);
    }

    /**
     * Gets the string at the specified column index (0-based).
     */
    std::string getString(int col) const {
        const unsigned char *s = sqlite3_column_text(mStmt, col);
        return s == nullptr ? "" : reinterpret_cast<const char *>(s);
    }

    /**
     * Binds the parameter (1-based) to the int value.
     */
    void bindInt(int pos, int value) const {
        sqlite3_bind_int(mStmt, pos, value);
    }

    /**
     * Binds the parameter (1-based) to the long value.
     */
    void bindLong(int pos, long value) const {
        sqlite3_bind_int64(mStmt, pos, value);
    }

    /**
     * Binds the parameter (1-based) to the string value.
     */
    void bindString(int pos, std::string const &value) const {
        // Use transient for safety. Maybe static would work, but we'd
        // have to guarantee that the string is not destroyed before
        // the prepared statement is, and although that's true today,
        // it might not be true some day, and none of this is performance-critical.
        sqlite3_bind_text(mStmt, pos, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
    }
};

/**
 * Connection to the database.
 */
class Database final {
    sqlite3 *mDb;

    /**
     * Returns a prepared statement of the SQL query. Throws on error.
     */
    std::unique_ptr<PreparedStatement> prepare(std::string const &sql) const;

public:
    Database();
    ~Database();

    // Can't copy, might prematurely close the connection.
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    // Queries.
    void printPersons() const;
    std::vector<Photo> getAllPhotos() const;
    std::optional<Photo> getPhotoByHashBack(std::string const &hashBack) const;
    std::vector<PhotoFile> getAllPhotoFiles() const;

    // Updates.
    void savePhoto(Photo const &photo) const;
    int32_t insertPhoto(Photo const &photo) const; // Returns ID.
    void savePhotoFile(PhotoFile const &photoFile) const;
};
