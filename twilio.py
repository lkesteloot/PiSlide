
# Simple REST interface to the Twilio phone SMS service.

import urllib.request, urllib.parse, urllib.error
import urllib.request, urllib.error, urllib.parse
import base64
import json
import os

import config

# How often, in seconds, to fetch messages.
if config.PARTY_MODE:
    FETCH_PERIOD_S = 1
else:
    FETCH_PERIOD_S = 60

USER_AGENT = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36"
AUTHORIZATION = "Basic " + base64.b64encode(config.TWILIO_SID + ":" + config.TWILIO_TOKEN)

# A downloaded image.
class Image(object):
    def __init__(self, pathname, content_type, phone_number, date_sent):
        # Pathname of where the image was saved.
        self.pathname = pathname

        # Content type of the image.
        self.content_type = content_type

        # Phone number that sent the image.
        self.phone_number = phone_number

        # When the image was sent (string).
        self.date_sent = date_sent

# All information about this fetch.
class DownloadResults(object):
    def __init__(self, messages, images):
        # JSON structure of all messages available, for debugging.
        self.messages = messages

        # List of Image objects that were downloaded.
        self.images = images

# Returns the raw content of the specified Twilio-relative URL (after the
# domain name), or None of an error occurred. Set auth to True for most
# requests, False for JPEG requests.
def get_raw(url, auth, logger):
    url = "https://api.twilio.com" + url
    headers = {
            # We get a 403 fetching images without this:
            "User-Agent": USER_AGENT,
    }
    # Our auth information.
    if auth:
        headers["Authorization"] = AUTHORIZATION
    request = urllib.request.Request(url, None, headers)
    try:
        f = urllib.request.urlopen(request)
        data = f.read()
        f.close()
    except urllib.error.HTTPError as e:
        if logger:
            logger.warning("Can't fetch \"%s\" from Twilio (%s)" % (url, e,))
        return None

    return data

# Returns the JSON content of the specified Twilio-relative URL (after the
# domain name), or None of an error occurred.
def get_json(url, logger):
    j = get_raw(url, True, logger)
    return json.loads(j) if j else None

# Save the image at the specified Twilio-relative URL (after the
# domain name), or None of an error occurred. Returns whether
# successful.
def save_image(uri, pathname, logger):
    logger.info("Fetching photo to " + pathname)
    raw_image = get_raw(uri, False, logger)
    if not raw_image:
        return False

    try:
        f = open(pathname, "wb")
        f.write(raw_image)
        f.close()
    except IOError as e:
        if logger:
            logger.warning("Can't save \"%s\" (%s)" % (pathname, e,))
        return False

    return True

# Return all message information, or None if it can't be fetched.
def list_messages(logger):
    return get_json("/2010-04-01/Accounts/%s/Messages.json" % config.TWILIO_SID, logger)

# Delete the specified resource (message or media).
def delete_resources(uri, logger):
    url = "https://api.twilio.com" + uri
    headers = {
        "Authorization": "Basic " + base64.b64encode(config.TWILIO_SID + ":" + config.TWILIO_TOKEN),
    }
    request = urllib.request.Request(url, None, headers)
    request.get_method = lambda: "DELETE"
    try:
        f = urllib.request.urlopen(request)
        f.read()
        f.close()
    except urllib.error.HTTPError as e:
        if logger:
            logger.warning("Can't delete Twilio resource \"%s\" (%s)" % (uri, e,))

# Connects to Twilio, downloads the first few message, downloads their
# images to files on disk, and optionally deletes the images and messages.
#
# image_path: directory where to save the image
#
# Returns a DownloadResults object, or None if the messages can't be fetched.
def download_images(image_path, delete, logger):
    images = []

    messages = list_messages(logger)
    if not messages:
        return None

    for message in messages["messages"]:
        source_phone_number = message["from"]
        date_sent = message["date_sent"]
        # logger.info("Fetching message from %s" % source_phone_number)
        direction = message["direction"]
        num_media = int(message["num_media"])
        if direction == "inbound" and num_media > 0:
            subresource_uris = message.get("subresource_uris")
            if subresource_uris:
                media_list = subresource_uris.get("media")
                if media_list:
                    medias = get_json(media_list, logger)
                    for media in medias["media_list"]:
                        content_type = media["content_type"]
                        photo_sid = media["sid"]
                        uri = media["uri"]
                        if uri.endswith(".json") and content_type == "image/jpeg":
                            image_uri = uri[:-5]
                            pathname = os.path.join(image_path, photo_sid + ".jpg")
                            save_image(image_uri, pathname, logger)
                            images.append(Image(pathname, content_type, source_phone_number, date_sent))
                        if delete:
                            delete_resources(uri, logger)
        if delete:
            # We've seen cases where messages appeared in this list a few
            # seconds before their photos were available. If we catch it in
            # that window, we'll delete the message before we have a chance
            # to get the photo. So don't delete messages, just let them live
            # indefinitely.
            ## delete_resources(message["uri"], logger)
            pass

    return DownloadResults(messages, images)

def main():
    # As a test, this connects to the Twilio service, downloads all
    # the available photos, and delete all media and messages.
    delete = False

    import logging, pprint
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    logger.addHandler(logging.StreamHandler())

    results = download_images(".", delete, logger)
    pprint.pprint(results.messages)
    for image in results.images:
        print(("%s (%s): %s" % (image.phone_number, image.date_sent, image.pathname)))

if __name__ == "__main__":
    main()

