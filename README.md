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
* It can display the next three buses at a particular bus stop (from a NextBus feed).
* You can send it new photos by texting them to a Twilio phone number.
* It has a "party mode" that displays the Twilio phone number, so guests can
  take photos, text them, and see them appear within a few seconds.
* It displays the time.

# Using the software

* The left and right arrow keys move to the previous and next slide.
* The space bar toggles pausing the slide show. After a few minutes the
  slide show is automatically unpaused.
* The "L" and "R" keys rotate the photo left and right. The photo file on disk
  is unmodified; only the way it's displayed is affected.
* The digits "1" to "5" rate the photo. The default rating is 3, and the default
  filter only shows photos with a rating of at least 3. Here is the suggested
  way to think about these numbers:
  * A "1" means "I'm okay deleting this photo."
  * A "2" means "I don't want to show this photo on this slide show."
  * A "3" means "This photo is good enough to show on this slide show." This is the
    default value.
  * A "4" means "For a party we should only show these photos." You can use the
    `--min_rating` command-line flag to only show photos with a rating of 4 or above.
  * A "5" means "On my deathbed I want to flip through a photo album with this photo in it."
* The "E" key emails the photo.
* The "Shift-Q" key quits the program.
* The "Shift-D" key toggles the display of debugging information.
* If you have a Sonos system hooked up, the follow keys are also enabled:
  * The "M" key toggles mute.
  * The "S" key stops the music.
  * The "F1" to "F10" keys play the corresponding radio station in your Sonos
    radio station favorites list.
* If you've set up NextBus information (agency and stop), type "B" to toggle
  information about upcoming buses at that stop.
* If you've set up Twilio fetching, type "Shift-T" to toggle fetching new photos.

# Configuring the software

Copy the file `config_example.py` to `config.py` and edit the values. The software
pre-processes images into a parallel directory, so find a place for those (not
necessarily a sibling of the original, as suggested by the example config file).

Pip install the following packages:

* `pi3d`
* `Pillow` for pi3d
* `soco` if you want to control a Sonos.
* `boto3` if you want to send emails.

# Configuring the Raspberry Pi

Put this in `/boot/config.txt`:

    disable_overscan=1
    hdmi_force_hotplug=1
    hdmi_drive=2      (ONLY FOR HDMI DISPLAYS, NOT DVI)
    gpu_mem=256

In `/etc/kbd/config`, modify this line to disable the screen saver:

    BLANK_TIME=0

# Running the software

Usage:

    % python pislide.py [args]

Args:

* `--min_rating STARS`: Show only photos with a rating of `STARS`, from 1 to 5 (default 3).
* `--min_days DAYS`: Show only photos that are at least `DAYS` old (default no minimum).
* `--max_days DAYS`: Show only photos that are at most `DAYS` old (default no maximum).

# The C++ version of the software

Build:

```sh
# Takes about a minute:
cmake -B build
# Takes about a minute:
cmake --build build --parallel
```

Build and run:

```sh
cmake --build build --target run --parallel
```

# Libraries

`TinySHA1.hpp` is from `https://github.com/mohaps/TinySHA1`

`TinyEXIF.cpp` is from `https://github.com/cdcseacave/TinyEXIF`

`QR-Code-generator` is from `https://github.com/nayuki/QR-Code-generator`

# Contributing

Conventions in the code:

* `filename`: Only the file part (`foo.jpg`).
* `pathname`: Relative to a (photo) root (`Vacation/foo.jpg`).
* `absolute_pathname`: Relative to filesystem root (`/mnt/data/photos/Vacation/foo.jpg`).

# License

Copyright 2018 Lawrence Kesteloot

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

