
#include <stdexcept>
#include <sstream>

#include "database.h"

constexpr std::string DATABASE_PATHNAME = "pislide.db";

namespace {
    /**
     * Convenience method for getting a C++ string from a column.
     */
    std::string sqlite3_column_string(sqlite3_stmt *stmt, int col) {
        return reinterpret_cast<const char *>(sqlite3_column_text(stmt, col));
    }
}

Database::Database() {
    mDb = nullptr;

    int rc = sqlite3_open(DATABASE_PATHNAME.c_str(), &mDb);
    if (rc != SQLITE_OK) {
        std::stringstream ss;
        ss << "can't open database \"" << DATABASE_PATHNAME << "\": " << sqlite3_errmsg(mDb);
        sqlite3_close(mDb);
        throw std::invalid_argument(ss.str());
    }
}

Database::~Database() {
    sqlite3_close(mDb);
}

std::unique_ptr<PreparedStatement> Database::prepare(std::string const &sql) const {
    sqlite3_stmt *stmt = nullptr;

    int rc = sqlite3_prepare_v2(mDb, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::stringstream ss;
        ss << "can't prepare statement \"" << sql << "\": " << sqlite3_errmsg(mDb);
        sqlite3_finalize(stmt);
        throw std::invalid_argument(ss.str());
    }

    return std::make_unique<PreparedStatement>(stmt);
}

void Database::printPersons() const {
    auto stmt = prepare("SELECT id, email_address FROM person");

    int rc;
    while ((rc = sqlite3_step(stmt->get())) == SQLITE_ROW) {
        Person person(
                sqlite3_column_int(stmt->get(), 0),
                sqlite3_column_string(stmt->get(), 1));

        printf("%d %s\n", person.getId(), person.getEmailAddress().c_str());
    }
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Can't get row: %s\n", sqlite3_errmsg(mDb));
        return;
    }
}

