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

# Interfaces with the Sonos music system.

import soco
import Queue
import threading
import logging
import time

STATUS_POLL_INTERVAL = 1

TUNEIN_METADATA_TEMPLATE = """
<DIDL-Lite xmlns:dc="http://purl.org/dc/elements/1.1/"
    xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/"
    xmlns:r="urn:schemas-rinconnetworks-com:metadata-1-0/"
    xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/">
    <item id="R:0/0/0" parentID="R:0/0" restricted="true">
        <dc:title>{title}</dc:title>
        <upnp:class>object.item.audioItem.audioBroadcast</upnp:class>
        <desc id="cdudn" nameSpace="urn:schemas-rinconnetworks-com:metadata-1-0/">
            {service}
        </desc>
    </item>
</DIDL-Lite>' """

DATA_TYPE_RADIO_STATIONS = 1

# Sending commands to the Sonos can block, so we do all that in a separate
# thread and use queues to send commands and get status back.
class SonosController(object):
    def __init__(self):
        self.logger = logging.getLogger(__name__)
        self.logger.setLevel(logging.DEBUG)

        self.players = []
        self.player = None
        self.thread = None
        self.running = True

        # SonosCommand objects:
        self.command_queue = Queue.Queue()

        # SonosStatus objects:
        self.status_queue = Queue.Queue()

    def start(self):
        self.thread = threading.Thread(target=self._loop)
        self.thread.daemon = True
        self.thread.start()

    def stop(self):
        # I think this is thread-safe.
        self.running = False

        # Exit the get().
        self.send_command(None)

        if self.thread:
            self.thread.join()

    # Commands are:
    #     "mute": Mute the music (toggle).
    #     "stop": Stop the music.
    #     "play_radio_station": Play radio station N (first argument) of favorite radio stations.
    def send_command(self, command, *args):
        self.command_queue.put(SonosCommand(command, *args))

    def get_status(self):
        try:
            return self.status_queue.get_nowait()
        except Queue.Empty:
            return None

    # Runs in the other thread.
    def _loop(self):
        self.players = list(soco.discover() or [])

        # Go into party mode immediately if not already in it.
        if self.players and len(self.players[0].group.members) != len(self.players):
            self.players[0].partymode()
        self.player = self.players[0].group.coordinator

        while self.running:
            try:
                status = self._fetch_status()
                if status:
                    self.status_queue.put(status)

                try:
                    command = self.command_queue.get(True, STATUS_POLL_INTERVAL)
                    self._process_command(command)
                except Queue.Empty:
                    # No problem, ignore.
                    pass
            except BaseException as e:
                # Probably connection error. Wait a bit and try again.
                self.logger.warning("Got exception in Sonos thread: %s" % e)
                time.sleep(60)

        self.logger.info("Exiting Sonos loop")

    def _process_command(self, command):
        if not command.command or not self.player:
            return

        self.logger.info("Processing command: " + command.command)

        if command.command == "mute":
            mute = self.player.mute
            for player in self.players:
                player.mute = not mute
        elif command.command == "stop":
            self.player.stop()
        elif command.command == "play_radio_station":
            index = command.args[0]
            response = self.player.get_favorite_radio_stations()
            stations = response["favorites"]
            data = SonosData(DATA_TYPE_RADIO_STATIONS, [station["title"] for station in stations])
            if index >= 0 and index < len(stations):
                station = stations[index]
                title = station["title"]
                self.status_queue.put(SonosStatus.make_message("Switching to " + title, data))
                metadata = TUNEIN_METADATA_TEMPLATE.format(title=title, service="SA_RINCON65031_")
                uri = station["uri"]
                uri = uri.replace("&", "&amp;")
                self.player.play_uri(uri, metadata)

    def _fetch_status(self):
        if not self.player:
            return None

        # Find out what's playing.
        track_info = self.player.get_current_track_info()
        if not track_info:
            # I think this is generally not None.
            return None

        # Don't distinguish between paused and stop. Either way show no UI.
        transport_info = self.player.get_current_transport_info()
        playing = transport_info and transport_info.get("current_transport_state") == "PLAYING"

        return SonosStatus(
                track_info["album"],
                track_info["artist"],
                track_info["title"],
                track_info["duration"],
                track_info["position"],
                track_info["album_art"],
                playing)

# Command to the Sonos.
class SonosCommand(object):
    def __init__(self, command, *args):
        self.command = command
        self.args = args

# Status from the Sonos.
class SonosStatus(object):
    def __init__(self, album, artist, title, duration, position, album_art, playing, data=None):
        self.album = album
        self.artist = artist
        self.title = title
        self.duration = duration
        self.position = position
        self.album_art = album_art
        self.playing = playing
        self.data = data

    @staticmethod
    def make_message(message, data=None):
        return SonosStatus("", "", message, 0, 0, "", True, data)

    def __str__(self):
        return "%s from %s (%s)" % (self.title, self.artist, self.position)

class SonosData(object):
    def __init__(self, data_type, data):
        # Possible type values:
        # DATA_TYPE_RADIO_STATIONS: List of radio station names.
        self.data_type = data_type
        self.data = data

# Command-line test.
def main():
    controller = SonosController()

    # Start the thread.
    controller.start()

    # Loop and print the status.
    while True:
        status = controller.get_status()
        if status:
            print status
        time.sleep(1)

if __name__ == "__main__":
    main()
