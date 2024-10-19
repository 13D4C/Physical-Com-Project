#include "DHT.h"

#define DHTPIN 27
#define DHTTYPE DHT22

#define SOILPIN A0
#define RELAY_WATER 13
#define RELAY_FAN 17

#include <WiFi.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

unsigned long currentMillis;
unsigned long test;

float tempset = 30;
float humidset = 65;
float moistureset = 60;
float v;
int valve_1 = 0;
int valve_2 = 0;

unsigned long timer_set;
unsigned long Water_timer_set;
unsigned long fan_timer_set;

bool clock_stat = false;
bool Water_stat = false;
bool fan_stat = false;

const char* ssid = "WIFi ของใครก็ตามที่เอาโค้ดไปรันต่อ";
const char* password = "รหัสไวไฟ";

const char* mqtt_server = "broker.netpie.io";
const char* NETPIE_TOKEN = "YOUR_TOKEN";
const char* NETPIE_SECRET = "YOUR_SECRET";
const char* NETPIE_APPID = "YOUR_APPID";

WiFiClient espClient;
PubSubClient client(espClient);

char msg[50];



DHT dht(DHTPIN, DHTTYPE);

float VDP(float tem ,float hum)
{
    float vpd = (610.78/1000) * pow(2.71828,(tem*17.2614/(tem+237.37)))*(1-(hum/100));
    return vpd;
}

void setup_wifi() 
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(NETPIE_APPID, NETPIE_TOKEN, NETPIE_SECRET)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  dht.begin();

  Wire.begin();
  Wire.setClock(10000);
}

void loop()
{
  currentMillis = millis();

  if(currentMillis > 100)
  {
    if (!client.connected()) 
    {
      reconnect();
    }
  }

  client.loop();

  float humid = dht.readHumidity();
  float temp = dht.readTemperature();
  float soil = readSensor();
  float vlower_set = 0.28;
  float vupper_set = 1.4;
  float v = VDP(temp, humid);


  if (currentMillis - test > 5000 )
  {
    if (isnan(humid) || isnan(temp))
    {
      Serial.println(F("Failed to read from DHT sensor!"));
    }
    Serial.print(F("Humidity: "));
    Serial.print(humid);
    Serial.print(F("% Temperature: "));
    Serial.print(temp);
    Serial.print(F(" C "));
    Serial.print('\n');
    Serial.println(soil);
    Serial.print('\n');

    String payload = String(temp);
    payload.toCharArray(msg, 50);
    client.publish("@private/data/temp", msg);

    payload = String(humid);
    payload.toCharArray(msg, 50);
    client.publish("@private/data/humid", msg);

    payload = String(soil);
    payload.toCharArray(msg, 50);
    client.publish("@private/data/soil", msg);

    payload = String(v);
    payload.toCharArray(msg, 50);
    client.publish("@private/data/vpd", msg);

    Serial.print("Published data: ");
    Serial.println(payload);

    test = currentMillis;
  }


  if (!clock_stat)
  {
    timer_set = currentMillis;
    clock_stat = true;
    Serial.print("timer stats");
    Serial.println(timer_set);
  }

  if (clock_stat && currentMillis - timer_set > 30000 )
  {
      Serial.print("time count : ");
      Serial.println(currentMillis - timer_set);
      clock_stat = false;  
      //send online data
      valve_1 = 0;
      valve_2 = 0;
  }

  if (v <= vlower_set && v > vupper_set)
  {   
      if (temp < tempset && humid < humidset && soil < moistureset)
      {
          if(!Water_stat)
          {
              Water_timer_set = currentMillis;
              Water_stat = true;
          }

          if(Water_timer_set && currentMillis - Water_timer_set < 5000)
          {
                digitalWrite(RELAY_WATER, HIGH);
                valve_1 += 1;
          }
          else
          {
              Water_stat = false;
              digitalWrite(RELAY_WATER, LOW);
          }
      }
      else
      {
          Water_stat = false;
          digitalWrite(RELAY_WATER, LOW);
      }


      if (temp > tempset && humid < humidset && soil < moistureset)
      {
          if(!Water_stat)
          {
              Water_timer_set = currentMillis;
              Water_stat = true;
          }

          if(Water_timer_set && currentMillis - Water_timer_set < 5000)
          {
                digitalWrite(RELAY_WATER, HIGH);
                valve_1 += 1;
          }
          else
          {
              Water_stat = false;
              digitalWrite(RELAY_WATER, LOW);
          }
      }
      else
      {
          Water_stat = false;
          digitalWrite(RELAY_WATER, LOW);
      }


      if (temp > tempset && humid > humidset && soil > moistureset)
      {
          if(!fan_stat)
          {
              fan_timer_set = currentMillis;
              fan_stat = true;
          }

          if(fan_stat && currentMillis - fan_timer_set < 5000)
          {
            digitalWrite(RELAY_FAN, HIGH);
          }
          else
          {
              fan_stat = false;
              digitalWrite(RELAY_FAN, LOW);
          }
      }
      else
      {
          fan_stat = false;
          digitalWrite(RELAY_FAN, LOW);
      }


      if (temp > tempset && humid < humidset && soil > moistureset)
      {
          if(!fan_stat)
          {
              fan_timer_set = currentMillis;
              fan_stat = true;
          }

          if(fan_stat && currentMillis - fan_timer_set < 5000)
          {
              digitalWrite(RELAY_FAN, HIGH);
          }
          else
          {
              fan_stat = false;
              digitalWrite(RELAY_FAN, LOW);
          }
      }
      else
      {
          fan_stat = false;
          digitalWrite(RELAY_FAN, LOW);
      }


      if (temp < tempset && humid > humidset && soil < moistureset)
      {
          if(!fan_stat)
          {
              fan_timer_set = currentMillis;
              Water_timer_set = currentMillis;
              fan_stat = true;
              Water_stat = true;
          }

          if(fan_stat && currentMillis - fan_timer_set < 5000)
          {
              digitalWrite(RELAY_FAN, HIGH);
          }
          else
          {
              digitalWrite(RELAY_FAN, LOW);
          }

          if(Water_timer_set && currentMillis - Water_timer_set < 5000)
          {
                digitalWrite(RELAY_WATER, HIGH);
                valve_1 += 1;
          }
          else
          {
              Water_stat = false;
              digitalWrite(RELAY_WATER, LOW);
          }
      }
      else
      {
          fan_stat = false;
          Water_stat = false;
          digitalWrite(RELAY_FAN, LOW);
          digitalWrite(RELAY_WATER, LOW);
      }
  }
  else
  {
      fan_stat = false;
      Water_stat = false;
      digitalWrite(RELAY_FAN, LOW);
      digitalWrite(RELAY_WATER, LOW);
  }
}

int readSensor() 
{
  int sensorValue = analogRead(SOILPIN);

  int outputValue = map(sensorValue, 0, 1023, 255, 0);

  return outputValue;
}
