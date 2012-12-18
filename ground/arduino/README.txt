UAVTalk for Arduino
-------------------

THIS PORT IS ABANDONED. PUSHED BY USER DEMAND BUT IS VERY OLD. DON'T EXPECT A LOT FROM IT.
Lot of changes was done to the main code. Feel free to port if you are interested in that.


This directory contains the support libraries for an Arduino port of the
UAVTalk messaging protocol.  This code is based on the flight version of
UAVTalk, and has a minimal amount of changes.

The steps for using this library are:

1) Copy the subdirectories under ground/arduino/libraries into your
   arduino libraries directory.
2) Build the auto-generated code by either using "make all", or just
   "make uavobjects_arduino"
3) Copy the generated code in build/uavobject-synthetics/arduino/*
   into your libraries directory:

  mkdir <arduino>/libraries/UAVObjects
  cp build/uavobject-synthetics/arduino/UAVObjects/* <arduino>/libraries/UAVObjects

4) Create a sketch that uses the library.  the following is an example:

#include <PIOS_CRC.h>
#include <UAVTalk.h>
#include <UAVObj.h>
#include <UAVObjects.h>

int32_t WriteMsg(uint8_t *data, int32_t length) {
  Serial.write(data, length);
}

void ObjEventCallback(UAVObjEvent* ev) {
  uint32_t id = UAVObjGetID(ev->obj);
  switch(ev->event) {
  case EV_UNPACKED:
    //ns.print("Unpacked event for ");
    //ns.println(id, HEX);
    break;
  case EV_UPDATED:
  case EV_UPDATED_MANUAL:
    //ns.print("Updated event for ");
    //ns.println(id, HEX);
    UAVTalkSendObject(ev->obj, ev->instId, 1, 100);
    break;
  case EV_UPDATE_REQ:
    //ns.print("Update request event for ");
    //ns.println(id, HEX);
    break;
  }
}

void setup() {
  // Initailize the serial port(s)
  Serial.begin(57600);

  // Initialize UAVTalk
  UAVTalkInitialize(WriteMsg);
  UAVObjInitialize();

  // Initialize the UAVTalk objects that you need.
  GCSTelemetryStatsInitialize();
  GCSTelemetryStatsConnectCallback(ObjEventCallback);
}

void loop() {
  if(Serial.available()) {
    UAVTalkProcessInputStream(Serial.read());
  }
}
