#!/usr/bin/env python

#  Generate a version blob for
# the OpenPilot firmware.
#

import binascii
import os
from time import time

file = open("test.bin","wb")
# Get the short commit tag of the current git repository.
# It is 7 characters long, we pad with a zero, we end up with a
# 4 byte (int32) value:
hs= os.popen('git rev-parse --short HEAD').read().strip() + "0"
print "Version: " + hs
hb=binascii.a2b_hex(hs)
file.write(hb)
# Then the Unix time into a 32 bit integer:
print "Date: "  + hex(int(time())).lstrip('0x')
hb = binascii.a2b_hex(hex(int(time())).lstrip('0x'))
file.write(hb)
file.close()


