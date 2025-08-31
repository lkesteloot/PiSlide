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

# Defines the various low-level model structures. These mirror the database tables.

# Represents a photo that was taken. Multiple files on disk (PhotoFile) can represent
# this photo.
class Photo(object):
    def __init__(self, id, hash_back, rotation, rating, date, display_date, label):
        self.id = id
        # SHA-1 of the last 1 kB of the file.
        self.hash_back = hash_back
        # In degrees.
        self.rotation = rotation
        # 1 to 5, where 1 means "I'd be okay deleting this file", 2 means "I don't want
        # to show this on this slide show program", 3 is the default, 4 means "This is
        # a great photo", and 5 means "On my deathbed I want a photo album with this photo."
        self.rating = rating
        # Unix date when the photo was taken. Defaults to the date of the file.
        self.date = date
        # Date as displayed to the user. Usually a friendly version of "date" but might
        # be more vague, like "Summer 1975".
        self.display_date = display_date
        # Label to display. Defaults to a cleaned up version of the pathname, but could be
        # edited by the user.
        self.label = label

        # In-memory use only, not stored in database. Refers to the preferred photo file to
        # show for this photo.
        self.pathname = None

# Represents a photo file on disk. Multiple of these (including minor changes in the header)
# might represent the same Photo.
class PhotoFile(object):
    def __init__(self, pathname, hash_all, hash_back):
        # Pathname relative to the root of the photo tree.
        self.pathname = pathname
        # SHA-1 of the entire file.
        self.hash_all = hash_all
        # SHA-1 of the last 1 kB of the file.
        self.hash_back = hash_back

# Represents an email that was sent to a person about a photo.
class Email(object):
    def __init__(self, id, pathname, person_id, sent_at, photo_id):
        self.id = id
        self.person_id = person_id
        self.sent_at = sent_at
        self.photo_id = photo_id

