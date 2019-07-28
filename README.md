# IoT-Project Smart box

Parts:
 - Wireless module ESP8266 - send data into the Cloud MQTT
 - RFID sensor - loggin/logout into the system with TAC card and disable/enable PIR sensor
 - Motion control(PIR) sensor - detect motion if it's enable
 - LCD display - display motion detection, user is loggin/logout

Purpose:
- Detects motion in 2 meters. Unless user is authentificated with RFID TAC send warning message to Cloud MQTT. 
  Than send notification to admin phone MQTT application. Message are been visualized on LCD display.
