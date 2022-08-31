#include "Ubidots.h"
#include <DallasTemperature.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_Sensor.h> 
#include <Adafruit_ADXL345_U.h>
#include "MAX30100.h"
#include "MAX30100_PulseOximeter.h"




const char* UBIDOTS_TOKEN = "BBFF-zqTiPNdVa059CSNG8pNPXKjigHdSW9";  // Put here your Ubidots TOKEN
const char* WIFI_SSID = "Room 245";      // Put here your Wi-Fi SSID
const char* WIFI_PASS = "pisosr0245";      // Put here your Wi-Fi password
Ubidots ubidots(UBIDOTS_TOKEN, UBI_HTTP);



#define DHTPIN D8     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);


// Data wire is plugged into D4 on the NodeMCU
#define ONE_WIRE_BUS D4
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);


Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified();
#define REPORTING_PERIOD_accelerometer    500
uint32_t tsLastReport_acc = 0;
float Xposition;
float Yposition;
float Zposition;


#define REPORTING_PERIOD_MS     1000
PulseOximeter pox;
float a,b;
int i = 10;
float c = 0;
uint32_t tsLastReport = 0;

float ecgSignal = 0;
#define ecgPin A0



void onBeatDetected()
{
    Serial.println("STAY STILL Measuring...");
}

void setup()
{
    Serial.begin(115200);
    ubidots.wifiConnect(WIFI_SSID, WIFI_PASS);
    sensors.begin();
    dht.begin();
    pinMode(ecgPin, INPUT);

    if (!pox.begin()) {
        Serial.println("POX FAILED");
        for(;;);
    } else {
        Serial.println("POX SUCCESS");
    }
   if(!accel.begin())
   {
      Serial.println("Accelerometer not found");
   }

    // The default current for the IR LED is 50mA and it could be changed
    //   by uncommenting the following line. Check MAX30100_Registers.h for all the
    //   available options.
    pox.setIRLedCurrent(MAX30100_LED_CURR_4_4MA);

    // Register a callback for the beat detection
    pox.setOnBeatDetectedCallback(onBeatDetected);
    
}

void loop()
{
/************ Starting Pulse Oximeter ***********************************/
Wire.beginTransmission(87);
    pox.update();

    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
      a = pox.getHeartRate();
      b = pox.getSpO2();
          if( a == 0 || b == 0)
            {
              return;
            }

              if (i>0)
              {
                c = ((c+a)/2);
                i = i-1;
                Serial.println(i);
              }
             if (i==0)
              {
                i=10;
                Serial.print("Heart rate: ");
                Serial.print(c);
                Serial.print(" bpm & SpO2: ");
                Serial.print(b);
                Serial.println("%");
                c=0;             
              } 
        tsLastReport = millis();
    }
Wire.endTransmission(87);

/*********** Ending Pulse Oximeter ***********/



/******************Starting Accelerometer ********************/
Wire.beginTransmission(83);

   if (millis() - tsLastReport_acc > REPORTING_PERIOD_accelerometer) {
       sensors_event_t event; 
       accel.getEvent(&event);
    
       Xposition = event.acceleration.x;
       Yposition = event.acceleration.y;
       Zposition = event.acceleration.z;
    
       Serial.println(Xposition);
       Serial.println(Yposition);
       Serial.println(Zposition);

      tsLastReport_acc = millis();
   }

Wire.endTransmission(83);
/****************** Ending Accelerometer *********************/



/*****************Starting Body Temperature *****************/
    sensors.requestTemperatures(); // Send the command to get temperatures
    float tempC = sensors.getTempCByIndex(0);
  
    // Check if reading was successful
    if(tempC != DEVICE_DISCONNECTED_C) 
    {
      Serial.print("Temperature is: ");
      Serial.println(tempC);
    } 
//    else
//    {
//      tempC = 32;
//    }

/*****************Ending Body Temperature *****************/




/*****************Starting DHT Sensor *****************/

    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      h = 0;
      t = 0;
    }

/*****************Ending DHT Sensor *****************/


/******************* ECG Signal **********************/
      ecgSignal = analogRead(ecgPin);


/********************Sending Data to Ubidots **************/

//      Serial.println(tempC);
//      Serial.println(t);
//      Serial.println(h);
//      Serial.println(c);
//      Serial.println(b);
//      Serial.println(Xposition);
//      Serial.println(Yposition);
//      Serial.println(Zposition);
//      Serial.println(ecgSignal);
              
    ubidots.add("BodyTemperature", tempC);
    ubidots.add("RoomTemperature", t);
    ubidots.add("RoomHumidity", h);
    ubidots.add("HeartRate", c);
    ubidots.add("OxigenLevel", b);
    ubidots.add("Xposition", Xposition);
    ubidots.add("Yposition", Yposition);
    ubidots.add("Zposition", Zposition);
    ubidots.add("ECGsignal", ecgSignal);

    
    bool bufferSent = false;
    bufferSent = ubidots.send();  // Will send data to a device label that matches the device Id

    if (bufferSent) {
      // Do something if values were sent properly
      Serial.println("Values sent by the device");
    }

}
