#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define LED_PIN     D4
#define NUM_LEDS    1
#define SOIL_PIN    A0
#define MSG_BUFFER_SIZE 50
#define PLANT_INTERVAL 500
CRGB leds[NUM_LEDS];


const char* ssid = ""; //Your WiFi ssid
const char* password = ""; //Your WiFi password
const char* server_ip = ""; //Sever name or ip(format xxx.xxx.x.x)

int server_port = 1883; //Server port, usually 1883 or 8883
const char* topic_mode = "garden/watering/soil/mode"; // Topic you want to subscribe to
const char* topic_soil = "garden/watering/soil/level"; // Topic you want to publish to
const char* topic_solenoid = "garden/watering/solenoid";
int work_mode = 0; //0 home assistant; 1 - garden assistant

char msg[MSG_BUFFER_SIZE];

unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
unsigned long previousMillis2 = 0;

unsigned long update_interval = 60000; //1min
const unsigned long watering_interval = 20 * 60000; //20min (15min watering, 5min solenoid valve cooldown)
int soil_level = 0;
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(server_ip, server_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected())
    reconnect();
  client.loop();
  currentMillis = millis();
  if (work_mode == 1) {
    if (currentMillis - previousMillis >= update_interval) {
      soil_level = read_soil();
      set_LED(soil_level);
      char string_to_send[16];
      itoa(soil_level, string_to_send, 10);
      client.publish(topic_soil, string_to_send);
      previousMillis = currentMillis;
      if (currentMillis - previousMillis2 >= watering_interval && soil_level <= 50) {
        char watering_time[] = "15"; // Watering time
        client.publish(topic_solenoid, watering_time);
        previousMillis2 = currentMillis;
      }
    }
  }
  else {
    if (currentMillis - previousMillis >= PLANT_INTERVAL) {
      soil_level = read_soil();
      set_LED(soil_level);
      previousMillis = currentMillis;
    }
  }
}
void set_LED(int soil) {
  if (soil > 70)
    leds[0] = CRGB(0, 255, 0);
  else if (soil > 50)
    leds[0] = CRGB(255, 255, 0);
  else
    leds[0] = CRGB(255, 0, 0);
  FastLED.show();
}

int read_soil() {
  int sensor_value = 0;
  for (int i = 0; i < 5; ++i) {
    sensor_value += analogRead(SOIL_PIN);
  }
  sensor_value /= 5 ;
  sensor_value = map(sensor_value, 0, 1023, 100, 0);  // value to output to a PWM pin
  Serial.println("sensor = ");
  Serial.println(sensor_value);
  return sensor_value;
}

void reconnect() {
  char* clientID = "WemosD1_soil_sensor";
  while (!client.connected()) {
    if (client.connect(clientID)) {
      client.subscribe(topic_mode, 0);
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    Serial.print(" ");
  }
  Serial.println();
  for (int i = 0; i < length; i++) {
    Serial.print(payload[i]);
    Serial.print(" ");
  }

  if (strcmp(topic, topic_mode) == 0) {
    work_mode = (char)payload[0] - '0';
    Serial.println();
    Serial.println(work_mode);
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  delay(500);
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
