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

# AWS SES (Simple Email Service) client.

# AWS creds are in ~/.aws/credentials.

# pip install boto3
import boto3

import os
import mimetypes
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.image import MIMEImage

# Set to False to test emails without attachments.
ATTACH_PHOTO = True

# Returns whether successful.
def send_email(title, absolute_pathname, email_from, email_address_list):
    client = boto3.client("ses")

    response = client.send_raw_email(
            Source=email_from["email_address"],
            Destinations=email_address_list,
            RawMessage={
                "Data": make_email(email_from, email_address_list, title, absolute_pathname),
            },
        )

def make_email(email_from, email_address_list, title, absolute_pathname):
    from_name = email_from["name"]
    from_first_name = email_from["first_name"]
    from_email_address = email_from["email_address"]

    message = MIMEMultipart("mixed")
    message.set_charset("UTF-8")
    message["Subject"] = "Photo (" + title + ")"
    message["From"] = "%s <%s>" % (from_name, from_email_address)
    message["To"] = ", ".join(email_address_list)
    message.attach(MIMEText("Thinking of you...\n\n%s" % (from_first_name,), "plain"))
    if ATTACH_PHOTO:
        data = open(absolute_pathname).read()
        filename = os.path.basename(absolute_pathname)
        message.attach(MIMEImage(data, name=filename))
    return message.as_string()

if __name__ == "__main__":
    # Send test email.
    absolute_pathname = "/filename.jpg"
    email_from = {
        "name": "My Name",
        "first_name": "My First Name",
        "email_address": "from@example.com",
    }
    send_email(email_from, "TITLE HERE", absolute_pathname, ["to@example.com"])

