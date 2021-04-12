#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <DHT_U.h>
#include "MqttMessage.h"
#include "configFile.h"

//Variables
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor for normal 16mhz Arduino
int moistor;
float hum;  //Stores humidity value
float temp; //Stores temperature value
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
int value = 0;
MqttMessage messages[MSG_BUFFER_ARRAY_SIZE];
int mqttMessageIndex = 0;
bool pumpOn = false;    // pump is on
int pumpTime = 2000;          // the amount of time the pump is active at a time
int moistorTriggerValue = 500;  // the value that triggers the moistor
int nextMoistorCheck = 300000;  // the value that triggers the pump timer
unsigned long shutPumpOff = 0;  // when to shut pump off
unsigned long nextPumpCheck = 0;  // Next water moistorCheck
void callback(char* topic, byte* message, unsigned int length);




void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(500);
  Serial.println("Booting up");

  pinMode(MOISTORPIN, INPUT);
  pinMode(PUMPPIN, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // insert pin layout
  dht.begin();
  Serial.println("Booting Compleat");
}

void loop()
{
  client.loop();
  if (!client.connected()) {
    reconnect();
  }
  SendMQTT();


  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    
    lastMsg = now;
    // read DHT sensor values
    ReadSoilSensor();
    ReadDHTSensor();
    
    // set up mqtt messages
    SetupHumiditiMessage();
    SetupTempetureMessage();
    SetupMoistorMessage();
    SetupSoilMoistureMessage();

    // check the pump run it if neede
    CheckPumpStatus(now);
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  char cMessage[length];

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    cMessage[i] = (char)message[i];
  }
  Serial.println();

  if (strcmp(topic, "/room/bedrooms/1/soilMosistorTriggerSet/1") == 0) {
    int tempNum = atoi(cMessage);
    if (tempNum > 0 && tempNum < 1024)
    {
      moistorTriggerValue = tempNum;
      String temp = String(moistorTriggerValue);
      NewMqttMessage(temp, "/room/bedrooms/1/soilMosistorTriggerValue/1");
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe("/room/bedrooms/1/soilMosistorTriggerSet/1");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void SetupHumiditiMessage() {
  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.println("%");
  char cHum[16];
  sprintf(cHum, "%.1f", hum);

  NewMqttMessage(cHum, "/room/bedrooms/1/humidity/1");
}

void SetupTempetureMessage() {
  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.println(" Celsius");
  char cTemp[16];
  sprintf(cTemp, "%.1f", temp);

  NewMqttMessage(cTemp, "/room/bedrooms/1/tempeture/1");
}

void SetupMoistorMessage() {
  char cMoistor[16];
  itoa(moistor, cMoistor, 10);

  NewMqttMessage(cMoistor, "/room/bedrooms/1/moistor/1");
}

void SetupSoilMoistureMessage() {
  if (moistor >= 700) {
    Serial.println("Air");
    NewMqttMessage("Air", "/room/bedrooms/1/moistorStatus/1");
  } else if (moistor > 600) {
    Serial.println("Super Dry Dirt");
    NewMqttMessage("Super Dry Dirt", "/room/bedrooms/1/moistorStatus/1");
  } else if (moistor > 500) {
    Serial.println("Dry Dirt");
    NewMqttMessage("Dry Dirt", "/room/bedrooms/1/moistorStatus/1");
  } else if (moistor > 300) {
    Serial.println("Wet soil");
    NewMqttMessage("Wet soil", "/room/bedrooms/1/moistorStatus/1");
  } else {
    Serial.println("What am i looking at ??");
    NewMqttMessage("What am i looking at ??", "/room/bedrooms/1/moistorStatus/1");
  }
}

void TurnPumpOn(unsigned long timer) {
  pumpOn = true;
  shutPumpOff = timer + pumpTime;
  digitalWrite(PUMPPIN, HIGH);
  NewMqttMessage("Pump ON", "/room/bedrooms/1/pumpStatus/1");
  Serial.println("Pump ON");
}

void TurnPumpOff(unsigned long timer) {
  NewMqttMessage("Pump OFF", "/room/bedrooms/1/pumpStatus/1");
  digitalWrite(PUMPPIN, LOW);
  pumpOn = false;
  nextPumpCheck = timer + nextMoistorCheck;
}

void CheckPumpStatus(unsigned long timer) {
  if (moistor > moistorTriggerValue && !pumpOn && timer >= nextPumpCheck) {
    TurnPumpOn(timer);
  }
  
  if (timer >= shutPumpOff && pumpOn) {
    TurnPumpOff(timer);
  }
}
void ReadSoilSensor() {
  moistor = analogRead(MOISTORPIN);
  Serial.print("Soil Mosistor Level: ");
  Serial.println(moistor);
}

void ReadDHTSensor() {
  // put your main code here, to run repeatedly:
  //Read data and store it to variables hum and temp
  hum = dht.readHumidity();
  temp = dht.readTemperature();
}

void NewMqttMessage(String message, String topic) {
  if (mqttMessageIndex < MSG_BUFFER_ARRAY_SIZE) {
    messages[mqttMessageIndex] = MqttMessage(message, topic);
    mqttMessageIndex++;
  } else {
    Serial.println("To Many Messages");
  }
}

void SendMQTT() {
  if (mqttMessageIndex != 0) {
    for (int i = 0; i <= mqttMessageIndex; i++) {
      Serial.print("Publish Topic: ");
      Serial.print(messages[i].Topic);
      Serial.print(" Publish message: ");
      Serial.println(messages[i].Message);

      // convert to char array
      char topic[MSG_BUFFER_SIZE];
      messages[i].Topic.toCharArray(topic, MSG_BUFFER_SIZE);
      char message[MSG_BUFFER_SIZE];
      messages[i].Message.toCharArray(message, MSG_BUFFER_SIZE);

      // publish the message to the mqtt broker
      client.publish(message, topic);
    }
    mqttMessageIndex = 0;
  }
}
