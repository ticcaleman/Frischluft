
// https://github.com/256dpi/arduino-mqtt/blob/master/examples/ArduinoWiFi101/ArduinoWiFi101.ino
#include <MQTT.h>

WiFiClient net;
String myMacAddress;

MQTTClient client(256);                 // payload increased size from 128 to 256

long nReconnections = 0;
unsigned long nWifiLost = 0;


long        logInterval =  180000; // 300000: 5 minutes; 180000: 3 minutes
//long        logInterval =  60000; // 300000: 5 minutes; 180000: 3 minutes
long        lastMillis  = -logInterval;     // to start logging from beginning
// long int goes from  -2147483648 to 2147483647



String getMacAddress() {
  byte mac[6];
  byte mac_reversed[6];
  WiFi.macAddress(mac);
  
  for(int i = 0; i<6; i++){
    mac_reversed[i] = mac[5-i];
  }
  
  String cMac = "";
  for (int i = 0; i < 6; ++i) {
    if (mac_reversed[i]<0x10) {cMac += "0";}
    cMac += String(mac_reversed[i],HEX);
    if(i<5)
    cMac += ""; // put : or - if you want byte delimiters
  }
  cMac.toUpperCase();
  return cMac;
}

void myprintMacAddress()
{
  byte mac[6];                     // the MAC address of your Wifi shield

  if ( WiFi.status() != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while(true);
  }
  // if you are connected, print your MAC address:
  else {
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);
  }
}

void setClientSubscriptions(){

  
    client.setWill(sendTopic.c_str(), "Error disconnected sensor ...\"", true, 2);
  // void setWill(const char topic[]);
  // void setWill(const char topic[], const char payload[]);
  // void setWill(const char topic[], const char payload[], bool retained, int qos);

  client.subscribe("/hello"); //SUBSCRIBE TO TOPIC /hello
  client.subscribe("/temp"); //SUBSCRIBE TO TOPIC /t
  client.subscribe("/humidity"); //SUBSCRIBE TO TOPIC /h
  client.subscribe("/arduino/setLogIntrvMin");
  client.subscribe("/arduino/setLogIntrvMili");
  client.subscribe("/arduino/getLogIntrvMili");
  client.subscribe("/arduino/getSensorLabel/myPersonalMac");
  client.subscribe("/arduino/setSensorLabel/#");  

}

void setKeepAlive(){
  //client.setOptions(int keepAlive, bool cleanSession, int timeout);
  // The keepAlive option controls the keep alive logInterval in seconds (default: 10).
  // The cleanSession option controls the session retention on the broker side (default: true).
  // The timeout option controls the default timeout for all commands in milliseconds (default: 1000).  
  // client.setOptions(180, true, 1300);  
  // change with logInterval 
  int keepAlive;
  keepAlive = int(1.02 * (logInterval / 1000.));
  client.setOptions(keepAlive, true, 3000); 
  Serial.print("keepAlive time [s]: ");
  Serial.println(keepAlive);  
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  if (topic == "/arduino/setLogIntrvMin"){
    logInterval  =  payload.toInt() * 1000 * 60;
  }

  if (topic == "/arduino/setLogIntrvMili"){
    logInterval  =  payload.toInt();     
  }

  if (topic == "/getLogIntrvMili"){
    String dataString = "";
    dataString = "{\"mac\":\"" + myMacAddress + "\",\"ID\":" + String(sensorId); 
    dataString += ",\"measurementFrec\":\"" + String(logInterval) + "}";
    client.publish(topic, dataString);
  }


  if (topic == "/setSensorLabel//myPersonalMac"){
    // check with IPAddress
    // sensorLabel = payload;   
  }
}
