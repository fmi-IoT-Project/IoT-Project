#include<ESP8266WiFi.h>
#include<PubSubClient.h>
#include<SPI.h>
#include<MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define RST_PIN         D3          // Configurable, see typical pin layout above
#define SS_PIN          D0         // Configurable, see typical pin layout above

const char* ssid = "";
const char* password = "";
const char* mqttServer = "m10.cloudmqtt.com";
const int mqttPort = 11083;
const char* mqttUser = "espwmcbt";
const char* mqttPassword = "W3lhbe-Y94Bb";
int pirPin = D8;
bool motionFlag = false;
bool LOCKED = true;

String adminCard = "640244167";

const int maxCards = 2;
String memberCards[maxCards];
int numberCards = 0;


LiquidCrystal_I2C lcd(0x3F, 16, 2); // Create LCD instance
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
WiFiClient espClient;              // Create WiFi instance
PubSubClient client(espClient);

// Initialize WiFi Network
void setWiFi() {
  WiFi.begin(ssid, password);

  connectToWiFi();
}
void connectToWiFi() {
  Serial.print("Connecting to WiFi");

  printMessage("Connecting to", true, 0);
  printMessage("the WiFi...", false, 1);
  delay(1000);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nConnected to the WiFi network");
  printMessage("Connected to", true, 0);
  printMessage("the WiFi!", false, 1);
  delay(2000);
}

// Initialize MQTT server
void setMQTT() {
  client.setServer(mqttServer, mqttPort);
  connectToMQTT();
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
      Serial.println("connected");
      client.subscribe("security/motion");
      client.subscribe("security/login");
      client.subscribe("security/logout");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

// Initialize RDIF sensor
void setRFID() {
  SPI.begin();
  mfrc522.PCD_Init();
}

// Initialize motion sensor
void setPIR() {
  pinMode(pirPin, INPUT);
}

// Initialize LED monitor
void setLCD() {
  Wire.begin(D1, D2);
  lcd.backlight();
  lcd.begin(16, 2);
}

void setup() {
  Serial.begin(115200);

  setRFID();
  setPIR();
  setLCD();
  setWiFi();
  setMQTT();

}

String readCardNumber() {

  printMessage("Scan your card!", true, 0);
  Serial.println("Scan your card!");

  delay(1000);

  unsigned seconds = 16;
  int count = 1;

  while (seconds) {

    lcd.setCursor(count - 1, 1);
    lcd.print("-");
    count++;

    Serial.println(seconds);

    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent()) {
      delay(1000);
      --seconds;
      continue;
    }

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()) {
      delay(1000);
      --seconds;
      continue;
    }

    String ID = getCardID(mfrc522.uid.uidByte, mfrc522.uid.size);

    if (ID == adminCard) {
      givePermission();
    }

    Serial.print("New card detected\nID: ");
    Serial.println(ID);

    return ID;
  }
  return "";
}

String getCardID(byte *buffer, byte bufferSize) {

  String ID = "";
  for (byte i = 0; i < bufferSize; i++) {
    ID += buffer[i];
  }

  return ID;
}

bool existingCard(String cardNumber) {

  for (int i = 0; i < numberCards; ++i) {
    if (memberCards[i] == cardNumber) {
      return true;
    }
  }
  if(cardNumber == adminCard) {
    return true;
  }
  
  return false;
}

void printMessage(String mess, bool cleanScreen, int line) {

  if (cleanScreen)
    lcd.clear();

  int messSize = mess.length();
  if (messSize < 16) {
    int startCursorFrom = (16 - messSize) / 2;
    lcd.setCursor(startCursorFrom, line);
  }

  lcd.print(mess);
}

void detectedMotion() {

  if (motionFlag) {
    return;
  }

  motionFlag = true;
  Serial.println("Motion detected!");

  String cardNumber = readCardNumber();
  bool exist = existingCard(cardNumber);

  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  if (cardNumber == "") {
    printMessage("ALARM!", true, 0);
    Serial.println("ALARM!");

    client.publish("security/motion", "Motion");
    delay(3000);
    return;
  }

  if (!exist) {
    client.publish("security/motion", "Motion");
    printMessage("Invalid card!", true, 0);
    printMessage("ALARM!", false, 1);

    Serial.println("Invalid card!ALARM!");

  } else {
    String mess = cardNumber;

    client.publish("security/login", mess.c_str());
    printMessage("Hello", true, 0);
    printMessage(mess, false, 1);
    Serial.println( "Hello " + mess);
    LOCKED = false;
  }
  delay(2000);
}

void checkOut() {
  printMessage("To lock", true, 0);
  printMessage("scan your card", false, 1);

  delay(1000);
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  String cardNumber = getCardID(mfrc522.uid.uidByte, mfrc522.uid.size);
  if (cardNumber == adminCard) {
    givePermission();
  }

  bool exist = existingCard(cardNumber);

  Serial.print("New card detected\nID: ");
  Serial.println(cardNumber);

  if (!exist) {
    printMessage("You have no", true, 0);
    printMessage("permission!", false, 1);

    Serial.println("You have no permission!");

  } else {
    
    if (WiFi.status() != WL_CONNECTED) {
      connectToWiFi();
    }
    if (!client.connected()) {
      connectToMQTT();
    }
    client.loop();

    client.publish("security/logout", cardNumber.c_str());
    printMessage("Bye", true, 0);
    printMessage(cardNumber, false, 1);
    Serial.println( "Bye " + cardNumber);
    LOCKED = true;
  }
  delay(2000);
}
void givePermission() {

  printMessage("Manage card!", true, 0);
  Serial.println("Manage card!");

  delay(1000);

  unsigned seconds = 5;
  int count = 1;

  while (seconds) {

    lcd.setCursor(count - 1, 1);
    lcd.print("/");
    count++;

    Serial.println(seconds);

    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent()) {
      delay(1000);
      --seconds;
      continue;
    }

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()) {
      delay(1000);
      --seconds;
      continue;
    }

    String ID = getCardID(mfrc522.uid.uidByte, mfrc522.uid.size);

    if (existingCard(ID)) {
      for(int i = 0;i < numberCards;++i)
      {
        if(ID == memberCards[i])
        {
          memberCards[i] = memberCards[numberCards-1];
          numberCards--;         
          printMessage("Card removed", true, 0);       
          printMessage(ID, false, 1);
        }
      }
      break;
    }
    else {

      if (numberCards < maxCards) {

        memberCards[numberCards] = ID;
        numberCards += 1;
        
        printMessage("New card:", true, 0);
        printMessage(ID, false, 1);

        Serial.print("New card detected\nID: ");
        Serial.println(ID);

      } else {
        printMessage("Memory full", true, 0);
      }
      break;
    }
  }
  delay(3000);
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  // Nobody is at home
  if (LOCKED) {
    int valueRID = digitalRead(pirPin);

    if (valueRID == 0) {
      motionFlag = false;
      printMessage("The system is", true, 0);
      printMessage("locked!", false, 1);
      delay(1000);
    } else {
      detectedMotion();
    }
    //Someone(admin) is at home
  } else {
    checkOut();
  }
}
