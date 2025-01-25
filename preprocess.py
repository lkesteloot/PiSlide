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

# Preprocess the images so that the slideshow can run faster. The
# pi3d library wants images at a specific set of widths. Resizing some
# of the larger images takes over 20 seconds, so we must do this ahead
# of time into a parallel tree.

import os
from PIL import Image

# From pi3d's Texture.py:
MAX_HEIGHT = 1920
WIDTHS = [4, 8, 16, 32, 48, 64, 72, 96, 128, 144, 192, 256,
        288, 384, 512, 576, 640, 720, 768, 800, 960, 1024, 1080, 1920]

# Process file from src_root into dst_root. The pathname is relative to both.
def process_file(src_root, dst_root, pathname):
    # Absolute pathnames.
    src_absolute_pathname = os.path.join(src_root, pathname)
    dst_absolute_pathname = os.path.join(dst_root, pathname)

    # Compute destination directory.
    dst_dir = os.path.split(dst_absolute_pathname)[0]

    # Create it if necessary.
    if not os.path.exists(dst_dir):
        os.makedirs(dst_dir)

    # Get date of file source.
    mtime = os.path.getmtime(src_absolute_pathname)

    if os.path.exists(dst_absolute_pathname) and \
            int(os.path.getmtime(dst_absolute_pathname)) == int(mtime):

        # Timestamp matches. Assume already processed.
        return

    print("    Processing file " + src_absolute_pathname)

    # Read image.
    try:
        im = Image.open(src_absolute_pathname)
    except IOError as e:
        # Just skip image.
        print("        Got error processing file: " + str(e))
        return
    orig_width, orig_height = width, height = im.size
    size = width*height
    exif = im.info.get("exif", b"")

    # If not okay size, resize in memory. This code taken from Texture.py in pi3d:
    if height > width and height > MAX_HEIGHT: # fairly rare circumstance
        new_height = MAX_HEIGHT
        new_width = int(width*MAX_HEIGHT/height)
        new_size = new_width*new_height
        print("        Resizing from (%d,%d) to (%d,%d), which is %d%% of original" % (width, height, new_width, new_height, new_size*100/size))
        im = im.resize((new_width, new_height), Image.BICUBIC)
        width, height = im.size
    n = len(WIDTHS)
    for i in range(n - 1, 0, -1):
        if width == WIDTHS[i]:
            # No need to resize as already a golden size.
            break
        if width > WIDTHS[i]:
            new_width = WIDTHS[i]
            new_height = int(new_width*height/width)
            new_size = new_width*new_height
            print("        Resizing from (%d,%d) to (%d,%d), which is %d%% of original" % (width, height, new_width, new_height, new_size*100/size))
            im = im.resize((new_width, new_height), Image.BICUBIC)
            width, height = im.size
            break

    # Save file.
    im.save(dst_absolute_pathname, exif=exif)

    # Change date to match original.
    os.utime(dst_absolute_pathname, (mtime, mtime))

# Process files from src_root into dst_root. Only process files specified
# by the (relative) pathnames.
def process_trees(src_root, dst_root, pathnames):
    print("Preprocessing trees with %d files..." % len(pathnames))

    for file_count, pathname in enumerate(pathnames):
        process_file(src_root, dst_root, pathname)

        if (file_count + 1) % 10000 == 0:
            print("    Analyzed %d files" % (file_count + 1,))

    print("    Analyzed %d files" % (len(pathnames),))

