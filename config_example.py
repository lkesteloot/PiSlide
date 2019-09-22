
# Blank configuration. Rename this to config.py and fill out the values.

import os

# The location of the photo tree.
DATA_DIR = "/mnt/data"
# The location of the original photos.
ROOT_DIR = os.path.join(DATA_DIR, "photos")
# The location of the preprocessed photos.
PROCESSED_ROOT_DIR = os.path.join(DATA_DIR, "processed_photos")

# Flags.
ENABLE_SONOS = False

# People we can email from.
EMAIL_FROM = [
    {
        "name": "First Last",
        "first_name": "First",
        "email_address": "first.last@example.com",
    },
    {
        "name": "Other Last",
        "first_name": "Other",
        "email_address": "other.last@example.com",
    },
]

# Directory parts to not display in the label.
BAD_PARTS = set([
    "Published", "Originals", "Unsorted",
])

# Directories to skip altogether.
UNWANTED_DIRS = [
    ".thumbnails",
    ".small",
    "Broken",
    "Private",
]

# If a pathname part has this prefix, the whole part is ignored when displaying
# the label.
BAD_FILE_PREFIXES = ["DSC_", "PICT", "100_", "IMG_", "P101", "VIP_", "Image",
        "pick", "dcp_", "DSCF"]

# Fill this in if you want to display NextBus information with the "b" key.
NEXTBUS_AGENCY_TAG = None    # E.g., "sf-muni"
NEXTBUS_STOP_ID = None

