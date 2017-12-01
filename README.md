# PiSlide

This program displays a slide show of images from the local disk.
It's intended to be run in a picture frame or a monitor mounted on a wall,
for example in a kitchen. It has the following features:

* Runs on a Raspberry Pi.
* For the current photo it shows the name and date, as well as its rating (one
  to five stars).
* It lets you rotate photos. The rotation is recorded in the database without
  modifying the photo. This allows you to rsync photos from another computer.
* You can filter the photos by rating or date.
* You can email photos to friends.
* If you have a Sonos music system on your network, it will show the current
  song and artist, and let you play a radio station with one key, stop the music,
  or toggle mute.
* It displays the time.

# Configuring the software

Copy the file `config_example.py` to `config.py` and edit the values. The software
pre-processes images into a parallel directory, so find a place for those (not
necessarily a sibling of the original, as suggested by the example config file).

# Configuring the Raspberry Pi

Put this in `/boot/config.txt`:

    disable_overscan=1
    hdmi_force_hotplug=1
    hdmi_drive=2      (ONLY FOR HDMI DISPLAYS, NOT DVI)
    gpu_mem=256

Also in `/etc/kbd/config`, modify this line:

    BLANK_TIME=0

For pi3d:

    sudo pip install Pillow

To mount the external drive, put this in `/etc/fstab`:

    UUID=9a435c70-07c0-48e9-bc8a-a4d6902595db /mnt/data	ext4	defaults,noatime  0       0

Find the UUID by looking in `/dev/disk/by-uuid`.

If the boot sequence tries to mount the drive before the USB driver has detected it,
add this to `/boot/cmdline.txt`:

    rootdelay=5

# Contributing

Conventions in the code:

* *filename*: Only the file part (foo.jpg).
* *pathname*: Relative to a (photo) root (`Vacation/foo.jpg`).
* *absolute_pathname*: Relative to filesystem root (`/mnt/data/photos/Vacation/foo.jpg`).

# License

Copyright 2017 Lawrence Kesteloot

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

