#include <PubSubClient.h>
#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include "SoftwareSerial.h"
#define DEBUG true
#define LED 11 
float temp;

#define WIFI_AP "*******" //Wifi Name
#define WIFI_PASSWORD "*******" //Wifi Password
const char* mqtt_server = "*******"; //Server Ip address, MQTT Server. Usually in Raspberry Pi I have made a server
const char* mqttUser = "*******";      // if you don't have MQTT Username, no need input
const char* mqttPassword = "*******";  // if you don't have MQTT Password, no need input


SoftwareSerial soft(2, 3); // RX, TX
int status = WL_IDLE_STATUS;

WiFiEspClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[50];
int value = 0;


long anotherMsg =0;
char anothermsg[50];
int PIRon=1;
int wifiReset=0;

long offTimeMsg=0;
char offmsg[50];
int PIRoff=0;

//PIR SENSOR
#define pirPin 9
int calibrationTime = 30;
long unsigned int lowIn;
long unsigned int pause = 500;
boolean lockLow = true;
boolean takeLowTime;
int PIRValue = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void(*resetFunc)(void)=0;

void ResetWifi()
{
  esp8266Serial("AT+RST\r\n", 5000, DEBUG); // Reset the ESP8266
  delay(10000);
  wifiReset=0;
   resetFunc();   
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),mqttUser,mqttPassword)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
      wifiReset=0;
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      wifiReset++;
      if(wifiReset == 10)
      {
        ResetWifi();
      }
    }
  }
}

void setup()
{
  pinMode(LED, OUTPUT);
   digitalWrite(LED, LOW);
   Serial.begin(9600);   
    FirstWiFi();
    client.setServer(mqtt_server, 1883);
}

void loop()
{
   if (!client.connected()) {
    reconnect();
  }
  client.loop();

  PIRSensor();
  temp=analogRead(A0);
  temp = temp * 0.48828125;
  String check =(String)temp;
  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    snprintf (msg, 50, "%s", check.c_str());
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic", msg);
    client.setCallback(callback);
  }
  

}
void FirstWiFi()
{
  // initialize serial for ESP module
  soft.begin(9600);
  // initialize ESP module
  WiFi.init(&soft); 
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(WIFI_AP);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
    Serial.println(WiFi.localIP());
    delay(500);
  }
  Serial.println("Connected to AP");
}
void PIRSensor() {
   if(digitalRead(pirPin) == HIGH) {
      if(lockLow) {
         PIRValue = 1;
         lockLow = false;
         Serial.println("Motion detected.");
        digitalWrite(LED,HIGH);
        long anow = millis();
        String stringPIRon =(String)PIRon;
        if(anow - anotherMsg > 2000)
        {
          anotherMsg =anow;
          snprintf(anothermsg,"%s",stringPIRon.c_str());
          Serial.println("Publish PIR ON Message:");
          Serial.println(anothermsg);
          client.publish("PirTopic",anothermsg); 
        }
      }
      takeLowTime = true;
   }
   if(digitalRead(pirPin) == LOW) {
      if(takeLowTime){
         lowIn = millis();takeLowTime = false;
      }
      if(!lockLow && millis() - lowIn > pause) {
         PIRValue = 0;
         lockLow = true;
         Serial.println("Motion ended.");
         digitalWrite(LED,LOW);
         long aanow = millis();
         String stringPIRoff = (String)PIRoff;
         if(aanow - offTimeMsg > 2000)
         {
            offTimeMsg = aanow;
            snprintf(offmsg,"%s",stringPIRoff.c_str());
            Serial.println("Publish PIR OFF Message:");
            Serial.println(offmsg);
            client.publish("PirTopic",offmsg);
         }
      }
   }
}

String esp8266Serial(String command, const int timeout, boolean debug)
  {
    String response = "";
    soft.print(command);
    long int time = millis();
    while ( (time + timeout) > millis())
      {
        while (soft.available())
          {
            char c = soft.read();
            response += c;
          }
      }
    if (debug)
      {
        Serial.print(response);
      }
    return response;
}
