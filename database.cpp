
#include <iostream> // TODO remove
#include <stdexcept>
#include <sstream>

#include "database.h"

static std::string DATABASE_PATHNAME = "pislide.db";
static std::string PHOTO_FIELDS = "id, hash_back, rotation, rating, date, display_date, label";
static std::string PHOTO_FILE_FIELDS = "pathname, hash_all, hash_back";

// --------------------------------------------------------------------------------

bool PreparedStatement::step() const {
    int rc = sqlite3_step(mStmt);
    if (rc == SQLITE_ROW) {
        return true;
    }
    if (rc == SQLITE_DONE) {
        return false;
    }
    std::stringstream ss;
    ss << "can't step prepared statement: " << sqlite3_errmsg(sqlite3_db_handle(mStmt));
    throw std::invalid_argument(ss.str());
}

// --------------------------------------------------------------------------------

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

    while (stmt->step()) {
        Person person {
            .id = stmt->getInt(0),
            .emailAddress { stmt->getString(1) },
        };
        printf("%d %s\n", person.id, person.emailAddress.c_str());
    }
}

std::vector<Photo> Database::getAllPhotos() const {
    auto stmt = prepare(std::string("SELECT ") + PHOTO_FIELDS + " FROM photo");
    std::vector<Photo> photos;

    while (stmt->step()) {
        photos.emplace_back(
                stmt->getInt(0),
                stmt->getString(1),
                stmt->getInt(2),
                stmt->getInt(3),
                stmt->getLong(4),
                stmt->getString(5),
                stmt->getString(6));
    }

    return photos;
}

std::vector<PhotoFile> Database::getAllPhotoFiles() const {
    auto stmt = prepare(std::string("SELECT ") + PHOTO_FILE_FIELDS + " FROM photo_file");
    std::vector<PhotoFile> photoFiles;

    while (stmt->step()) {
        photoFiles.emplace_back(
                stmt->getString(0),
                stmt->getString(1),
                stmt->getString(2));
    }

    return photoFiles;
}

void Database::savePhoto(Photo const &photo) const {
    auto stmt = prepare(std::string("INSERT OR REPLACE INTO photo (") + PHOTO_FIELDS + ") VALUES (?, ?, ?, ?, ?, ?, ?)");

    stmt->bindInt(1, photo.id);
    stmt->bindString(2, photo.hashBack);
    stmt->bindInt(3, photo.rotation);
    stmt->bindInt(4, photo.rating);
    stmt->bindLong(5, photo.date);
    stmt->bindString(6, photo.displayDate);
    stmt->bindString(7, photo.label);

    auto error = stmt->step();
    if (error) {
        throw std::invalid_argument("can't execute statement");
    }
}
