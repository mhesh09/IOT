#include <Arduino.h>
#include <Stream.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

//AWS
#include "sha256.h"
#include "Utils.h"


//WEBSockets
#include <Hash.h>
#include <WebSocketsClient.h>

//MQTT PAHO
#include <SPI.h>
#include <IPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>



//AWS MQTT Websocket
#include "Client.h"
#include "AWSWebSocketClient.h"
#include "CircularByteBuffer.h"

extern "C" {
  #include "user_interface.h"
}


//AWS IOT config, change these:
char wifi_ssid[]       = "**********";
char wifi_password[]   = "********************";
char aws_endpoint[]    = "**********-ats.iot.us-west-2.amazonaws.com";
char aws_key[]         = "**********SLNK3SGQ";
char aws_secret[]      = "**********uftJ0zEz22SvR/qfVs1rh99";
char aws_region[]      = "us-west-2";
const char* aws_topic  = "$aws/things/IOT_Testing/shadow/update";
int port = 443;
float temp;
const int LED=13;
//MQTT config
const int maxMQTTpackageSize = 512;
const int maxMQTTMessageHandlers = 1;

ESP8266WiFiMulti WiFiMulti;

AWSWebSocketClient awsWSclient(1000);

IPStack ipstack(awsWSclient);
MQTT::Client<IPStack, Countdown, maxMQTTpackageSize, maxMQTTMessageHandlers> client(ipstack);

//# of connections
long connection = 0;

//generate random mqtt clientID
char* generateClientID () {
  char* cID = new char[23]();
  for (int i=0; i<22; i +=1)
    cID[i]=(char)random(1, 256);
  return cID;
}

//count messages arrived
int arrivedcount = 0;

//callback to handle mqtt messages
void messageArrived(MQTT::MessageData& md)
{
  MQTT::Message &message = md.message;

  Serial.print("Message ");
  Serial.print(++arrivedcount);
  Serial.print(" arrived: qos ");
  Serial.print(message.qos);
  Serial.print(", retained ");
  Serial.print(message.retained);
  Serial.print(", dup ");
  Serial.print(message.dup);
  Serial.print(", packetid ");
  Serial.println(message.id);
  Serial.print("Payload ");
  char* msg = new char[message.payloadlen+1]();
  memcpy (msg,message.payload,message.payloadlen);

  String command1,command2;
  for(int i=0;i<message.payloadlen;i++)
  {
    
    if(i==3||i==4||i==5||i==6||i==7||i==8)
    {
      command1 += msg[i];  
    }
    //Serial.println(msg[i]);

    if(i==12||i==13||i==14)
    {
      command2 +=msg[i];
    }
  
  }
  
  Serial.println(command1);
  Serial.println(command2);

  if(command2 == "on\"")
  {
    Serial.println("There is data");
      digitalWrite(LED, HIGH);
    temperature();
  
  }

  


  
  if(command2 =="off")
{
  Serial.println("Next data has come");
   digitalWrite(LED, LOW);
}
  
  delete msg;
}

void temperature()
  {
    temp=analogRead(A0);
  temp = temp * 0.48828125;
  Serial.println(temp);
  MQTT::Message message;
    char buf[100];
 //     int check=1233;

    String newtest= "{\"state\":{\"reported\":{\"on\": false}, \"desired\":{\"temp\":";
    newtest += temp;
    newtest += "}}}";

  int n = newtest.length();

  char test[n+1];
  strcpy(test,newtest.c_str()); 
   strcpy(buf,test);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
    int rc = client.publish(aws_topic, message);  
  }
//connects to websocket layer and mqtt layer
bool connect () {



    if (client.isConnected ()) {    
        client.disconnect ();
    }  
    //delay is not necessary... it just help us to get a "trustful" heap space value
    delay (1000);
    Serial.print (millis ());
    Serial.print (" - conn: ");
    Serial.print (++connection);
    Serial.print (" - (");
    Serial.print (ESP.getFreeHeap ());
    Serial.println (")");




   int rc = ipstack.connect(aws_endpoint, port);
    if (rc != 1)
    {
      Serial.println("error connection to the websocket server");
      return false;
    } else {
      Serial.println("websocket layer connected");
    }


    Serial.println("MQTT connecting");
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 4;
    char* clientID = generateClientID ();
    data.clientID.cstring = clientID;
    rc = client.connect(data);
    //delete[] clientID;
    if (rc != 0)
    {
      Serial.print("error connection to MQTT server");
      Serial.println(rc);
      return false;
    }
    Serial.println("MQTT connected");
    return true;
}

//subscribe to a mqtt topic
void subscribe () {
   //subscript to a topic
    int rc = client.subscribe(aws_topic, MQTT::QOS0, messageArrived);
    if (rc != 0) {
      Serial.print("rc from MQTT subscribe is ");
      Serial.println(rc);
      return;
    }
    Serial.println("MQTT subscribed");
}

//send a message to a mqtt topic
void sendmessage () {
    //send a message
    MQTT::Message message;
    char buf[100];
      int check=1233;

    String newtest= "{\"state\":{\"reported\":{\"on\": false}, \"desired\":{\"on\":";
    newtest += check;
    newtest += "}}}";

  int n = newtest.length();

  char test[n+1];
  strcpy(test,newtest.c_str()); 
   strcpy(buf,test);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
    int rc = client.publish(aws_topic, message); 
}


void setup() {
   pinMode(LED, OUTPUT);
    
    wifi_set_sleep_type(NONE_SLEEP_T);
   
    Serial.begin (115200);
    delay (2000);
    Serial.setDebugOutput(1);

    //fill with ssid and wifi password
    WiFiMulti.addAP(wifi_ssid, wifi_password);
    Serial.println ("connecting to wifi");
    while(WiFiMulti.run() != WL_CONNECTED) {  
        delay(100);
        Serial.print (".");
    }
    Serial.println ("\nconnected");

    //fill AWS parameters    
    awsWSclient.setAWSRegion(aws_region);  
    awsWSclient.setAWSDomain(aws_endpoint);
    awsWSclient.setAWSKeyID(aws_key);
    awsWSclient.setAWSSecretKey(aws_secret);
    awsWSclient.setUseSSL(true);

    if (connect ()){
      subscribe ();
      sendmessage ();
    }

}

void loop() {
  //keep the mqtt up and running
  if (awsWSclient.connected ()) {    
      client.yield(50);
  } else {
    //handle reconnection
    if (connect ()){
      subscribe ();      
    }
  }

}
