#include "WizFi360.h"

#include <Arduino.h>
#include <stdlib.h>
#include "pio.h"
void  sensorRead();
void  thingspeakTrans();
void printWifiStatus();

/* Baudrate */
#define SERIAL_BAUDRATE   115200

UART Serial2(4, 5); //It has to be GPIO pins not board pins so GP4 AND GP5

static char ssid[] = "0123456789";       // your network SSID (name)
static char pass[] = "";   // your network password
volatile int status = WL_IDLE_STATUS;  // the Wifi radio's status

char server[] = "api.thingspeak.com"; // server address
String apiKey ="1KOX97Q6W6RF54HZ";                    // apki key

// sensor buffer
char temp_buf[10] = {1,2,3,4,5,6,7,8,9,10};
char humi_buf[10] = {1,2,3,4,5,6,7,8,9,10};
char cds_buf[10] = {1,2,3,4,5,6,7,8,9,10};

unsigned long lastConnectionTime = 0;         // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 30000L; // delay between updates, in milliseconds

// Initialize the Ethernet client object
WiFiClient client;

void setup() {              
  
  // initialize serial for debugging
  Serial.begin(SERIAL_BAUDRATE);
  // initialize serial for WizFi360 module
  Serial2.begin(SERIAL_BAUDRATE);

  WiFi.init(&Serial2);
  
  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
    if(status == WL_CONNECT_FAILED){
      Serial.println("You couldnot connect to the network");
    }
  }
  Serial.println("You're connected to the network");
  
  printWifiStatus();

  //First transmitting
  thingspeakTrans();
}

//Transmitting sensor value to thingspeak
void thingspeakTrans()
{
  // close any connection before send a new request
  // this will free the socket on the WiFi shield
  client.stop();

  Serial.println("stop the client");

  // if there's a successful connection
  if (client.connect(server, 80)) {
    Serial.println("Connecting...");
    
    // send the Get request
    client.print(F("GET /update?api_key="));
    client.print(apiKey);
    client.print(F("&field1="));
    client.print(temp_buf);
    client.print(F("&field2="));
    client.print(humi_buf);
    client.print(F("&field3="));
    client.print(cds_buf);
    client.println();
    // note the time that the connection was made
    lastConnectionTime = millis();
  }
  else {
    // if you couldn't make a connection
    Serial.println("Connection failed");
  }
}

void printWifiStatus()
{
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}



void loop()
{
  // if there's incoming data from the net connection send it out the serial port
  // this is for debugging purposes only   
  while (client.available()) {
    char c = client.read();
    Serial.print("recv data: ");
    Serial.write(c);
    Serial.println();
  }
  
  // if 30 seconds have passed since your last connection,
  // then connect again and send data
  if (millis() - lastConnectionTime > postingInterval) {
    thingspeakTrans();
  }
}

