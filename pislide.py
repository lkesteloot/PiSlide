# Copyright 2018 Lawrence Kesteloot
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

# Importing pi3d takes a long time, so show something.
print "Importing..."
import os
import sys
import random
import time
import re
import logging
import argparse
import math
import Queue
import threading
from PIL import Image
sys.path.insert(1, "/home/pi/pi3d")
import pi3d
import sha
import collections

import db
import email_service
from model import Photo, PhotoFile, Email
import preprocess
import config

if config.ENABLE_SONOS:
    import sonos
if config.NEXTBUS_AGENCY_TAG:
    import nextbus
if config.TWILIO_SID:
    import twilio

# These are for debugging, should normally be False:
QUICK_SPEED = False

# Set this to a relative directory in order to restrict photos to only
# those within that sub-tree. This is useful for forcing the display of
# only some kinds of photos.
SUBDIR = None
# SUBDIR = "Vacation/Ibiza"

LOG_FORMAT = "%(asctime)s %(levelname)s: %(name)s: %(message)s"
TEXT_SHADOW_COLOR = (0, 0, 0)
DATE_RE = re.compile(r"\d\d\d\d-\d\d-\d\d(.*)")
TIME_RE = re.compile(r"\d\d\.\d\d\.\d\d(.*)")
MAX_CACHE_SIZE = 4
DEFAULT_SHOW_DEBUG = False
DISPLAY_MARGIN = 50
TIME_TOP_DISPLAY_MARGIN = 50
TRACK_TOP_DISPLAY_MARGIN = 150
MUSIC_LEADING = 40
STATION_LEADING = 60
BUS_MAJOR_LEADING = 45
BUS_MINOR_LEADING = 35
EMAIL_LEADING = 60
FIRST_FUNCTION_KEY = 265
MAX_SUGGESTED_EMAILS = 8
MAX_NO_FILE_WARNINGS = 10
STAR_SCALE = 30
# For some reason our star draws too far to the left, so we adjust it. I can't
# figure out why.  For all I know maybe the text is too far to the right.
STAR_OFFSET = 10
MAX_PAUSE_SECONDS = 5*60
MAX_BUS_SECONDS = 60*60
MAX_TWILIO_SECONDS = 8*60*60

# The display time includes only one transition (the end one):
SLIDE_DISPLAY_S = 5 if QUICK_SPEED else 14
SLIDE_TRANSITION_S = 2

TEXT_COLOR = (255, 255, 255, 200)
DATE_COLOR = (255, 255, 255, 150)
TIME_COLOR = (255, 255, 255, 100)
MUSIC_COLOR = (255, 255, 255, 100)
STATION_COLOR = (255, 255, 255, 200)
EMAIL_COLOR = (255, 255, 255, 255)
BUS_COLOR = (255, 255, 255, 200)
print "Loading fonts..."
# There seems to be a bug in newer versions of pi3d where text in large fonts show some
# junk above and below the text. A size of 48 seems to work fine.
MAX_FONT_SIZE = 48
TEXT_FONT = pi3d.Font("FreeSans.ttf", TEXT_COLOR, font_size=min(48, MAX_FONT_SIZE))
DATE_FONT = pi3d.Font("FreeSans.ttf", DATE_COLOR, font_size=min(40, MAX_FONT_SIZE))
TIME_FONT = pi3d.Font("FreeSans.ttf", TIME_COLOR, font_size=min(64, MAX_FONT_SIZE))
TRACK_FONT = pi3d.Font("FreeSans.ttf", MUSIC_COLOR, font_size=min(48, MAX_FONT_SIZE))
ARTIST_FONT = pi3d.Font("FreeSans.ttf", MUSIC_COLOR, font_size=min(32, MAX_FONT_SIZE))
STATION_FONT = pi3d.Font("FreeSans.ttf", STATION_COLOR, font_size=min(48, MAX_FONT_SIZE))
EMAIL_FONT = pi3d.Font("FreeSans.ttf", EMAIL_COLOR, font_size=min(64, MAX_FONT_SIZE))
BUS_MAJOR_FONT = pi3d.Font("FreeSans.ttf", BUS_COLOR, font_size=min(48, MAX_FONT_SIZE))
BUS_MINOR_FONT = pi3d.Font("FreeSans.ttf", BUS_COLOR, font_size=min(32, MAX_FONT_SIZE))

TWILIO_PATH = os.path.join(config.ROOT_DIR, config.TWILIO_SUBDIR)

g_frame_count = 0

# If "s" starts with prefix, return "s" without prefix. Otherwise returns "s".
def strip_prefix(s, prefix):
    if s.startswith(prefix):
        s = s[len(prefix):]

    return s

# Whether the pathname refers to an image that we want to show.
def is_image(pathname):
    _, ext = os.path.splitext(pathname)
    return ext.lower() in [".jpg", ".jpeg"]

# Configure a new logger and return it.
def configure_logger():
    # When we imported pi3d, it set up a default handler that dumps
    # to stdout. Remove it. It dumps a bunch of crap that interferes
    # with exceptions.
    rootLogger = logging.getLogger()
    for handler in rootLogger.handlers:
        rootLogger.removeHandler(handler)

    # Our own logger to our file.
    handler = logging.FileHandler("pislide.log")
    handler.setFormatter(logging.Formatter(LOG_FORMAT))
    handler.setLevel(logging.DEBUG)

    # Add our logger to the root one, so we can see what other libs
    # are saying.
    rootLogger.addHandler(handler)
    rootLogger.setLevel(logging.INFO)

    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)

    # Log this so that it's easy to see when the program started.
    logger.info("------------------------------------------------------------------")

    return logger

# Return a number that is t (which is 0 to 1) between a and b.
def interpolate(a, b, t):
    return a + (b - a)*t

# Clamp x at a specified low or high value.
def clamp(x, low, high):
    return min(max(x, low), high)

# The string.capitalize() function lower-cases the rest of the string. We
# want to leave it untouched.
def capitalize(s):
    if s:
        s = s[0].upper() + s[1:]

    return s

# Whether the string is entirely made up of digits of base "base".
def is_entirely_number(s, base):
    # Try to parse it. Not perfect, it'll accept " 123 ", but close enough.
    try:
        int(s, base)
    except ValueError:
        return False

    return True

# Guess whether the string is a 4-digit year.
def is_year(s):
    return len(s) == 4 and (s.startswith("19") or s.startswith("20"))

# Convert mtime (integer) to a nice string.
def mtime_to_string(mtime):
    # Convert to a tuple.
    localtime = time.localtime(mtime)

    # Convert to a nice string.
    return time.strftime("%B %-d, %Y", localtime)

# Convert a part of a pathname (a segment between slashes) to a nice string.
def clean_part(part):
    # Replace underscores with spaces.
    part = part.replace("_", " ")

    # Strip initial date.
    match = DATE_RE.match(part)
    if match:
        part = match.group(1).strip()

    # Strip initial time. Usually after DATE_RE for Dropbox camera uploads.
    match = TIME_RE.match(part)
    if match:
        part = match.group(1).strip()

    # Capitalize.
    part = capitalize(part)

    return part

# Whether to keep this part of the pathname in the nice string.
def keep_part(part):
    # Empty?
    if not part:
        return False

    # Bad prefix?
    for prefix in config.BAD_FILE_PREFIXES:
        if part.startswith(prefix):
            return False

    # Bad altogether?
    if part in config.BAD_PARTS:
        return False

    # All numbers (and not a year)?
    if is_entirely_number(part, 10) and not is_year(part):
        return False
    if part[0] == "P" and is_entirely_number(part[1:], 10):
        return False

    # Twilio image?
    if part.startswith("ME") and len(part) == 34 and is_entirely_number(part[2:], 16):
        return False

    return True

# Converts a pathname to a nice string. For example "Vacation/Ibiza/Funny_moment.jpg"
# becomes "Vacation, Ibiza, Funny moment".
def clean_pathname(pathname):
    parts = pathname.split("/")

    # Strip extension.
    parts[-1] = os.path.splitext(parts[-1])[0]

    # Remove and clean up sections.
    parts = [clean_part(part) for part in parts if keep_part(part)]

    # Another pass to remove empty parts (clean_part can generate those).
    parts = [part for part in parts if part]

    return ", ".join(parts)

# Subclass Texture so we can modify the image.
class SlideshowTexture(pi3d.Texture):
    def __init__(self, pathname):
        super(SlideshowTexture, self).__init__(pathname, blend=True, mipmap=True)

    # Override Texture.
    def _load_disk(self):
        super(SlideshowTexture, self)._load_disk()

        # Add a black border around the edge because texture anti-aliasing works
        # better than polygon anti-aliasing.
        img = self.image
        width, height, depth = img.shape

        BORDER_SIZE = max(2, int(width/1000.0 + 0.5))

        # Generate a view that has the X and Y axes transposed so that
        # we can slice it by rows and fill them quickly.
        img_t = img.transpose(1, 0, 2)

        for b in range(BORDER_SIZE):
            # Top/bottom border.
            img_t[b].fill(0)
            img_t[height - 1 - b].fill(0)

            # Left/right border.
            img[b].fill(0)
            img[width - 1 - b].fill(0)

# Caches a pi3d String object.
class CachedString(object):
    # Justify is "C" for central, can be "R" for right or "L" for left.
    def __init__(self, shader, font, z, justify):
        self.shader = shader
        self.font = font
        self.z = z
        self.justify = justify

        self.text = None
        self.x = None
        self.y = None
        self.string = None

    def get(self, text, x, y):
        if text != self.text:
            self.text = text
            self.x = x
            self.y = y

            self.string = pi3d.String(font=self.font, string=text, is_3d=False, x=x, y=y, z=self.z, justify=self.justify)
            self.string.set_shader(self.shader)

        # Move it if necessary.
        if x != self.x or y != self.y:
            self.x = x
            self.y = y

            self.string.positionX(x)
            self.string.positionY(y)

        return self.string

# Fetches bus information in a different thread, keeping the
# most recent data available for fetching and showing in the UI.
class NextbusFetcher(object):
    def __init__(self):
        self.logger = logging.getLogger("nextbus")
        self.logger.setLevel(logging.DEBUG)

        self.fetching = False
        self.thread = None
        self.running = True

        # Bool for whether fetching:
        self.command_queue = Queue.Queue()

        # Predictions objects:
        self.predictions_queue = Queue.Queue()

    # Start the thread. Does not start fetching.
    def start(self):
        self.thread = threading.Thread(target=self._loop)
        self.thread.daemon = True
        self.thread.start()

    # Stop the thread.
    def stop(self):
        # I think this is thread-safe.
        self.running = False

        # Exit the get().
        self.set_fetching(False)

        if self.thread:
            self.thread.join()
            self.thread = None

    # Set whether to periodically fetch predictions.
    def set_fetching(self, fetching):
        self.command_queue.put(fetching)

    # Gets the most recent predictions, or None if there aren't any.
    def get_predictions(self):
        try:
            return self.predictions_queue.get_nowait()
        except Queue.Empty:
            return None

    # Runs in the other thread.
    def _loop(self):
        while self.running:
            try:
                try:
                    self.fetching = self.command_queue.get(True, 10)
                except Queue.Empty:
                    # No problem, ignore.
                    pass

                if self.fetching:
                    predictions = nextbus.get_predictions_for_stop(config.NEXTBUS_AGENCY_TAG, config.NEXTBUS_STOP_ID)
                    self.predictions_queue.put(predictions)
            except BaseException as e:
                # Probably connection error. Wait a bit and try again.
                self.logger.warning("Got exception in nextbus thread: %s" % e)
                time.sleep(60)

        self.logger.info("Exiting nextbus loop")

# Fetches photos from Twilio in a different thread, keeping the
# list of fetched images available for showing in the UI.
class TwilioFetcher(object):
    def __init__(self):
        self.logger = logging.getLogger("twilio")
        self.logger.setLevel(logging.DEBUG)

        if not os.path.exists(TWILIO_PATH):
            os.makedirs(TWILIO_PATH)

        self.fetching = False
        self.thread = None
        self.running = True

        # Bool for whether fetching:
        self.command_queue = Queue.Queue()

        # Each entry is a pathname to a newly-downloaded and resized photo:
        self.photo_pathname_queue = Queue.Queue()

    # Start the thread. Does not start fetching.
    def start(self):
        self.thread = threading.Thread(target=self._loop)
        self.thread.daemon = True
        self.thread.start()

    # Stop the thread.
    def stop(self):
        # I think this is thread-safe.
        self.running = False

        # Exit the get().
        self.set_fetching(False)

        if self.thread:
            self.thread.join()
            self.thread = None

    # Set whether to periodically fetch images.
    def set_fetching(self, fetching):
        self.command_queue.put(fetching)

    # Gets the most recent photo pathname, or None if there isn't one.
    def get_photo_pathname(self):
        try:
            return self.photo_pathname_queue.get_nowait()
        except Queue.Empty:
            return None

    # Runs in the other thread.
    def _loop(self):
        while self.running:
            try:
                try:
                    self.fetching = self.command_queue.get(True, twilio.FETCH_PERIOD_S)
                except Queue.Empty:
                    # No problem, ignore.
                    pass

                if self.fetching:
                    results = twilio.download_images(TWILIO_PATH, True, self.logger)
                    if results:
                        for image in results.images:
                            self._process_image(image)
                    else:
                        # Don't spam API.
                        time.sleep(60)
            except BaseException as e:
                # Probably connection error. Wait a bit and try again.
                self.logger.warning("Got exception in twilio thread: %s" % e)
                time.sleep(60)

        self.logger.info("Exiting twilio loop")

    # Resize the downloaded image.
    def _process_image(self, image):
        pathname = image.pathname
        self.logger.info("Received Twilio image from %s on %s: %s (%s)" %
                (image.phone_number, image.date_sent, pathname, image.content_type))

        if image.content_type != "image/jpeg":
            self.logger.warn("Can't handle image type " + image.content_type)
            return

        if not pathname.startswith(config.ROOT_DIR):
            self.logger.warn("Image %s is not under %s" % (pathname, config.ROOT_DIR))
            return

        # Find path relative to root.
        pathname = pathname[len(config.ROOT_DIR):].lstrip("/")

        # Resize image, since we're already in a worker thread.
        preprocess.process_file(config.ROOT_DIR, config.PROCESSED_ROOT_DIR, pathname)

        self.photo_pathname_queue.put(pathname)

# A displayed slide.
class Slide(pi3d.ImageSprite):
    def __init__(self, photo, texture, shader, star_sprite, load_time, frame_count):
        super(Slide, self).__init__(texture, shader, w=1.0, h=1.0)

        self.photo = photo
        self.texture = texture
        self.star_sprite = star_sprite
        self.load_time = load_time
        self.frame_count = frame_count
        self.is_broken = False

        # What we're drawing.
        self.actual_rotate = 0
        self.actual_width = 0
        self.actual_height = 0
        self.actual_zoom = 0
        self.actual_alpha = 0
        self.set_alpha(self.actual_alpha)

        # Filled in later in compute_ideal_size().
        self.ideal_width = 0
        self.ideal_height = 0

        # For the zoom.
        self.start_zoom = 0.9
        self.end_zoom = 1.3 # Clip off a bit.
        self.swap_zoom = None # None means "False" but also "Not yet set".

        # We've not moved yet, so our actuals are bogus.
        self.configured = False

        # When we were last used (drawn, moved, requested).
        self.last_used = 0

        show_twilio_instructions = config.PARTY_MODE and config.TWILIO_SID
        label = config.TWILIO_MESSAGE if show_twilio_instructions else self.photo.label
        self.name_label = pi3d.String(font=TEXT_FONT, string=label,
                is_3d=False, x=0, y=-360, z=0.05)
        self.name_label.set_shader(shader)

        display_date = "" if show_twilio_instructions else self.photo.display_date
        self.date_label = pi3d.String(font=DATE_FONT, string=display_date,
                is_3d=False, x=0, y=-420, z=0.05)
        self.date_label.set_shader(shader)

        self.show_labels = False

    # Save our own state to the database.
    def persist_state(self, con):
        db.save_photo(con, self.photo)

    def __str__(self):
        # We might get called by the super constructor, so make sure we've been
        # initialized.
        photo = getattr(self, "photo", None)
        if photo:
            filename = os.path.split(photo.pathname)[1]
            return "%s (%.1fs, %d frames)" % (filename, self.load_time, self.frame_count)
        else:
            # Just use the default method.
            return super(Slide, self).__str__()

    # Compute the ideal width and height given the screen size and the rotation
    # to fit the photo as large as possible.
    def compute_ideal_size(self, screen_width, screen_height):
        texture_width = self.texture.ix
        texture_height = self.texture.iy

        angle = self.photo.rotation % 360
        rotated = angle == 90 or angle == 270

        if rotated:
            # Swap, it's rotated on its side.
            texture_width, texture_height = texture_height, texture_width

        if screen_width*texture_height > texture_width*screen_height:
            # Screen is wider than photo.
            height = screen_height
            width = texture_width*height/texture_height
        else:
            # Photo is wider than screen.
            width = screen_width
            height = texture_height*width/texture_width

        if rotated:
            # Swap back.
            width, height = height, width

        self.ideal_width = width
        self.ideal_height = height

    # Move toward our ideal values and update the sprite position.
    def move(self, paused, prompting_email, time):
        if self.configured:
            step = 0.3
        else:
            step = 1.0
            self.configured = True

        self.actual_rotate = interpolate(self.actual_rotate, self.photo.rotation, step)
        self.rotateToZ(self.actual_rotate)

        self.actual_width = interpolate(self.actual_width, self.ideal_width, step)
        self.actual_height = interpolate(self.actual_height, self.ideal_height, step)

        if paused:
            ideal_zoom = 1.0
            ideal_alpha = (0.2 if prompting_email else 1.0) if time >= 0 else 0.0
            self.show_labels = time >= 0
        else:
            t = time/SLIDE_DISPLAY_S
            ideal_zoom = interpolate(self.start_zoom, self.end_zoom,
                    1 - t if self.swap_zoom else t)

            if time < 0:
                ideal_alpha = clamp((time + SLIDE_TRANSITION_S)/SLIDE_TRANSITION_S, 0.0, 1.0)
                self.show_labels = False
            elif time < SLIDE_DISPLAY_S - SLIDE_TRANSITION_S:
                ideal_alpha = 1.0
                self.show_labels = True
            else:
                ideal_alpha = clamp((SLIDE_DISPLAY_S - time)/SLIDE_TRANSITION_S, 0.0, 1.0)
                self.show_labels = False

        self.actual_zoom = interpolate(self.actual_zoom, ideal_zoom, step)
        self.scale(self.actual_width*self.actual_zoom, self.actual_height*self.actual_zoom, 1.0)

        # Alpha is always slow.
        self.actual_alpha = interpolate(self.actual_alpha, ideal_alpha, 0.3)
        self.set_alpha(self.actual_alpha)

        self.touch()

    def draw(self):
        super(Slide, self).draw()
        if self.show_labels:
            self.name_label.draw()
            self.date_label.draw()

            if self.photo.rating != 3:
                for i in range(self.photo.rating):
                    self.star_sprite.scale(STAR_SCALE, STAR_SCALE, 1)
                    self.star_sprite.position((-(self.photo.rating - 1)/2.0 + i)*STAR_SCALE*1.2 + STAR_OFFSET, -470, 0.05)
                    self.star_sprite.draw()
        self.touch()

    def turn_off(self):
        self.configured = False
        self.actual_alpha = 0.0
        self.set_alpha(self.actual_alpha)

    def touch(self):
        self.last_used = time.time()

# When we can't load an image, use this object as a stand-in.
class BrokenSlide(object):
    def __init__(self):
        self.is_broken = True

        # When we were last used (drawn, moved, requested).
        self.last_used = 0

        # Whether we zoom in or out.
        self.swap_zoom = False

    def touch(self):
        self.last_used = time.time()

# A separate thread with an API for submitting slides to load asynchronously.
class SlideLoader(object):
    def __init__(self, shader, star_sprite, screen_width, screen_height):
        self.shader = shader
        self.star_sprite = star_sprite
        self.screen_width = screen_width
        self.screen_height = screen_height

        self.thread = None
        self.running = True

        # Photo:
        self.request_queue = Queue.Queue()

        # (Photo, Slide) tuples:
        self.reply_queue = Queue.Queue()

        # Already requested photo IDs:
        self.already_requested_ids = set()

    def start(self):
        self.thread = threading.Thread(target=self.loop)
        self.thread.daemon = True
        self.thread.start()

    def stop(self):
        # I think this is thread-safe.
        self.running = False

        # Break out of loop().
        self.request_slide(None)

        if self.thread:
            self.thread.join()

    # Request that this slide be loaded. Calling this while this photo ID
    # is already in the queue is a no-op. Pass None for photo to kill
    # the thread.
    def request_slide(self, photo):
        if photo:
            if photo.id not in self.already_requested_ids:
                self.already_requested_ids.add(photo.id)
                self.request_queue.put(photo)
        else:
            # Kill thread.
            self.request_queue.put(None)

    # Generates (photo, slide) tuples. The slide may be None if the slide couldn't be loaded.
    def get_loaded_slides(self):
        while True:
            try:
                photo, slide = self.reply_queue.get_nowait()
                self.already_requested_ids.discard(photo.id)
                yield photo, slide
            except Queue.Empty:
                break

    # Runs in thread.
    def loop(self):
        while self.running:
            photo = self.request_queue.get()
            if photo is None:
                # End thread.
                break

            slide = None
            try:
                slide = self._load(photo)
            finally:
                # Do this no matter what.
                self.reply_queue.put( (photo, slide) )

        LOGGER.info("Exiting SlideLoader loop")

    # Runs in thread.
    def _load(self, photo):
        global g_frame_count

        absolute_pathname = os.path.join(config.PROCESSED_ROOT_DIR, photo.pathname)
        LOGGER.info("Loading %s" % (absolute_pathname,))

        # Load the image.
        try:
            before_time = time.time()
            before_frame_count = g_frame_count
            texture = SlideshowTexture(absolute_pathname)
            after_frame_count = g_frame_count
            after_time = time.time()
            load_time = after_time - before_time
            frame_count = after_frame_count - before_frame_count
        except Exception as e:
            LOGGER.error("Got error reading file \"%s\": %s" % (absolute_pathname, e))
            return None

        slide = Slide(photo, texture, self.shader, self.star_sprite, load_time, frame_count)
        slide.compute_ideal_size(self.screen_width, self.screen_height)

        return slide

# Given a list of photos, can retrieve a photo by index, caching a few of them.
class SlideCache(object):
    def __init__(self, slide_loader):
        self.slide_loader = slide_loader

        # From photo ID to Slide object.
        self.cache = {}

    # Return immediately with a Slide object if we've loaded this photo,
    # or return None and optionally start loading the photo asynchronously.
    def get(self, photo, fetch=True):
        # Before doing anything, see if the loader has anything for us.
        self._fill_cache()

        slide = self.cache.get(photo.id)
        if not slide and fetch:
            # Cap size of cache. Do this before we load the next slide.
            self._shrink_cache()

            # Load the photo.
            self.slide_loader.request_slide(photo)

        return slide

    def _fill_cache(self):
        # Drain the return queue of the loader.
        for photo, slide in self.slide_loader.get_loaded_slides():
            # Make sure the cache has space.
            self._shrink_cache()

            # The slide might be None if we couldn't load it. This causes problems
            # throughout the rest of the program, and we can't easily purge it
            # from the cache, so we make a fake slide.
            if not slide:
                slide = BrokenSlide()

            self.cache[photo.id] = slide

            if slide.is_broken:
                LOGGER.error("Couldn't load image")
            else:
                LOGGER.debug("Reading image took %.1f seconds (%d frames)." % (slide.load_time, slide.frame_count))

    # Leave at least one place in the cache.
    def _shrink_cache(self):
        while len(self.cache) >= MAX_CACHE_SIZE:
            self._purge_oldest()

    # Purge one slide that was used least recently.
    def _purge_oldest(self):
        oldest_photo_id = None
        oldest_time = None

        for photo_id, slide in self.cache.items():
            if oldest_time is None or slide.last_used < oldest_time:
                oldest_photo_id = photo_id
                oldest_time = slide.last_used

        if oldest_photo_id is not None:
            del self.cache[oldest_photo_id]

    def get_slides(self):
        return self.cache.values()

    def get_cache(self):
        return self.cache

# Handles the display of the slideshow (current slide, transitions, and actions from the user).
class Slideshow(object):
    def __init__(self, photos, display, shader, star_sprite, screen_size, con):
        self.photos = photos
        self.display = display
        self.shader = shader
        self.screen_width = screen_size[0]
        self.screen_height = screen_size[1]
        self.con = con

        self.slide_loader = SlideLoader(shader, star_sprite, self.screen_width, self.screen_height)
        self.slide_loader.start()

        self.slide_cache = SlideCache(self.slide_loader)

        self.sonos = sonos.SonosController() if config.ENABLE_SONOS else None
        if self.sonos:
            self.sonos.start()
        self.sonos_status = None

        self.show_debug = DEFAULT_SHOW_DEBUG
        # Photo ID to CachedString.
        self.debug_cache = {}

        self.paused = False
        self.pause_start_time = None
        self.time = 0.0
        self.previous_frame_time = None

        self.prompting_email = False
        self.email_error = False
        self.email_address = ""
        self.email_from = 0
        self.email_from_label = CachedString(shader, EMAIL_FONT, 0.05, "L")
        self.email_prompt_label = CachedString(shader, EMAIL_FONT, 0.05, "L")
        self.email_label = CachedString(shader, EMAIL_FONT, 0.05, "L")
        self.suggested_email_labels = [
            CachedString(shader, EMAIL_FONT, 0.05, "L") for i in range(MAX_SUGGESTED_EMAILS)]
        # List of (email_address, already_emailed) tuples.
        self.suggested_emails = []

        # Thread to fetch NextBus information.
        self.nextbus_fetcher = NextbusFetcher() if config.NEXTBUS_AGENCY_TAG else None
        if self.nextbus_fetcher:
            self.nextbus_fetcher.start()
        self.showing_bus = False
        self.bus_start_time = None
        self.bus_predictions = None
        self.bus_route_label = CachedString(shader, BUS_MAJOR_FONT, 0.05, "L")
        self.bus_direction_label = CachedString(shader, BUS_MINOR_FONT, 0.05, "L")
        self.bus_stop_label = CachedString(shader, BUS_MINOR_FONT, 0.05, "L")
        self.bus_prediction_labels = [CachedString(shader, BUS_MINOR_FONT, 0.05, "L") for i in range(3)]

        # Thread to fetch Twilio photos.
        self.twilio_fetcher = TwilioFetcher() if config.TWILIO_SID else None
        if self.twilio_fetcher:
            self.twilio_fetcher.start()
        self.fetching_twilio = False
        self.twilio_start_time = None

        # Labels to show onscreen.
        self.time_label = CachedString(shader, TIME_FONT, 0.05, "R")
        self.track_label = CachedString(shader, TRACK_FONT, 0.05, "R")
        self.artist_label = CachedString(shader, ARTIST_FONT, 0.05, "R")
        self.station_labels = [CachedString(shader, STATION_FONT, 0.05, "L") for i in range(10)]

    def shutdown(self):
        # Blocks until these threads are dead, so we don't get exceptions
        # while shutting down the interpreter.
        self.slide_loader.stop()
        if self.sonos:
            self.sonos.stop()
        if self.nextbus_fetcher:
            self.nextbus_fetcher.stop()
        if self.twilio_fetcher:
            self.twilio_fetcher.stop()

    # Returns whether we took the key.
    def take_key(self, key):
        if self.prompting_email:
            if key == 27: # ESC
                self.prompting_email = False
                self.paused = False
                self.pause_start_time = None
            elif key == 8 or key == 263:
                if len(self.email_address) > 0:
                    self.email_address = self.email_address[:-1]
                    self.update_suggested_emails()
            elif key == 9:
                if len(self.suggested_emails) == 1:
                    self.complete_email(0)
            elif key == 10 or key == 13:
                self.email_address = self.email_address.strip()
                if self.email_address:
                    # Submit email address.
                    success = self.email_photo(self.email_address)
                else:
                    success = True
                if success:
                    self.prompting_email = False
                    self.paused = False
                    self.pause_start_time = None
                else:
                    self.email_error = True
            elif key >= 32 and key < 128:
                self.email_address += chr(key)
                self.update_suggested_emails()
            elif key >= FIRST_FUNCTION_KEY and key < FIRST_FUNCTION_KEY + MAX_SUGGESTED_EMAILS:
                index = key - FIRST_FUNCTION_KEY
                if index < len(self.suggested_emails):
                    self.complete_email(index)
            elif key == FIRST_FUNCTION_KEY + 12 - 1:
                self.email_from = (self.email_from + 1) % len(config.EMAIL_FROM)
            else:
                # Ignore key.
                LOGGER.info("Unknown key during email entry: " + str(key))
            return True
        else:
            return False

    # Compute the current index based on the time.
    def get_current_photo_index(self):
        return int(math.floor(self.time/SLIDE_DISPLAY_S))

    # Get the photo by its index, where index can go on indefinitely.
    def photo_by_index(self, index):
        return self.photos[index % len(self.photos)] if self.photos else None

    # Returns a tuple with these items:
    #
    #     current slide index
    #     current slide (None if not yet loaded or couldn't load).
    #     time into current slide (always valid).
    #     next slide (None if not yet showing next slide, or not loaded).
    #     time into next slide (None if not yet showing next slide, otherwise negative).
    #
    def get_current_slide(self):
        photo_index = self.get_current_photo_index()
        if not self.photos:
            return photo_index, None, 0.0, None, 0.0

        current_slide = self.slide_cache.get(self.photo_by_index(photo_index))
        if current_slide and current_slide.swap_zoom is None:
            current_slide.swap_zoom = photo_index % 2 == 0
        current_time_offset = self.time - photo_index*SLIDE_DISPLAY_S

        if current_time_offset >= SLIDE_DISPLAY_S - SLIDE_TRANSITION_S:
            next_slide = self.slide_cache.get(self.photo_by_index(photo_index + 1))
            if next_slide and next_slide.swap_zoom is None:
                next_slide.swap_zoom = (photo_index + 1) % 2 == 0
            next_time_offset = current_time_offset - SLIDE_DISPLAY_S
        else:
            next_slide = None
            next_time_offset = None

        return photo_index, current_slide, current_time_offset, next_slide, next_time_offset

    # Prefetch the next n photos.
    def prefetch(self, n):
        if not self.photos:
            return

        photo_index = self.get_current_photo_index()
        for i in range(n):
            slide = self.slide_cache.get(self.photo_by_index(photo_index + i))
            if slide:
                # Consider the prefetched slides touched because we always prefer them
                # to the oldest slides.
                slide.touch()

    def draw(self):
        _, current_slide, _, next_slide, _ = self.get_current_slide()

        # Check configured here because there's a chance that we'll
        # get a loaded slide from the loader between the move and
        # the draw, meaning that the draw will happen before it's
        # been positioned. Skip drawing in case the bad positioning
        # causes problems (despite zero alpha).
        if current_slide and not current_slide.is_broken and current_slide.configured:
            # Draw first slide further back. Note that we only need this
            # because in pi3d's Buffer.py file they enable GL_DEPTH_TEST.
            # They don't provide a way to disable it, and it doesn't
            # seem to hurt our frame rate. If we ever want to go faster,
            # we could fix that.
            current_slide.positionZ(0.2)
            current_slide.draw()
        if next_slide and not next_slide.is_broken and next_slide.configured:
            next_slide.positionZ(0.1)
            next_slide.draw()

        # Any slide we didn't draw we should mark as not configured so that
        # next time it's drawn it can jump immediately to its initial position.
        for slide in self.slide_cache.get_slides():
            if slide is not current_slide and slide is not next_slide and not slide.is_broken:
                slide.turn_off()

        # Upper-left:
        if self.prompting_email:
            self.draw_email_prompt()
        elif self.show_debug:
            self.draw_debug()
        elif self.showing_bus:
            self.draw_bus()

        # Upper-right:
        self.draw_time()
        self.draw_sonos()

    def draw_debug(self):
        # Map from photo ID to Slide.
        cache = self.slide_cache.get_cache()
        current_photo_index, _, _, _, _ = self.get_current_slide()
        unused_keys = set(self.debug_cache.keys())

        x = -self.screen_width/2 + DISPLAY_MARGIN
        y = self.screen_height/2 - DISPLAY_MARGIN
        for photo_index in range(current_photo_index - 5, current_photo_index + 5):
            photo = self.photo_by_index(photo_index)
            if not photo:
                continue
            slide = self.slide_cache.get(photo, False)
            text = "%d: %s" % (photo_index, slide)
            if photo_index == current_photo_index:
                text += " (current)"

            label = self.debug_cache.get(photo.id)
            if label:
                unused_keys.discard(photo.id)
            else:
                label = CachedString(self.shader, TEXT_FONT, 0.05, "L")
                self.debug_cache[photo.id] = label

            string = label.get(text, x, y)
            string.draw()

            y -= 60

        # Purge cache of unused items.
        for photo_id in unused_keys:
            del self.debug_cache[photo_id]

    def draw_email_prompt(self):
        x = -self.screen_width/2 + DISPLAY_MARGIN
        y = self.screen_height/2 - DISPLAY_MARGIN

        # Draw "from" address.
        text = "From " + config.EMAIL_FROM[self.email_from]["first_name"] + " (F12)"
        string = self.email_from_label.get(text, x, y)
        string.draw()
        y -= EMAIL_LEADING

        # Draw prompt.
        if self.email_error:
            prompt = "Error sending to:"
        else:
            prompt = "Email addresses:"
        string = self.email_prompt_label.get(prompt, x, y)
        string.draw()
        y -= EMAIL_LEADING

        # Add cursor.
        text = self.email_address
        if int(time.time() * 4) % 2 == 0:
            text += "_"

        # Draw email.
        string = self.email_label.get(text, x, y)
        string.draw()
        y -= EMAIL_LEADING
        y -= EMAIL_LEADING

        # Draw suggestions.
        if not self.suggested_emails:
            text = "(No matching emails)"
            string = self.suggested_email_labels[0].get(text, x, y)
            string.draw()
            y -= EMAIL_LEADING
        else:
            for i, (suggested_email, already_emailed) in enumerate(self.suggested_emails):
                if i < len(self.suggested_email_labels):
                    text = "F%-2d %s" % (i + 1, suggested_email)
                    if already_emailed:
                        text += " (already emailed)"
                    string = self.suggested_email_labels[i].get(text, x, y)
                    string.draw()
                    y -= EMAIL_LEADING


    def draw_time(self):
        mtime = time.localtime(time.time())
        text = time.strftime("%-I:%M", mtime)
        x = self.screen_width/2 - DISPLAY_MARGIN
        y = self.screen_height/2 - TIME_TOP_DISPLAY_MARGIN
        string = self.time_label.get(text, x, y)
        string.draw()

    def draw_sonos(self):
        if not self.sonos:
            return

        sonos_status = self.sonos.get_status()
        if sonos_status:
            self.sonos_status = sonos_status

        title = ""
        artist = ""
        stations = None
        if self.sonos_status:
            if self.sonos_status.playing:
                title = self.sonos_status.title
                artist = self.sonos_status.artist
            if self.sonos_status.data:
                if self.sonos_status.data.data_type == sonos.DATA_TYPE_RADIO_STATIONS:
                    stations = self.sonos_status.data.data

        x = self.screen_width/2 - DISPLAY_MARGIN
        y = self.screen_height/2 - TRACK_TOP_DISPLAY_MARGIN
        string = self.track_label.get(title, x, y)
        string.draw()

        y -= MUSIC_LEADING
        string = self.artist_label.get(artist, x, y)
        string.draw()

        if stations:
            x = -self.screen_width/2 + DISPLAY_MARGIN
            y = self.screen_height/2 - DISPLAY_MARGIN
            for i in range(min(len(stations), len(self.station_labels))):
                label = "F%d: %s" % (i + 1, stations[i])
                string = self.station_labels[i].get(label, x, y)
                string.draw()
                y -= STATION_LEADING

    def draw_bus(self):
        x = -self.screen_width/2 + DISPLAY_MARGIN
        y = self.screen_height/2 - DISPLAY_MARGIN

        # Fetch the most recent prediction and cache it.
        new_predictions = self.nextbus_fetcher.get_predictions()
        if new_predictions:
            self.bus_predictions = new_predictions

        # Show predictions if we got any.
        if self.bus_predictions and self.bus_predictions.predictions:
            predictions = self.bus_predictions.predictions

            string = self.bus_route_label.get(predictions[0].direction.route.title, x, y)
            string.draw()
            y -= BUS_MAJOR_LEADING

            string = self.bus_direction_label.get(predictions[0].direction.title, x, y)
            string.draw()
            y -= BUS_MINOR_LEADING

            string = self.bus_stop_label.get(self.bus_predictions.stop_title, x, y)
            string.draw()
            y -= BUS_MINOR_LEADING

            for i in range(min(len(self.bus_prediction_labels), len(predictions))):
                pred = predictions[i]
                local_time = time.localtime(pred.epoch_time/1000)
                time_label = time.strftime("%-I:%M%P", local_time)
                rel_label = "arriving" if pred.minutes == 0 else "%d min" % pred.minutes
                label = "%s (%s)" % (time_label, rel_label)
                string = self.bus_prediction_labels[i].get(label, x, y)
                string.draw()
                y -= BUS_MINOR_LEADING
        elif self.bus_start_time is not None and time.time() - self.bus_start_time >= 0.5:
            # Show something while waiting for the first prediction.
            string = self.bus_route_label.get(". . .", x, y)
            string.draw()
            y -= BUS_MAJOR_LEADING

    # See if any images came in from the Twilio thread. If so, process them
    # and show them next.
    def fetch_twilio_photos(self):
        if self.twilio_fetcher:
            pathname = self.twilio_fetcher.get_photo_pathname()
            if not pathname:
                return

            # Compute hash, add to database. This takes a bit of time and
            # causes a hiccup in the animation. We'd have to split the hash
            # and the database work to avoid that.
            photo_id = handle_new_and_renamed_file(self.con, pathname)

            # Fetch back from database and fix up pathname.
            photo = db.get_photo_by_id(self.con, photo_id)
            if photo:
                photo.pathname = pathname

                # Get global photo index. This number goes on forever, not wrapping.
                photo_index = self.get_current_photo_index()

                if self.photos:
                    # We want that "photo_index + 1" have this new photo. Since we
                    # wrap all index numbers by the photo count and the photo count
                    # is about to go up by one, we must adjust our concept of time
                    # to pretend there's been this many photos all along.
                    wrapped_photo_index = photo_index % len(self.photos)

                    # The number of virtual photos that were inserted in the past.
                    insert_count = photo_index / len(self.photos)

                    # Adjust the time by this number of virtual photos.
                    self.time += insert_count*SLIDE_DISPLAY_S

                    # Insert into our array.
                    self.photos.insert(wrapped_photo_index + 1, photo)

                    # Prefetch it.
                    self.prefetch(1)
                else:
                    # First photo, just add it.
                    self.photos.append(photo)
                    self.time = 0

    def rotate_clockwise(self):
        _, slide, _, _, _ = self.get_current_slide()
        if slide and not slide.is_broken:
            slide.photo.rotation -= 90
            slide.persist_state(self.con)
            slide.compute_ideal_size(self.screen_width, self.screen_height)
        self.jump_relative(0)

    def rotate_counterclockwise(self):
        _, slide, _, _, _ = self.get_current_slide()
        if slide and not slide.is_broken:
            slide.photo.rotation += 90
            slide.persist_state(self.con)
            slide.compute_ideal_size(self.screen_width, self.screen_height)
        self.jump_relative(0)

    def jump_relative(self, delta_slide):
        _, _, time_offset, _, _ = self.get_current_slide()
        self.time += delta_slide*SLIDE_DISPLAY_S - time_offset
        if self.time < 0:
            self.time = 0

    def toggle_debug(self):
        self.show_debug = not self.show_debug

    def toggle_pause(self):
        self.paused = not self.paused
        if self.paused:
            self.pause_start_time = time.time()
        else:
            self.pause_start_time = None

    def move(self):
        # Amount of time since last frame.
        now = time.time()
        delta_time = now - self.previous_frame_time if self.previous_frame_time else 0
        self.previous_frame_time = now

        # Auto-disable paused after a while.
        if self.paused and self.pause_start_time is not None and time.time() - self.pause_start_time >= MAX_PAUSE_SECONDS:
            self.paused = False
            self.pause_start_time = None

        # Auto-disable bus after a while.
        if self.showing_bus and self.bus_start_time is not None and time.time() - self.bus_start_time >= MAX_BUS_SECONDS:
            self.set_show_bus(False)

        # Auto-disable Twilio after a while.
        if self.fetching_twilio and self.twilio_start_time is not None and time.time() - self.twilio_start_time >= MAX_TWILIO_SECONDS:
            self.set_fetch_twilio(False)

        # Advance time if not paused.
        if not self.paused:
            self.time += delta_time

        photo_index, slide, time_offset, next_slide, next_time_offset = self.get_current_slide()
        if slide and not slide.is_broken:
            slide.move(self.paused, self.prompting_email, time_offset)
        if next_slide and not next_slide.is_broken:
            next_slide.move(self.paused, self.prompting_email, next_time_offset)

    def mute(self):
        if self.sonos:
            self.sonos.send_command("mute")

    def stop_music(self):
        if self.sonos:
            self.sonos.send_command("stop")

    def play_radio_station(self, index):
        if self.sonos:
            self.sonos.send_command("play_radio_station", index)

    # Show the email prompt.
    def prompt_email(self):
        self.paused = True
        self.pause_start_time = None
        self.prompting_email = True
        self.email_error = False
        self.email_address = ""
        self.update_suggested_emails()

    # Returns whether successful.
    def email_photo(self, email_addresses):
        _, current_slide, _, _, _ = self.get_current_slide()
        title = ", ".join(filter(None, [current_slide.photo.label, current_slide.photo.display_date]))

        # Send the high-res version.
        absolute_pathname = os.path.join(config.ROOT_DIR, current_slide.photo.pathname)

        # Emails can be comma-separated.
        email_address_list = [email_address.strip() for email_address in email_addresses.split(",")]

        # Remove empty addresses (from trailing commas, etc.).
        email_address_list = filter(None, email_address_list)

        LOGGER.info("Emailing " + absolute_pathname + " to " + str(email_address_list))

        success = True
        try:
            email_service.send_email(title, absolute_pathname, config.EMAIL_FROM[self.email_from], email_address_list)
        except Exception as e:
            LOGGER.warn("Got exception emailing to \"%s\" (%s)" % (email_address_list, e))
            success = False

        if success:
            for email_address in email_address_list:
                db.email_sent(self.con, current_slide.photo.id, email_address)

        return success

    def update_suggested_emails(self):
        # Get current slide pathname.
        _, current_slide, _, _, _ = self.get_current_slide()

        # Pick the last of the comma-separated email addresses.
        email_address = self.email_address.split(",")[-1].strip()

        # Get top email addresses for this prefix.
        suggested_emails = db.get_emails_with_prefix(self.con, email_address, MAX_SUGGESTED_EMAILS)

        # Figure out who's been emailed this photo.
        suggested_emails = [(email_address, db.has_photo_been_emailed_by(self.con, current_slide.photo.id, email_address)) for email_address in suggested_emails]

        self.suggested_emails = suggested_emails

    def complete_email(self, index):
        new_email_address, _ = self.suggested_emails[index]

        i = self.email_address.rfind(",")
        if i == -1:
            # Replace only component.
            self.email_address = new_email_address + ", "
        else:
            # Replace last component.
            self.email_address = self.email_address[:i + 1] + " " + new_email_address + ", "
        self.update_suggested_emails()
        self.email_error = False

    def rate_photo(self, rating):
        # Rating is a number [1..5].
        _, slide, _, _, _ = self.get_current_slide()
        if slide and not slide.is_broken:
            slide.photo.rating = rating
            slide.persist_state(self.con)

            # Auto-skip to the next one if we don't like this one.
            if rating == 1 or rating == 2:
                self.jump_relative(1)

    def toggle_bus(self):
        self.set_show_bus(not self.showing_bus)

    def set_show_bus(self, show_bus):
        if self.nextbus_fetcher:
            self.showing_bus = show_bus
            if self.showing_bus:
                self.bus_start_time = time.time()
            else:
                self.bus_start_time = None
            self.bus_predictions = None
            self.nextbus_fetcher.set_fetching(show_bus)

    def toggle_twilio(self):
        self.set_fetch_twilio(not self.fetching_twilio)

    def set_fetch_twilio(self, fetch_twilio):
        if self.twilio_fetcher:
            self.fetching_twilio = fetch_twilio
            if self.fetching_twilio:
                self.twilio_start_time = time.time()
            else:
                self.twilio_start_time = None
            self.twilio_fetcher.set_fetching(fetch_twilio)

# Must remove these in-place.
def remove_unwanted_dirs(dirnames):
    for dirname in config.UNWANTED_DIRS:
        try:
            dirnames.remove(dirname)
        except ValueError:
            # Ignore.
            pass

# Gets a set of all pathnames in image directory. These are relative to the passed-in root dir.
def get_tree_photos(root_dir):
    tree_pathnames = set()

    LOGGER.debug("Scanning photo directories...")
    print "Scanning photo directories..."
    before = time.time()
    for dirpath, dirnames, filenames in os.walk(root_dir):
        remove_unwanted_dirs(dirnames)
        if filenames:
            for filename in filenames:
                if is_image(filename):
                    pathname = os.path.join(dirpath, filename)
                    pathname = unicode(pathname, "utf8")
                    tree_pathnames.add(os.path.relpath(pathname, root_dir))
    after = time.time()

    # Filter for debugging.
    if SUBDIR:
        tree_pathnames = set(pathname for pathname in tree_pathnames if pathname.startswith(SUBDIR + "/"))

    LOGGER.debug("There are %d photo files. Scanning directories took %.1f seconds." %
            (len(tree_pathnames), after - before))

    return tree_pathnames

# Get photos of at least this rating.
def filter_photos_by_rating(db_photos, min_rating):
    return [photo for photo in db_photos if photo.rating >= min_rating]

# Get photos within this time range.
def filter_photos_by_date(db_photos, min_days, max_days):
    min_seconds = min_days*24*60*60 if min_days else None
    max_seconds = max_days*24*60*60 if max_days else None
    now = time.time()

    def is_in_date_range(age, min_age, max_age):
        return (min_age is None or age >= min_age) and \
                (max_age is None or age <= max_age)

    return [photo for photo in db_photos if is_in_date_range(now - photo.date, min_seconds, max_seconds)]

# Return the db_photos array filtered by which pathnames contain the substring.
def filter_photos_by_substring(db_photos, substring):
    if substring is None or substring == "":
        return db_photos
    return [photo for photo in db_photos
            if substring in photo.pathname or
                (config.PARTY_MODE and photo.pathname.startswith(config.TWILIO_SUBDIR + "/"))]

# Look for new images or images that might have been moved or renamed in the tree.
# We must make sure to not lose the metadata (rating, rotation).
def handle_new_and_renamed_files(con, tree_pathnames, db_photo_files):
    # Map of photo files.
    db_pathnames = dict((photo_file.pathname, photo_file) for photo_file in db_photo_files)

    # Figure out which pathnames we need to analyze.
    pathnames_to_do = tree_pathnames - set(db_pathnames.keys())

    print "Analyzing %d new or renamed photos..." % len(pathnames_to_do)
    for pathname in pathnames_to_do:
        handle_new_and_renamed_file(con, pathname)

# Create a new photo file for this pathname, and optionally a new photo.
# Returns the photo ID (new or old).
def handle_new_and_renamed_file(con, pathname):
    print "    Computing hash for " + pathname
    label = clean_pathname(pathname)

    # We want the hashes of the original files, not the processed ones.
    absolute_pathname = os.path.join(config.ROOT_DIR, pathname)

    image_bytes = open(absolute_pathname).read()
    hash_all = sha.sha(image_bytes).hexdigest()
    hash_back = sha.sha(image_bytes[-1024:]).hexdigest()

    # Create a new photo file.
    photo_file = PhotoFile(pathname, hash_all, hash_back)
    db.save_photo_file(con, photo_file)

    # Now see if this was a renaming of another file.
    photo = db.get_photo_by_hash_back(con, hash_back)
    if not photo:
        # New photo.
        print "        New photo."
        rotation = get_file_rotation(absolute_pathname) or 0
        rating = 3
        mtime = os.path.getmtime(absolute_pathname)
        display_date = mtime_to_string(mtime)
        photo_id = db.create_photo(con, hash_back, rotation, rating, mtime, display_date, label)
    else:
        # Renamed or moved photo.
        print "        Renamed or moved photo."
        # Leave the timestamp the same, but update the label.
        photo.label = label
        db.save_photo(con, photo)
        photo_id = photo.id

    return photo_id

# Return the file's rotation in degrees, or None if it cannot be determined.
def get_file_rotation(absolute_pathname):
    rotation = None

    try:
        image = Image.open(absolute_pathname)
    except IOError:
        image = None

    if image and hasattr(image, "_getexif"):
        try:
            exif = image._getexif()
        except Exception as e:
            exif = None
            print "Exception getting EXIF data for %s: %s" % (absolute_pathname, e)
        if exif:
            exif_rotation = exif.get(0x0112, 1)
            if exif_rotation == 6:
                rotation = -90
            elif exif_rotation == 8:
                rotation = 90

    return rotation

# Assign a pathname to each photo, returning a new copy of db_photos with
# invalid photos (those with no disk files) removed.
def assign_photo_pathname(con, db_photos, tree_pathnames):
    # Get all photo files. We did this before but have since modified the DB.
    photo_files = db.get_all_photo_files(con)

    # Map from hash to list of photo files.
    photo_file_map = collections.defaultdict(list)
    for photo_file in photo_files:
        photo_file_map[photo_file.hash_back].append(photo_file)

    warning_count = 0

    # Handle each photo, making new array of photos.
    good_photos = []
    for photo in db_photos:
        # Try every photo file.
        for photo_file in photo_file_map[photo.hash_back]:
            # See if it's on disk.
            if photo_file.pathname in tree_pathnames:
                photo.pathname = photo_file.pathname
                good_photos.append(photo)
                break
        else:
            # Can't find any file on disk for this photo.
            warning_count += 1
            if warning_count <= MAX_NO_FILE_WARNINGS:
                print "No file on disk for %s (%s)" % (photo.hash_back, photo.label)

    if warning_count:
        print "Files missing on disk: %d" % warning_count

    return good_photos

def main():
    global g_frame_count

    # Parse command-line parameters.
    parser = argparse.ArgumentParser()
    parser.add_argument("--min_rating", help="minimum rating", type=int, default=3)
    parser.add_argument("--min_days", help="min age of photo", metavar="DAYS", type=int)
    parser.add_argument("--max_days", help="max age of photo", metavar="DAYS", type=int)
    parser.add_argument("--includes", help="only include these paths", metavar="DIR", type=str)
    args = parser.parse_args()

    # Open the database.
    con = db.connect()
    db.upgrade_schema(con)

    # Read photo directory tree. This is a set that represents the photos that
    # are actually available on disk.
    tree_pathnames = get_tree_photos(config.ROOT_DIR)

    # Resize images.
    preprocess.process_trees(config.ROOT_DIR, config.PROCESSED_ROOT_DIR, tree_pathnames)

    # Get photo files from database.
    db_photo_files = db.get_all_photo_files(con)

    # Compute missing hashes of photos, add them to database.
    handle_new_and_renamed_files(con, tree_pathnames, db_photo_files)

    # Keep only photos of the right rating and date range.
    db_photos = db.get_all_photos(con)
    print "Total photos: %d" % (len(db_photos),)
    db_photos = filter_photos_by_rating(db_photos, args.min_rating)
    print "Photos after rating filter: %d" % (len(db_photos),)
    db_photos = filter_photos_by_date(db_photos, args.min_days, args.max_days)
    print "Photos after date filter: %d" % (len(db_photos),)

    # Find a pathname for each photo.
    db_photos = assign_photo_pathname(con, db_photos, tree_pathnames)
    print "Photos after disk filter: %d" % (len(db_photos),)
    db_photos = filter_photos_by_substring(db_photos, args.includes)
    print "Photos after dir filter: %d" % (len(db_photos),)

    if not db_photos:
        print "Error: No photos found."
        sys.exit(0)

    # Shuffle the photos.
    random.shuffle(db_photos)

    # Initialize pi3d.
    display = pi3d.Display.create(background=(0.0, 0.0, 0.0, 1.0), frames_per_second=40)
    shader = pi3d.Shader("uv_flat")
    keyboard = pi3d.Keyboard()
    camera = pi3d.Camera(is_3d=False)
    # To save a tiny bit of work each loop:
    camera.was_moved = False

    # Load the star icon.
    star_texture = pi3d.Texture("outline-star-256.png", blend=True, mipmap=True)
    star_sprite = pi3d.ImageSprite(star_texture, shader, camera=camera, w=1.0, h=1.0, z=0.05)
    star_sprite.set_alpha(0.5)
    LOGGER.info("Star icon: %dx%d" % (star_texture.ix, star_texture.iy))

    # Get screen info.
    screen_size = (display.width, display.height)
    LOGGER.info("Screen size: %dx%d" % screen_size)

    slideshow = Slideshow(db_photos, display, shader, star_sprite, screen_size, con)
    if config.PARTY_MODE:
        slideshow.set_fetch_twilio(True)

    while display.loop_running():
        slideshow.prefetch(MAX_CACHE_SIZE/2 + 1)
        slideshow.move()
        slideshow.fetch_twilio_photos()
        slideshow.draw()
        g_frame_count += 1

        key = keyboard.read()
        if key != -1:
            took_key = slideshow.take_key(key)
            if not took_key:
                if key == ord("Q"):
                    slideshow.shutdown()
                    keyboard.close()
                    display.stop()
                elif key == ord("r"):
                    slideshow.rotate_clockwise()
                elif key == ord("l"):
                    slideshow.rotate_counterclockwise()
                elif key == ord(" "):
                    slideshow.toggle_pause()
                elif key == 261: # Right arrow.
                    slideshow.jump_relative(1)
                elif key == 260: # Left arrow.
                    slideshow.jump_relative(-1)
                elif key == ord("D"):
                    slideshow.toggle_debug()
                elif key == ord("m"):
                    slideshow.mute()
                elif key == ord("s"):
                    slideshow.stop_music()
                elif key == ord("e"):
                    slideshow.prompt_email()
                elif key == ord("b"):
                    slideshow.toggle_bus()
                elif key == ord("T"):
                    slideshow.toggle_twilio()
                elif key >= ord("1") and key <= ord("5"):
                    slideshow.rate_photo(key - ord("1") + 1)
                elif key >= FIRST_FUNCTION_KEY and key < FIRST_FUNCTION_KEY + 10:
                    slideshow.play_radio_station(key - FIRST_FUNCTION_KEY)
                else:
                    LOGGER.info("Got unknown key %d" % key)

    display.destroy()
    db.disconnect(con)

if __name__ == "__main__":
    LOGGER = configure_logger()
    main()
