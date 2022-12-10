// RemoteXY select connection mode and include library
#define REMOTEXY_MODE__ESP32CORE_WIFI_CLOUD

// the libraries 
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RemoteXY.h>

// RemoteXY connection settings 
#define REMOTEXY_WIFI_SSID "Mo Amir"
#define REMOTEXY_WIFI_PASSWORD "pass@2062"
#define REMOTEXY_CLOUD_SERVER "cloud.remotexy.com"
#define REMOTEXY_CLOUD_PORT 6376
#define REMOTEXY_CLOUD_TOKEN "c13a2f0c1a18149c8004f51e59704c9a"

// pins of the sensors 
#define SENSOR_PIN 21  // ESP32 pin GIOP21 connected to DS18B20 sensor's DQ pin
#define anInput 34     //analog feed from MQ135
#define co2Zero 55     //calibrated CO2 0 level

// RemoteXY configurate  
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =   // 131 bytes
  { 255,1,0,35,2,124,0,16,24,1,68,51,2,1,59,51,228,165,135,94,
  67,79,50,0,77,111,105,115,116,117,114,101,0,84,101,109,112,0,67,5,
  40,55,19,6,228,16,11,67,5,40,62,19,6,228,16,11,67,5,40,69,
  19,6,228,16,11,129,0,8,55,24,6,17,67,79,50,0,129,0,6,62,
  24,6,17,84,101,109,112,46,0,129,0,3,69,24,6,17,77,111,105,115,
  116,117,114,101,0,3,3,3,75,8,22,134,0,67,5,13,76,48,10,228,
  16,251,67,5,13,86,48,10,228,16,251 };
  
// this structure defines all the variables and events of your control interface 
struct {

    // input variables
  uint8_t S; // =0 if select position A, =1 if position B, =2 if position C, ... 

    // output variables
  float Graph_co2ppm;
  float Graph_tempC;
  float Graph_soil_moisture;
  char C[11];  // string UTF8 end zero 
  char T[11];  // string UTF8 end zero 
  char M[11];  // string UTF8 end zero 
  char ST1[251];  // string UTF8 end zero 
  char ST2[251];
    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop)

/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////
// pins of the Temp.
OneWire oneWire(SENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

// pin + variable
float moisture_sensor_pin = 35;
float soil_moisture;
float tempC;  // temperature in Celsius

void setup() {
  RemoteXY_Init();
  pinMode(anInput, INPUT);  //MQ135 analog feed set for input
  Serial.begin(9600);       //serial comms for debuging
  DS18B20.begin();          // initialize the DS18B20 sensor
}

void loop() {
  RemoteXY_Handler();

  //co2 starts
  int co2now[10];               //int array for co2 readings
  int co2raw = 0;               //int for raw value of co2
  int co2ppm = 0;               //int for calculated ppm
  int X = 0;                    //int for averaging
  for (int x = 0; x < 10; x++)  //samplpe co2 10x over 2 seconds
  {
    co2now[x] = analogRead(34);
  }
  for (int x = 0; x < 10; x++)  //add samples together
  {
    X = X + co2now[x];
  }
  co2raw = X / 10;            //divide samples by 10
  co2ppm = co2raw - co2Zero;  //get calculated %
  co2ppm = map(co2ppm, 1150, 2050, 100, 0);
  Serial.print("CO2 CONC. =");
  Serial.print(co2ppm);  // prints the value read
  Serial.println(" %");
  // co2 ended

  // Temp starts
  DS18B20.requestTemperatures();       // send the command to get temperatures
  tempC = DS18B20.getTempCByIndex(0);  // read temperature in °C
  Serial.print("Temperature: ");
  Serial.print(tempC);  // print the temperature in °C
  Serial.print("°C");
  // Temp ended

  //Soil Moisture Code Start
  soil_moisture = ((analogRead(moisture_sensor_pin) / -1) + 4095) / 100;  //Soil Moisture calibration
  soil_moisture = map(soil_moisture, 17, 0, 100, 0);
  Serial.print("Soil Moisture Value : ");
  Serial.print(soil_moisture);
  Serial.println(" %");
  //Soil Moisture Code End
  
  // codes for the communication
  RemoteXY.Graph_co2ppm = co2ppm;
  RemoteXY.Graph_soil_moisture = soil_moisture;
  RemoteXY.Graph_tempC = tempC;
 // Codes for text of each sensor 
  dtostrf(tempC, 0, 2, RemoteXY.T);     
  dtostrf(soil_moisture, 0, 2, RemoteXY.M);  
  dtostrf(co2ppm, 0, 2, RemoteXY.C);      
  
 // codes for the selection
  if (RemoteXY.S==0) {
    strcpy  (RemoteXY.ST1, "↑CO2 -->");   
    strcpy  (RemoteXY.ST2, " ↑T & ↓M");     
    /*  current position A */
  }
  else if (RemoteXY.S==1) {
        strcpy  (RemoteXY.ST1, "stable CO2--> "); 
        strcpy  (RemoteXY.ST2, " stable T & M");    
    /*  current position B */
  }
  else if (RemoteXY.S==2) {
    strcpy  (RemoteXY.ST1, "↓CO2--> ");  
    strcpy  (RemoteXY.ST2, "↓T & ↑M ");      /*  current position C */
  }  


}