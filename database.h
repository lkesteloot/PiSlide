
#pragma once

#include <string>
#include <memory>
#include <sqlite3.h>

/**
 * Model class for a person.
 */
class Person {
    int mId;
    std::string mEmailAddress;

public:
    Person(int id, std::string const &emailAddress) :
        mId(id), mEmailAddress(emailAddress) {}

    int getId() const {
        return mId;
    }

    std::string getEmailAddress() const {
        return mEmailAddress;
    }
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

    sqlite3_stmt *get() const {
        return mStmt;
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
};
