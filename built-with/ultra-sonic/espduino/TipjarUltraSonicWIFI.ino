/*
 * This code is for tipjar to sense the coin drop and send the event to thingspeak.com.
 * A web interface will let the user to configure wireless connection to ESPDUINO board.
 * Once the wireless connection is made, WIFI credentials are stored in EEPROM and retrieved. 
 * By Pugal Shanmugam for WestboroughIOT.
 */


#define echoPin 5 // Echo Pin
#define trigPin 1 // Trigger Pin
#define LEDPin 13 // Onboard LED

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

String apiKey = "IXGFE0UCK751R3V0";

const char* ssid = "IAMTIPJAR";
const char* password = "PASSWORD";

byte a[20];
byte b[20];

int ssidlen = 0;
int sspasslen = 0;
 
int maximumRange = 200; // Maximum range needed
int minimumRange = 0; // Minimum range needed
long duration, distance; // Duration used to calculate distance
int dis_without_tip; // sets the distance without a tip
int distance_w_adjust; // add some type of correction for senstivity factors.

//const char* host = "things.westboroughiot.com";
const char* host = "api.thingspeak.com";
String url = "/0/";
ESP8266WebServer server(80);
WiFiClient client;
boolean wificonnected = false;
int J = 0;

int addr = 0;

void handleRoot(){
  Serial.println("Enter handleRoot");
  String header;
  String content = "<html><body><H2>Welcome to TipJar Configuration</H2><br>";
  if (server.hasHeader("User-Agent")){
    content += "the user agent used is : " + server.header("User-Agent") + "<br><br>";
  }
  content += "<a href='/login'>Click here to  configure WiFi for TipJar</a></body></html>";
  server.send(200, "text/html", content);
}

void initialScreen(){
  Serial.println("Enter initialscreen");
  String header;
  String content = "<html><body><H2>Welcome to TipJar Configuration</H2><br><form action='/wifi' method='POST'><br>";
  int n=0;
  for (int i = 0; i < 3; ++i)
  {
  n = WiFi.scanNetworks();
  delay(5000);
  }
  Serial.println("scan done");
  if (n == 0)
    content += "<H1>No WiFi Networks Available</H1>";
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      
      content += "<input type=\"radio\" name=\"SSIDVal\" value=\""+WiFi.SSID(i)+"\">"+WiFi.SSID(i)+"</input></p>";
      
      delay(10);
    }
  }
   content += "Password:<input type='password' name='PASSWORD' placeholder='password'><br>";
   content += "<input type='submit' name='SUBMIT' value='Submit'></form><br>";
   content += "</body></html>";
   server.send(200, "text/html", content);
}
void WifiConnect(){

  Serial.println("WIFI Connect");
  String content = "";
  String strSSID = "";
  String strPass = "";

   if (server.hasArg("SSIDVal") && server.hasArg("PASSWORD")){
    
      strSSID = server.arg("SSIDVal");
      Serial.println("connecting toSSIDVal ******"+strSSID);
      strPass = server.arg("PASSWORD");
      Serial.println("connecting Pass ******"+strPass);
      char chSSID[strSSID.length()];
      char chPass[strPass.length()]; 
      strSSID.toCharArray(chSSID,strSSID.length()+1);
      strPass.toCharArray(chPass,strPass.length()+1);
      String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=1\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
      server.sendContent(header);
      WiFi.disconnect(true);
      Serial.println();
      Serial.print("connecting to ");
      Serial.println(strSSID);
      WiFi.begin(chSSID,chPass);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      String strIP= "";
      Serial.println(WiFi.localIP());
      content = "<html><body><H1>You have successfully connected to "+server.arg("SSIDVal")+"</H1><br>";
      wificonnected = true;
      EEPROM.begin(512);
       for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
       for (int i = 0; i < strSSID.length(); ++i)
          {
            EEPROM.write(i, strSSID[i]);
            Serial.print("Wrote: ");
            Serial.println(strSSID[i]); 
          }
        Serial.println("writing eeprom pass:"); 
        for (int i = 0; i < strPass.length(); ++i)
          {
            EEPROM.write(32+i, strPass[i]);
            Serial.print("Wrote: ");
            Serial.println(strPass[i]); 
          }    
        EEPROM.commit();
       
      
    //  content = "<body><H1>IP Address "+strIP(WiFi.localIP())+"</H1><br></html>"
  } else {
  content = "<html><body><H1>Unable to make a WiFi connection </H1><br>";
  }
  server.send(200, "text/html", content);
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
void setup(void) 
{
    
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(BUILTIN_LED, OUTPUT); // Use LED indicator (if required)
    WiFi.disconnect();
    WiFi.mode(WIFI_AP_STA);
    Serial.begin(115200);
    EEPROM.begin(512);
    Serial.println("Starting serial");
    String esid;
      for (int i = 0; i < 32; ++i)
        {
          esid += char(EEPROM.read(i));
        }
      Serial.print("SSID: ");
      Serial.println(esid);
      Serial.println("Reading EEPROM pass");
      String epass = "";
      for (int i = 32; i < 96; ++i)
        {
          epass += char(EEPROM.read(i));
        }
      Serial.print("PASS: ");
      Serial.println(epass);  
      if ( esid.length() > 1 ) {
          // test esid 
          WiFi.begin(esid.c_str(), epass.c_str());
          int V=0;
          while (WiFi.status() != WL_CONNECTED && V < 20) {
              delay(500);
              Serial.print(".");
              V++;
            }
            if (WiFi.status() != WL_CONNECTED)
            {
              Serial.print("wifi not connected so starting accessing point");
              WiFi.softAP(ssid, password);
              
            } else
            {
                Serial.print("Wifi Connected");
                wificonnected = true;
              
            }
        }   
    else {
    
    WiFi.softAP(ssid, password);
    Serial.print("Access point started");

    
    }
    EEPROM.end();
    Serial.println("");
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    
    server.on("/", handleRoot);
    
    server.on("/login", initialScreen);
    server.on("/wifi", WifiConnect);
    server.onNotFound(handleNotFound);
    //here the list of headers to be recorded
    const char * headerkeys[] = {"User-Agent","Cookie"} ;
    size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
    //ask server to track these headers
    server.collectHeaders(headerkeys, headerkeyssize);
    server.begin();
    Serial.println("HTTP server started");
}

void loop() 
{
  if (wificonnected)
{
     Serial.println("hello");
     String post_payload = "'{\"Location\":\"Westborough\",\"Business\":\"Central House\",\"ID\":\"JAR0001\"}'";
     digitalWrite(trigPin, LOW); 
     delayMicroseconds(2); 
     digitalWrite(trigPin, HIGH);
     delayMicroseconds(10); 
     digitalWrite(trigPin, LOW);
     duration = pulseIn(echoPin, HIGH);
     distance = duration/58.2;
     if (!dis_without_tip) {
         Serial.println("Setting non tip distance to ");
          Serial.println(distance);
        dis_without_tip = distance;
      }
     Serial.println(distance);
     distance_w_adjust = distance  + 1; // give some room for error of 1 
     if ( distance_w_adjust < dis_without_tip )
     {
        J = J + 1;
        Serial.println("tip");
        digitalWrite(BUILTIN_LED, HIGH);
      }   else 
      {
        
              digitalWrite(BUILTIN_LED, LOW); 
              Serial.println("no tip");
              if(J > 0)
              {     
                Serial.println(host);
                if (!client.connect(host, 80)) {
                  Serial.println("connection failed");
                  return;
               }
                Serial.println("Inside for loop");
                String postStr = apiKey;
                 postStr +="&field1=1";
                 postStr +="&field2=";
                 postStr +=String(J);
                 postStr +="&field3=0"; 
                 postStr += "\r\n\r\n";
                
                 client.print("POST /update HTTP/1.1\n");
                 client.print("Host: api.thingspeak.com\n");
                 client.print("Connection: close\n");
                 client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
                 client.print("Content-Type: application/x-www-form-urlencoded\n");
                 client.print("Content-Length: ");
                 client.print(postStr.length());
                 client.print("\n\n");
                 client.print(postStr);
                 delay(5000);
                 while(client.available()) {
                Serial.println("client is available");
                String line = client.readStringUntil('\r');
                Serial.print(line);
                 }
                client.stop();
                J = 0;
              }
        
       }
     
     //Delay 50ms before next reading.
     delay(50);
} 
 server.handleClient();


}
