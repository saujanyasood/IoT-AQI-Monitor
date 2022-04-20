#include <SDS011.h>
#include <PubSubClient.h>
#include "Adafruit_BME680.h"
#include <WiFi.h>
 
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme; // I2C

#define WLAN_SSID <wifi name>
#define WLAN_PASS <wifi password>

#define TOKEN <token>
#define MQTT_CLIENT_NAME <username>

// DEFINE CONSTANTS FOR UBIDOTS
#define VARIABLE_LABEL1 "aqi" // Assing the variable label
#define VARIABLE_LABEL2 "temperature" // Assing the variable label
#define VARIABLE_LABEL3 "humidity"
#define VARIABLE_LABEL4 "pm25"
#define VARIABLE_LABEL5 "pm10"
#define VARIABLE_LABEL6 "co"
#define VARIABLE_LABEL7 "quality"
#define DEVICE_LABEL "ESP32"

char mqttBroker[]  = "industrial.api.ubidots.com";
char payload[1000];
char topic1[150];
char topic2[150];
char topic3[150];
char topic4[150];
char topic5[150];
char topic6[150];
char topic7[150];
 
// Space to store values to send
char str_aqi[20];
char str_temperature[20];
char str_humidity[20];
char str_pm25[20];
char str_pm10[20];
char str_co[20];
 
/**************
  Auxiliar Functions
**************/
WiFiClient ubidots;
PubSubClient client(ubidots);

void callback(char* topic, byte* payload, unsigned int length)
{
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);
  Serial.write(payload, length);
  Serial.println(topic);
}
 
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.println("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_NAME, TOKEN, ""))
    {
      Serial.println("Connected");
    } else
    {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

// Defining constant variables
const int numReadingsPM10 = 24;
const int numReadingsPM25 = 24;
const int numReadingsCO = 8;

// DEFINING VARIABLES
float p10,p25;
int AQI;
int iMQ7 = 25;
int MQ7Raw = 0;
int MQ7ppm = 0;
double RvRo;
int ConcentrationINmgm3;
int readingsPM10[numReadingsPM10];      // the readings from the analog input
int readIndexPM10 = 0;              // the index of the current reading
int totalPM10 = 0;                  // the running total
int averagePM10 = 0;                // the average
int readingsPM25[numReadingsPM25];      // the readings from the analog input
int readIndexPM25 = 0;              // the index of the current reading
int totalPM25 = 0;                  // the running total
int averagePM25 = 0;                // the average
int readingsCO[numReadingsCO];      // the readings from the analog input
int readIndexCO = 0;              // the index of the current reading
int totalCO = 0;                  // the running total
int averageCO = 0;                // the average

void setup() 
{
  Serial.begin(9600);
  while (!Serial);
  Serial.println(F("AQI Monitor"));
  my_sds.begin(16,17);
  delay(10);
  Serial.print(F("Connecting to "));
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());
  client.setServer(mqttBroker, 1883);
  client.setCallback(callback);

  // Initialising arrays for pm10, pm2.5 and co
  for (int thisReading1 = 0; thisReading1 < numReadingsPM10; thisReading1++) {
    readingsPM10[thisReading1] = 0;
  }
  for (int thisReading2 = 0; thisReading2 < numReadingsPM25; thisReading2++) {
    readingsPM25[thisReading2] = 0;
  }
  for (int thisReading3 = 0; thisReading3 < numReadingsCO; thisReading3++) {
    readingsCO[thisReading3] = 0;
  }
 
  if (!bme.begin()) 
  {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }
 
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
}
 
void loop() 
{
   if (!client.connected())
  {
    reconnect();
  }
  if (! bme.performReading())
  {
    Serial.println("Failed to perform reading :(");
    return;
  }

  float temperature = bme.temperature;
  float humidity = bme.humidity;
  float gas = bme.gas_resistance / 1000.0;

   MQ7Raw = analogRead( iMQ7 );
   RvRo = MQ7Raw * (3.3 / 4095);
   MQ7ppm = 3.027*exp(1.0698*( RvRo ));
   error = my_sds.read(&p25,&p10);
   if (! error) {
     Serial.println("P2.5: "+String(p25));
     Serial.println("P10:  "+String(p10));
   }
   
   Serial.println("PM2.5: "+String(p25));
   Serial.println("PM10:  "+String(p10));

   ConcentrationINmgm3 = MQ7ppm* (28.06/24.45); //Converting PPM to mg/m3. Where 28.06 is Molecular mass of CO and 24.45 is the Molar volume
   totalPM10 = totalPM10 - readingsPM10[readIndexPM10];
   readingsPM10[readIndexPM10] = p10;
   totalPM10 = totalPM10 + readingsPM10[readIndexPM10];
   readIndexPM10 = readIndexPM10 + 1;
   if (readIndexPM10 >= numReadingsPM10) {
    readIndexPM10 = 0;
   }
   averagePM10 = totalPM10 / numReadingsPM10;
   Serial.print("Average PM10: ");
   Serial.println(averagePM10);
   totalPM25 = totalPM25 - readingsPM25[readIndexPM25];
   readingsPM25[readIndexPM25] = p25;
   totalPM25 = totalPM25 + readingsPM25[readIndexPM25];
   readIndexPM25 = readIndexPM25 + 1;
   if (readIndexPM25 >= numReadingsPM25) {
    readIndexPM25 = 0;
   }
   averagePM25 = totalPM25 / numReadingsPM25;
   Serial.print("Average PM2.5: ");
   Serial.println(averagePM25);
   totalCO = totalCO - readingsCO[readIndexCO];
   readingsCO[readIndexCO] = ConcentrationINmgm3;
   totalCO = totalCO + readingsCO[readIndexCO];
   readIndexCO = readIndexCO + 1;
   if (readIndexCO >= numReadingsCO) {
    readIndexCO = 0;
   }
   averageCO = totalCO / numReadingsCO;
   Serial.print("Average CO: ");
   Serial.println(averageCO);
   if (averagePM10 > averagePM25){
      AQI = averagePM10;
     }
   else {
     AQI = averagePM25;
    } 

  char quality[30];

  Serial.print("AQI = ");
  Serial.print(AQI);
  Serial.println();

  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" *C");
 
  Serial.print("Humidity = ");
  Serial.print(humidity);
  Serial.println(" %");

  if(AQI <= 50){
    strcpy(quality, "Good");
  }
  else if(AQI <= 100){
    strcpy(quality, "Satisfactory");
  }
  else if(AQI <= 200){
    strcpy(quality, "Moderately Polluted");
  }
  else if(AQI <= 300){
    strcpy(quality, "Poor");
  }
  else if(AQI <= 400){
    strcpy(quality, "Very Poor");
  }
  else strcpy(quality, "Severe");
  
  Serial.print("Quality: ");
  Serial.print(quality);
  Serial.println();

  // Publish data to Ubidots
  dtostrf(AQI, 4, 2, str_aqi);
  dtostrf(temperature, 4, 2, str_temperature);
  dtostrf(humidity, 4, 2, str_humidity);
  dtostrf(averagePM25, 4, 2, str_pm25);
  dtostrf(averagePM10, 4, 2, str_pm10);
  dtostrf(averageCO, 4, 2, str_co);

  sprintf(topic1, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", "");
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL1);
  sprintf(payload, "%s {\"value\": %s}}", payload, str_aqi);
  Serial.println("Publishing aqi to Ubidots Cloud");
  client.publish(topic1, payload);
 
  sprintf(topic2, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", "");
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL2);
  sprintf(payload, "%s {\"value\": %s}}", payload, str_temperature);
  Serial.println("Publishing temperature to Ubidots Cloud");
  client.publish(topic2, payload);
 
  sprintf(topic3, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", "");
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL3);
  sprintf(payload, "%s {\"value\": %s}}", payload, str_humidity);
  Serial.println("Publishing humidity data to Ubidots Cloud");
  client.publish(topic3, payload);
 
  sprintf(topic4, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", "");
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL4);
  sprintf(payload, "%s {\"value\": %s}}", payload, str_pm25);
  Serial.println("Publishing PM2.5 data to Ubidots Cloud");
  client.publish(topic4, payload);
 
  sprintf(topic5, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", "");
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL5);
  sprintf(payload, "%s {\"value\": %s}}", payload, str_pm10);
  Serial.println("Publishing PM 10 data to Ubidots Cloud");
  client.publish(topic5, payload);
 
  sprintf(topic6, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", "");
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL6);
  sprintf(payload, "%s {\"value\": %s}}", payload, str_co);
  Serial.println("Publishing CO data to Ubidots Cloud");
  client.publish(topic6, payload);

  sprintf(topic6, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", "");
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL7);
  sprintf(payload, "%s {\"value\": %s}}", payload, quality);
  Serial.println("Publishing air quality data to Ubidots Cloud");
  client.publish(topic7, payload);
 
  Serial.println();
  client.loop();
  delay(6000);
}
