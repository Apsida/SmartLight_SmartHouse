#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <GyverStepper.h>
#include <FastLED.h>


#define lenLamp 6
#define Lamp_pin 27
#define Sun_pin 14
#define Sun_len 3
#define PowerCheckPin 36
#define LightSensorRoom 2
#define LightSensorStreet 13

CRGB Lamp[lenLamp];
CRGB Sun[Sun_len];
GStepper<STEPPER4WIRE> stepper(2048, 18, 23, 19, 5);
WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "Keenetic-6741";
const char* password = "CxUEXoEn";
const char* mqtt_server = "192.168.11.96";
long lastMsg = 0;
char msg[50];
int value = 0;
int brightess = 0;
int pos = 0;
int br = 0;
int fl = 1;
int valueTakeInfo = 0;
int dayPowerInfo[24] = {};
int stat = 0;
uint32_t tmr;
byte counter;
byte steps = 1;  

void brightness_set(byte* info, unsigned int len){
  String temp = String((char*) info);
  int bright = String(temp.substring(0,len)).toInt();
  for (int i = 0; i < lenLamp; i++){
      Lamp[i] =  CRGB(bright, bright, bright);
      FastLED.show();
    }
}

void ton_set(byte* info, unsigned int len){
  char* end = nullptr;
  long ton = strtol((const char*) info, &end, 10);
  switch (ton){
    case 0:
      for (int i = 0; i < lenLamp; i++){
      Lamp[i] =  CRGB(255, 255, 255);
      FastLED.show();
    }
    case 1:
      for (int i = 0; i < lenLamp; i++){
      Lamp[i] =  CRGB(150, 230, 150);
      FastLED.show();
    }
    case 2:
      for (int i = 0; i < lenLamp; i++){
      Lamp[i] =  CRGB(150, 150, 230);
      FastLED.show();
    }
    }
}

void color_set(byte* info, unsigned int len){
  String color = String((char*) info);
  int red = String(color.substring(color.indexOf("(")+1,color.indexOf(","))).toInt();
  int green = String(color.substring(color.indexOf(",")+2,color.lastIndexOf(","))).toInt();
  int blue = String(color.substring(color.lastIndexOf(",")+2,color.indexOf(")"))).toInt();
  for (int i = 0; i < lenLamp; i++ ) {
    Lamp[i] = CRGB(green, red, blue);
    FastLED.show();
  }
}

void smart_mode(bool stat){
  int dif = map(analogRead(LightSensorStreet), 0, 4095, 0, 255) - map(analogRead(LightSensorRoom), 0, 4095, 0, 255) - map(analogRead(LightSensorRoom), 0, 4095, 0, 255);
  int val = brightess;
  if (brightess <= val and dif != 0){ 
        int np = stepper.getCurrent();
        if (np > 2020 and dif > 10){
          pos = 0;
          stepper.setTarget(pos);
        }
        else if (np > 1020 and dif < -10){    
          for (int i = 0; i < lenLamp; i++){
            Lamp[i] =  CHSV(brightess, 0, brightess);
            FastLED.show();
          }
          brightess = brightess + dif;
        }
        else if (np < 5 and dif > 10){
          for (int i = 0; i < lenLamp; i++){
            Lamp[i] =  CHSV(brightess, 0, brightess);
            FastLED.show();
          }
          brightess = brightess + dif;
        }
        else if (np < 3 and dif < -10){
          pos = 2048;
          stepper.setTarget(pos);
      }
  }
  if(brightess <= 0){
        brightess = 0;
        for (int i = 0; i < lenLamp; i++){
            Lamp[i] =  CHSV(brightess, 0, brightess);
            FastLED.show();
          }
      } else if(brightess > val){
        brightess = val;
        for (int i = 0; i < lenLamp; i++){
            Lamp[i] =  CHSV(brightess, 0, brightess);
            FastLED.show();
          }
      }
}
void garland_mode(){
  for (int i = 0; i < lenLamp; i++ ) {
    Lamp[i] = CHSV(counter + i * steps, 255, 255);
    FastLED.show();
  }
  counter ++
  delay(500);
}

void callback(char* topic, byte* payload, unsigned int len) {
  String topic_s = String(topic);
  Serial.println(topic_s);
  Serial.println(len);
  for (int i=0;i<len;i++) {
    Serial.print((char)payload[i]);
  }
  
  if(topic_s == String("light_brightness")){
    brightness_set(payload,len);
  }else if (topic_s == String("light_ton")){
    ton_set(payload,len);
  }else if (topic_s == String("Smart")){
    if ((char)payload[0] == '1'){
      stat = 2;
    }else{
      stat = 0;
    }
  }else if (topic_s == String("Garland")){
    if ((char)payload[0] == '1'){
      stat = 1;
    }else{
      stat = 0;
    }
  }else if (topic_s == String("light_color")){
    color_set(payload,len);
  }
}

  // if (topic_s == "light_color"){
  //   Serial.println("in");
  //   color_set(payload,len);
  // }

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

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

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");

      client.subscribe("butt");
      client.subscribe("light_color");
      client.subscribe("light_brightness");
      client.subscribe("light_ton");
      client.subscribe("Smart");
      client.subscribe("Garland");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  FastLED.addLeds<WS2812, Lamp_pin, RGB>(Lamp, lenLamp);
  FastLED.addLeds<WS2812, Sun_pin, RGB>(Sun, Sun_len);

  client.subscribe("butt");
  client.subscribe("light_color");
  client.subscribe("light_brightness");
  client.subscribe("light_ton");
  client.subscribe("Smart");
  client.subscribe("Garland");

}

void loop() {
  if (millis() - tmr >= 1000){
    tmr = millis();
    if (stat == 1){
      garland_mode();
    }else if (stat == 2){
      smart_mode();
    }
    for (int i = 0; i < Sun_len; i++){
      Sun[i] =  CRGB(br, br, br);
      FastLED.show();
    }
    br = br + 10 * fl;
    if (br == 255 || br == 0){
      fl = fl * (-1);
    }
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
