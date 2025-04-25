
#pragma once

#include <string>
#include <memory>
#include <sqlite3.h>

#include "model.h"

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
