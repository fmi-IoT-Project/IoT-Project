#include<ESP8266WiFi.h>
#include<PubSubClient.h>
#include<SPI.h>
#include<MFRC522.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#define RST_PIN         D3          // Configurable, see typical pin layout above
#define SS_PIN          D0         // Configurable, see typical pin layout above


const char* ssid = "Gratsiela";
const char* password = "0899936818";
const char* mqttServer = "m10.cloudmqtt.com";
const int mqttPort = 11083;
const char* mqttUser = "espwmcbt";
const char* mqttPassword = "W3lhbe-Y94Bb";
int pirPin = D4;

//LiquidCrystal_I2C lcd(0x3F,16,2);  // Create LCD instance
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
WiFiClient espClient;              // Create WiFi instance
PubSubClient client(espClient);    

// Initialize WiFi Network
void setWiFi(){
  
  WiFi.begin(ssid, password);
 
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println("\nConnected to the WiFi network");
}

// Initialize MQTT server
void setMQTT(){
  
  client.setServer(mqttServer, mqttPort);
  connectToMQTT();
  
}
void connectToMQTT(){
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) 
    {
      Serial.println("connected");  
      client.subscribe("security/motion");
    } 
    else 
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}
// Initialize RDIF sensor
void setRFID(){

  SPI.begin();
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

// Initialize motion sensor
void setPIR(){
   pinMode(pirPin,INPUT);
}

// Initialize LED monitor
void setLCD(){

}

void setup() {
 
  Serial.begin(115200);

  setWiFi();
  setMQTT();
  setRFID();
  setPIR();
  //setLCD();
}

void loop() {
/*Serial.println(digitalRead(pirPin));

if(digitalRead(pirPin) != 0){
  
    Serial.println("Motion");
    if(!client.connected())
      connectToMQTT();
    client.loop();
    
    client.publish("security/motion", "Motion");
    delay(1000);
  
}*/
  //Serial.println("New Lap");

  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }

  // Dump debug info about the card; PICC_HaltA() is automatically called
  Serial.println("New card detected\nDumping data...\n\n\n");
  mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  //Serial.println("New Lap");
 delay(1000);
}



