
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

#include <WebSocketsClient.h>
WebSocketsClient webSocket;

#define measurePin A0 //Connect dust sensor to Arduino A0 pin
#define ledPower D1   //Connect 3 led driver pins of dust sensor to Arduino D2
#define badAirPin D3
#define smokePin D4

int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 5680;

float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

bool socketConnected = false;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      socketConnected = false;
      break;
    case WStype_CONNECTED: {
        Serial.printf("[WSc] Connected to url: %s\n", payload);
        socketConnected = true;
        // send message to server when Connected
        //webSocket.sendTXT("Connected");
      }
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      //char* json = (char*) payload;
      //JsonObject& data = jsonBuffer.parseObject(json);


      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      hexdump(payload, length);

      // send data to server
      // webSocket.sendBIN(payload, length);
      break;
    case WStype_PING:
      // pong will be send automatically
      Serial.printf("[WSc] get ping\n");
      break;
    case WStype_PONG:
      // answer to a ping we send
      Serial.printf("[WSc] get pong\n");
      break;
  }

}

void setup() {
  pinMode(ledPower, OUTPUT);
  pinMode(smokePin, INPUT_PULLUP);
  pinMode(badAirPin, INPUT_PULLUP);

  Serial.begin(115200);
  WiFi.begin("DemoAP", "demoap123456789");    //WiFi connection

  while (WiFi.status() != WL_CONNECTED) {  //Wait for the WiFI connection
    delay(500);
    Serial.println("Waiting for connection");
  }

  webSocket.begin("192.168.88.253", 8080, "/airupdate");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(2000);
}

int avg = 0;

bool badAir = false;
bool smoke = false;

long long lastUpdate = 0;
int times = 0;

void loop() {
  avg = 0;
  smoke = false;

    webSocket.loop();
    
  if (millis() - lastUpdate > 1200) {
    digitalWrite(ledPower, LOW); // power on the LED
    delayMicroseconds(samplingTime);

    voMeasured = analogRead(measurePin); // read the dust value

    delayMicroseconds(deltaTime);
    digitalWrite(ledPower, HIGH); // turn the LED off
    delayMicroseconds(sleepTime);

    // 0 - 5V mapped to 0 - 1023 integer values
    // recover voltage
    calcVoltage = voMeasured * (5.0 / 1024.0);

    // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
    // Chris Nafis (c) 2012
    dustDensity = 0.17 * calcVoltage - 0.05;

    Serial.print("Raw Signal Value (0-1023): ");
    Serial.print(voMeasured);

    Serial.print(" - Voltage: ");
    Serial.print(calcVoltage);

    Serial.print(" - Dust Density: ");
    Serial.print(dustDensity * 1000);
    Serial.println(" ug/m3 ");
    if (dustDensity < 0) times--;
    avg += dustDensity * 1000;

    times++;
    lastUpdate = millis();
    if (times >= 6) {
      times = 0;
      avg /= 6;
      Serial.print("Final:");
      Serial.println(avg);
      if (digitalRead(badAirPin) == LOW) avg = 3000;
      if (digitalRead(smokePin) == LOW) smoke = true;

      String smokestr = smoke ? "true" : "false";
      String json = "{\"PM25\":" + String((int)avg) + ", \"Smoke\":" + smokestr + "}";
      Serial.println(json);
      webSocket.sendTXT(json);
    }
  }


  /*
    HTTPClient http; //Declare object of class HTTPClient
    http.begin("http://192.168.88.253:8080/airupdate"); //Specify request destination
    http.addHeader("Content-Type", "application/json"); //Specify content-type header

      int httpCode = http.POST(json); //Send the request
      String payload = http.getString(); //Get the response payload
      Serial.println(httpCode); //Print HTTP return code
      Serial.println(payload); //Print request response payload
      http.end(); //Close connection*/
}
