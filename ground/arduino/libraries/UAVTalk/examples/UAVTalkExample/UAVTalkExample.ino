/*
 * UAVTalkExample
 *
 * Bare minimum OpenPilot UAVTalk example program which can send and receive
 * UAVTalk packets through the serial port. It does not handle connection
 * states and only shows how to initialize the UAVTalk and send/receive
 * UAVObjects.
 *
 * Note that UAVTalk requires some RAM to register objects of interest.
 * This example was tested using ATmega128-based board with Wiring
 * compatible pin usage and CPU F=14.7456MHz to have accurate serial
 * port timings at 115200bps. This board is not supported by the Arduino
 * 1.0 by default, so you may want to try with Arduino Mega instead after
 * changing port rates to 38400 for exact timing.
 *
 * Serial is used as a debug (terminal) port.
 * Serial1 is the UAVTalk port connected to the OpenPilot board.
 * The board builtin LED will blink on every UAVTalk byte received.
 *
 * Reference board: http://wiring.org.co/download/wiring-Rev0004.pdf
 * More info about UAVTalk and OpenPilot: http://www.openpilot.org
 *
 * Sample output (some objects sent are ACKs for received ones):
 *
Initializing UAVTalk...
Initializing UAVObjects...
Setting callbacks...
UAVObject received: 2F7E2902
UAVObject received: 2F7E2902
UAVObject sent: ABC72744
5001: UAVTalkStats:
  txBytes: 48
  rxBytes: 60
  txObjectBytes: 21
  rxObjectBytes: 42
  txObjects: 3
  rxObjects: 2
  txErrors: 0
  rxErrors: 0
UAVObject received: 2F7E2902
UAVObject received: 2F7E2902
UAVObject received: 2F7E2902
UAVObject received: 2F7E2902
UAVObject sent: ABC72744
10002: UAVTalkStats:
  txBytes: 114
  rxBytes: 189
  txObjectBytes: 42
  rxObjectBytes: 126
  txObjects: 8
  rxObjects: 7
  txErrors: 0
  rxErrors: 0
UAVObject received: 2F7E2902
UAVObject received: 2F7E2902
UAVObject received: 2F7E2902
UAVObject received: 2F7E2902
UAVObject sent: ABC72744
15003: UAVTalkStats:
  txBytes: 180
  rxBytes: 318
  txObjectBytes: 63
  rxObjectBytes: 210
  txObjects: 13
  rxObjects: 12
  txErrors: 0
  rxErrors: 0
 */

#include <PIOS_CRC.h>
#include <UAVTalk.h>
#include <UAVObj.h>
#include <UAVObjects.h>

// Callback to send formatted object data to the UAVTalk port
int32_t WriteMsg(uint8_t *data, int32_t length) {
  Serial1.write(data, length);
}

// Callback for any object updates (local and remote)
void ObjEventCallback(UAVObjEvent *ev) {
  uint32_t id = UAVObjGetID(ev->obj);
  switch(ev->event) {
  case EV_UNPACKED:
    // Registered object received and unpacked.
    // Usually we may want to process it here.
    Serial.print("UAVObject received: ");
    Serial.println(id, HEX);
    break;
  case EV_UPDATED:
  case EV_UPDATED_MANUAL:
    // Local object updated and should be sent to remote.
    UAVTalkSendObject(ev->obj, ev->instId, 1, 0);
    Serial.print("UAVObject sent: ");
    Serial.println(id, HEX);
    break;
  case EV_UPDATE_REQ:
    // Object update request received. Usually we should send
    // current object value here.
    Serial.print("UAVObject update request for: ");
    Serial.println(id, HEX);
    break;
  }
}

// System setup
void setup() {
  // Initailize serial ports and LED
  Serial.begin(115200);  // debug (terminal) port
  Serial1.begin(115200); // UAVTalk port
  pinMode(LED_BUILTIN, OUTPUT);     

  // Initialize UAVTalk
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Initializing UAVTalk...");
  UAVTalkInitialize(WriteMsg);
  UAVObjInitialize();

  // Initialize the UAVTalk objects that you need
  Serial.println("Initializing UAVObjects...");
  GCSTelemetryStatsInitialize();
  FlightTelemetryStatsInitialize();

  // Set objects callbacks (using the same callback function)
  Serial.println("Setting callbacks...");
  GCSTelemetryStatsConnectCallback(ObjEventCallback);
  FlightTelemetryStatsConnectCallback(ObjEventCallback);

  digitalWrite(LED_BUILTIN, LOW);
}

// Main loop
void loop() {
  // Process input stream if new data have arrived
  while (Serial1.available()) {
    digitalWrite(LED_BUILTIN, HIGH);
    UAVTalkProcessInputStream(Serial1.read());
    digitalWrite(LED_BUILTIN, LOW);
  }

  // Here should be some code which updates some objects to be sent
  // to remote. Note that it should be done either periodically
  // or by some event. For this example we will send GCSTelemetryStats
  // every 5 seconds and display UAVTalk statistics.
  const unsigned long interval = 5000;
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;

    // Get current GCSTelemetryStats (they are not
    // updated in this example for simplicity)
    GCSTelemetryStatsData gcsStats;
    GCSTelemetryStatsGet(&gcsStats);

    // Set a field and send the object
    gcsStats.Status = GCSTELEMETRYSTATS_STATUS_DISCONNECTED;
    GCSTelemetryStatsSet(&gcsStats);

    // Show UAVTalk statistics
    UAVTalkStats uavStats;
    UAVTalkGetStats(&uavStats);

    Serial.print(currentMillis / 1000); Serial.println(": UAVTalkStats:");
    Serial.print("  txBytes: "); Serial.println(uavStats.txBytes);
    Serial.print("  rxBytes: "); Serial.println(uavStats.rxBytes);
    Serial.print("  txObjectBytes: "); Serial.println(uavStats.txObjectBytes);
    Serial.print("  rxObjectBytes: "); Serial.println(uavStats.rxObjectBytes);
    Serial.print("  txObjects: "); Serial.println(uavStats.txObjects);
    Serial.print("  rxObjects: "); Serial.println(uavStats.rxObjects);
    Serial.print("  txErrors: "); Serial.println(uavStats.txErrors);
    Serial.print("  rxErrors: "); Serial.println(uavStats.rxErrors);
  }
}

