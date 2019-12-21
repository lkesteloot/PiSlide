# Copyright 2017 Lawrence Kesteloot
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Methods for accessing the database.

import sqlite3
import logging
from model import Photo, PhotoFile, Email

LOGGER = logging.getLogger(__name__)
LOGGER.setLevel(logging.DEBUG)

# Returns a connection object.
def connect():
    return sqlite3.connect("pislide.db")

def disconnect(con):
    con.close()

# ------------------------------------------------------------------------

def _upgrade0(con):
    con.execute("""
        CREATE TABLE person (id INTEGER PRIMARY KEY, email_address TEXT)""")
    con.execute("""
        CREATE UNIQUE INDEX person_email_address_index ON person(email_address)""")
    con.execute("""
        CREATE TABLE photo (
                    id INTEGER PRIMARY KEY,
                    hash_back text NOT NULL,
                    rotation integer NOT NULL DEFAULT 0,
                    rating integer NOT NULL DEFAULT 3,
                    date integer NOT NULL,
                    display_date text NOT NULL,
                    label text NOT NULL)""")
    con.execute("""
        CREATE UNIQUE INDEX photo_hash_back ON photo(hash_back)""")
    con.execute("""
        CREATE TABLE "email" (
                    id INTEGER PRIMARY KEY,
                    person_id INTEGER,
                    sent_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                    photo_id INTEGER)""")
    con.execute("""
        CREATE TABLE "photo_file" (
                            pathname TEXT PRIMARY KEY,
                            hash_all TEXT,
                            hash_back TEXT)""")
    con.execute("""
        CREATE INDEX photo_file_hash_back ON photo_file(hash_back)""")

UPGRADES = [
    _upgrade0,
]

def upgrade_schema(con):
    con.execute("CREATE TABLE IF NOT EXISTS schema_version (version integer)")

    current_version = query_single_value(con, "SELECT version FROM schema_version ORDER BY version DESC LIMIT 1")
    start_version = current_version + 1 if current_version is not None else 0

    for upgrade_version in range(start_version, len(UPGRADES)):
        LOGGER.info("Applying upgrade %d" % upgrade_version)
        upgrade = UPGRADES[upgrade_version]
        upgrade(con)
        con.execute("INSERT INTO schema_version VALUES (?)", (upgrade_version,))

    con.commit()

    LOGGER.info("There are %d photos in database" % query_single_value(con, "SELECT count(*) FROM photo"))

# ------------------------------------------------------------------------

# Query helpers.

def query(con, cmd, *parameters):
    return list(con.execute(cmd, *parameters))

def query_single_row(con, cmd, *parameters):
    rows = query(con, cmd, *parameters)
    return rows[0] if rows else None

def query_single_value(con, cmd, *parameters):
    row = query_single_row(con, cmd, *parameters)
    return row[0] if row is not None else None

# ------------------------------------------------------------------------

# Methods for PhotoFile object.

PHOTO_FILE_FIELDS = "pathname, hash_all, hash_back"

def row_to_photo_file(row):
    return PhotoFile(row[0], row[1], row[2]) if row else None

def get_photo_files_by_hash_back(con, hash_back):
    return [row_to_photo_file(row) for row in query(con, "SELECT " + PHOTO_FILE_FIELDS + " FROM photo WHERE hash_back = ?", (hash_back,))]

def get_all_photo_files(con):
    return [row_to_photo_file(row) for row in con.execute("SELECT " + PHOTO_FILE_FIELDS + " FROM photo_file")]

def save_photo_file(con, photo_file):
    con.execute("INSERT OR REPLACE INTO photo_file (" + PHOTO_FILE_FIELDS + ") VALUES (?, ?, ?)",
            (photo_file.pathname, photo_file.hash_all, photo_file.hash_back))
    con.commit()

# ------------------------------------------------------------------------

# Methods for Photo object.

PHOTO_FIELDS = "id, hash_back, rotation, rating, date, display_date, label"

def row_to_photo(row):
    return Photo(row[0], row[1], row[2], row[3], row[4], row[5], row[6]) if row else None

def get_all_photos(con):
    return [row_to_photo(row) for row in con.execute("SELECT " + PHOTO_FIELDS + " FROM photo")]

def get_photo_by_id(con, photo_id):
    row = query_single_row(con, "SELECT " + PHOTO_FIELDS + " FROM photo WHERE id = ?", (photo_id,))
    return row_to_photo(row)

def get_photo_by_hash_back(con, hash_back):
    row = query_single_row(con, "SELECT " + PHOTO_FIELDS + " FROM photo WHERE hash_back = ?", (hash_back,))
    return row_to_photo(row)

# Returns the new photo ID.
def create_photo(con, hash_back, rotation, rating, date, display_date, label):
    cursor = con.cursor()
    cursor.execute("INSERT INTO photo (" + PHOTO_FIELDS + ") VALUES (NULL, ?, ?, ?, ?, ?, ?)", (hash_back, rotation, rating, date, display_date, label))
    photo_id = cursor.lastrowid
    con.commit()

    return photo_id

def save_photo(con, photo):
    con.execute("INSERT OR REPLACE INTO photo (" + PHOTO_FIELDS + ") VALUES (?, ?, ?, ?, ?, ?, ?)",
            (photo.id, photo.hash_back, photo.rotation, photo.rating, photo.date, photo.display_date, photo.label))
    con.commit()

# ------------------------------------------------------------------------

# Methods for Email object and persons.

EMAIL_FIELDS = "id, person_id, sent_at, photo_id"

def row_to_email(row):
    return Email(row[0], row[1], row[2], row[3])

def get_all_emails(con):
    return [row_to_email(row) for row in con.execute("SELECT " + EMAIL_FIELDS + " FROM email")]

# Record that an email has been sent.
def email_sent(con, photo_id, email_address):
    email_address = email_address.lower()

    # Create person if necessary.
    person_id = query_single_value(con, "SELECT id FROM person WHERE email_address = ?", (email_address,))
    if person_id is None:
        cursor = con.cursor()
        cursor.execute("INSERT INTO person (id, email_address) VALUES (NULL, ?)", (email_address,))
        person_id = cursor.lastrowid

    # Record email.
    con.execute("INSERT INTO email (id, photo_id, person_id) VALUES (NULL, ?, ?)", (photo_id, person_id))
    con.commit()

# Get list of emails that have a particular prefix in decreasing order of number of sent emails.
def get_emails_with_prefix(con, prefix, max_rows):
    rows = query(con, """SELECT email_address
                            FROM person
                            JOIN email ON person.id = email.person_id
                            WHERE email_address LIKE ?
                            GROUP BY email_address
                            ORDER BY count(*) DESC
                            LIMIT ?""",
                            (prefix + "%", max_rows))
    return [row[0] for row in rows]

def has_photo_been_emailed_by(con, photo_id, email_address):
    count = query_single_value(con, """SELECT count(*)
                                        FROM email
                                        JOIN person ON person.id = email.person_id
                                        WHERE email.photo_id = ? AND person.email_address = ?""",
                                        (photo_id, email_address))
    return count > 0

# ------------------------------------------------------------------------

if __name__ == "__main__":
    con = connect()
    upgrade_schema(con)
    disconnect(con)

