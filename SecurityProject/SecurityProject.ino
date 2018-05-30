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

String adminCards[][2] = { {"640244167", "Gratsi"}, {"68210143185", "Bojo"} };
int numberCards = 2;

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

bool existingCard(String cardNumber, String& nameUser) {

  for (int i = 0; i < numberCards; ++i) {
    if (adminCards[i][0] == cardNumber) {
      nameUser = adminCards[i][1];
      return true;
    }
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
  String nameUser = "";
  bool exist = existingCard(cardNumber, nameUser);

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
    String mess = cardNumber + " " + nameUser;

    client.publish("security/login", mess.c_str());
    printMessage("Hello", true, 0);
    printMessage(nameUser, false, 1);
    Serial.println( "Hello " + nameUser);
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

  String nameUser = "";
  String cardNumber = getCardID(mfrc522.uid.uidByte, mfrc522.uid.size);
  bool exist = existingCard(cardNumber, nameUser);

  Serial.print("New card detected\nID: ");
  Serial.println(cardNumber);

  if (!exist) {
    printMessage("You have not", true, 0);
    printMessage("permission!", false, 1);

    Serial.println("You have not permission!");

  } else {
    String mess = cardNumber + " " + nameUser;

    if (WiFi.status() != WL_CONNECTED) {
      connectToWiFi();
    }
    if (!client.connected()) {
      connectToMQTT();
    }
    client.loop();

    client.publish("security/logout", mess.c_str());
    printMessage("Bye", true, 0);
    printMessage(nameUser, false, 1);
    Serial.println( "Bye " + nameUser);
    LOCKED = true;
  }
  delay(2000);
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



