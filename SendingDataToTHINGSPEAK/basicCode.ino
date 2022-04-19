#include <SoftwareSerial.h>
SoftwareSerial espSerial(2,3);


String apiKey ="********************";

float temp;
#define led_pin 11

//String ssid="**********";
//String password="********************";

boolean DEBUG=true;

void showResponse(int waitTime)
{
  long t= millis();
  char c;
  while(t+waitTime>millis())
  {
    if(espSerial.available())
    {
      c=espSerial.read();
      if(DEBUG)
        {
          Serial.print(c);
        }
    }
  }
}




boolean thingSpeakWrite(float value1)
{
  String cmd= "AT+CIPSTART=\"TCP\",\"";
  cmd += "184.106.153.149"; //api.thingspeak.com
  cmd += "\",80";
  espSerial.println(cmd);
  if(DEBUG)
  {
    Serial.println(cmd);
  }
  if(espSerial.find("ERROR"))
  {
   Serial.println("AT+CIPSTART error");
   return false;
  }

  String getStr="GET /update?api_key=";
  getStr += apiKey;
  getStr +="&field1=";
  getStr += value1;
  getStr += "\r\n\r\n";

//  send data length
  cmd ="AT+CIPSEND=";
  cmd+= String(getStr.length());
  espSerial.println(cmd);
  Serial.println(cmd);
  delay(100);
  if(espSerial.find(">"))
  {
    espSerial.println(getStr);
    Serial.println(getStr);
    
    
  }
  else{
    espSerial.println(getStr);
    Serial.println(getStr);
    espSerial.println("AT+CIPCLOSE");
    Serial.println("AT+CIPCLOSE command has been executed");
    return false;
  }
  return true;
}


void setup() {
  // put your setup code here, to run once:
DEBUG=true;
Serial.begin(9600);

espSerial.begin(115200);


esp8266Serial("AT+RST\r\n", 5000, DEBUG); // Reset the ESP8266
//esp8266Serial("AT+UART_CUR=9600\r\n", 5000, DEBUG);//Enable this line to set Serial Port to 9600bs Speed
 esp8266Serial("AT+CWMODE=1\r\n", 5000, DEBUG); //Set station mode Operation
  esp8266Serial("AT+CWJAP=\"EW2G\",\"emi19ce258\"\r\n", 5000, DEBUG);//Enter your WiFi network's SSID and Password.
  esp8266Serial("AT+CIFSR\r\n", 5000, DEBUG);  
    esp8266Serial("AT+CIPMUX=1\r\n", 5000, DEBUG);
    esp8266Serial("AT+CIPSERVER=1,80\r\n", 5000, DEBUG);

showResponse(1000);
showResponse(5000);

if(DEBUG)
{
  Serial.println("Setup completed");
}

}

void loop() {
  // put your main code here, to run repeatedly:
  temp=analogRead(A0);
  temp=temp*0.48828125;
  if(DEBUG){
  Serial.println(temp);
  }

  delay(1000);
  thingSpeakWrite(temp);
}
String esp8266Serial(String command, const int timeout, boolean debug)
  {
    String response = "";
    espSerial.print(command);
    long int time = millis();
    while ( (time + timeout) > millis())
      {
        while (espSerial.available())
          {
            char c = espSerial.read();
            response += c;
          }
      }
    if (debug)
      {
        Serial.print(response);
      }
    return response;
}
