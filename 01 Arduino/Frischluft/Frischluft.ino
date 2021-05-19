/*
  ArduinoMqttClient - WiFi Sender

  Reference:
  https://create.arduino.cc/projecthub/officine-innesto/control-your-iot-cloud-kit-via-mqtt-and-node-red-114b4b?ref=user&ref_id=65561&offset=0

  Connects to a MQTT broker and publishes a sensor data once a time.
*/


/*
 * School Version V01_31.01
 * all sensors connected
 * issues with noise sensor
 */
 

//#define SENSOR_DEBUG
//#define DEBUG_VOC_AND_CO2
 

// from https://github.com/mpflaga/Arduino-MemoryFree see also https://playground.arduino.cc/Code/AvailableMemory/
#include <MemoryFree.h>
#include <pgmStrToRAM.h> // not needed for new way. but good to have for reference.

#ifdef ARDUINO_SAMD_MKRWIFI1010
#include <WiFiNINA.h>
#define WIFILIB    "WIFININA" 
#elif ARDUINO_SAMD_MKR1000
#include <WiFi101.h>
#define WIFILIB    "WIFI101" 
#else
#error unknown board
#endif

#include "arduino_secrets.h"

const int pinSoundSensor = A0;
const int pinAirQlSensor = A2;

const char ssid[]     = SECRET_SSID;    // Network SSID (name)
const char pass[]     = SECRET_PASS;    // Network password (use for WPA, or use as key for WEP)
int status            = WL_IDLE_STATUS;     // the Wifi radio's status

#define BROKER_IP         "192.168.12.50" // RPI4_03 SensorServer
const int   port        = 1883;
String      sendTopic   = "/arduino/";  
const int   sensorId    = 3;  
String      sensorLabel = "-";

#include "MQTT_Subs.h"  // include all stuff about subscriptions
#include "SensObj.h"    // include all stuff about the sensors, packed as a class SensObj 

SensObj sensor;


// Try to connect to SSID and Broker 
void connect() {
  int sleepTime;
  
  // wait a random shifttime (0-20sec), to avoid that two sensors connect to the broker in the same time intervals
  // inside NodeRed, as only one database is used, if two messages arrive in the same time interval, the broker takes only one of them
  // this timeinterval can be nanosec, milisec, sec, min, ... I use nanosec, but in order to increase the pobability of a different time
  // each arduino waits a random time (see classic example probability same birthday of students in a classroom) 
  sleepTime = random(20000);
  Serial.print("Sleeping ");
  Serial.print(sleepTime);
  Serial.println(" ms.");
  delay(sleepTime);
  
  
  lastMillis = lastMillis - sleepTime;
  
  Serial.println("Connecting.");
  nReconnections++;
  
    
  Serial.print("WifiStatus: ");
  Serial.println(WiFi.status());

  // connect to SSID
  if ( WiFi.status() != WL_CONNECTED) {
    nWifiLost++;
    while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
      // unsuccessful, retry in 4 seconds
      Serial.print("Failed connecting SSID ");
      Serial.println(ssid);
      delay(6000);
      Serial.println("retrying ... ");
    }
  }
  Serial.println("Wifi Connected.");
      

  // Connect to MQTT broker  
  if ((WiFi.status() == WL_CONNECTED) && (client.connect(myMacAddress.c_str(), MQTT_USER, MQTT_PASS))) {  
    Serial.print("Connected to connect to MQTT broker: ");
    Serial.println(BROKER_IP);
    Serial.print("ClientID: ");
    Serial.println(myMacAddress.c_str());
  
    setKeepAlive();
    setClientSubscriptions();
  }
  else{
    Serial.print("Unable to connect to MQTT broker: ");
    Serial.println(BROKER_IP);
    Serial.print("ClientID: ");
    Serial.println(myMacAddress.c_str());
  }
}


void setup() {
  IPAddress ip;                    // the IP address of my board

  Serial.begin(9600);
  delay(3000);
  #ifdef SENSOR_DEBUG
    while (!Serial);
  #endif

  Serial.print("Start Sensor ");
  Serial.println(sensorId);

  connectSSID();

  delay(1000);

  // Set MAC Address
  myMacAddress = getMacAddress();  

  // Set sendTopic
  sendTopic = "/arduino/";
  sendTopic +=  myMacAddress + "_ID" + String(sensorId);

  // pront mac, topic, 
  Serial.print("MAC: ");
  Serial.println(myMacAddress.c_str());
  Serial.print("Topic: ");
  Serial.println(sendTopic);
  
  Serial.println("Set broker and net.");
  client.begin(BROKER_IP, net);
  Serial.println("Set on message received.");
  client.onMessage(messageReceived);

  connect();  

  //starts all sensors
  sensor.start( A0, A2 );
  Serial.println("Sensors started.");

}

void loop(){
  // call loop() regularly to allow the library to send MQTT keep alives which
  // avoids being disconnected by the broker; delay(20) in hope to fix disconnection issues https://github.com/256dpi/arduino-mqtt
  /// Serial.println("INSIDE LOOP.");


  if ( client.connected() ) { 
    /// Serial.println("Connected and loop.");
    client.loop(); 
   } else
  { 
    /// Serial.println("Not connected. Connecting...");
    connect();
  }
  /// Serial.println("Outside If.");

  delay(20);
  
  sensor.addNoiseMeasurement();

  // avoid having delays in loop, we'll use the strategy from BlinkWithoutDelay
  if (millis() - lastMillis  >= logInterval ) {
    lastMillis = millis();

    String dataString = "";
    dataString = "{\"mac\":\""   + myMacAddress + "\",\"ID\":" + String(sensorId); 
    dataString += ",\"lbl\":\""  + sensorLabel + "\"";    
    dataString += ",\"RAM\":"    + String(freeMemory());
    dataString += ",\"RSSI\":"   + String(WiFi.RSSI());
    dataString += ",\"nRc\":"    + String(nReconnections);
    dataString += ",\"nWifiL\":" + String(nWifiLost);
    sensor.addMeasurement( dataString ); 
    dataString += "}";

    Serial.print("topic: ");
    Serial.println(sendTopic);
    Serial.print("dataString: ");
    Serial.println(dataString);

    // see https://github.com/256dpi/arduino-mqtt
    // publish(const char topic[], const char payload[], bool retained, int qos);
    // client.publish(sendTopic, dataString);
    client.publish(sendTopic, dataString, false, 1);
  }
}

void connectSSID(){
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network");
  printCurrentNet();
  printWifiData();
}

void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}
