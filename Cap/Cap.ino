
//////////////////////////////////////////////
//        RemoteXY include library          //
//////////////////////////////////////////////

// RemoteXY select connection mode and include library 
#define REMOTEXY_MODE__ESP32CORE_WIFI_CLOUD
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RemoteXY.h>

// RemoteXY connection settings 
#define REMOTEXY_WIFI_SSID "Mo Amir"
#define REMOTEXY_WIFI_PASSWORD "pass@2062"
#define REMOTEXY_CLOUD_SERVER "cloud.remotexy.com"
#define REMOTEXY_CLOUD_PORT 6376
#define REMOTEXY_CLOUD_TOKEN "0d9ea571de83ab9ff8951a68159a997f"
#define REMOTEXY_ACCESS_PASSWORD "01153053649"

#define SENSOR_PIN 21  // ESP32 pin GIOP21 connected to DS18B20 sensor's DQ pin
#define anInput 34     //analog feed from MQ135
#define co2Zero 55     //calibrated CO2 0 level

// RemoteXY configurate  
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =   // 307 bytes
  { 255,1,0,36,2,44,1,16,64,5,131,1,1,3,30,7,1,16,1,72,
  111,109,101,32,80,97,103,101,0,131,0,32,3,31,7,2,16,1,71,114,
  97,112,104,32,80,97,103,101,0,129,0,11,30,43,7,1,144,84,101,109,
  112,101,114,97,116,117,114,101,44,0,68,51,2,12,59,49,2,228,165,135,
  94,67,79,50,0,77,111,105,115,116,117,114,101,0,84,101,109,112,0,67,
  5,37,25,23,8,3,228,16,11,67,5,38,46,22,8,3,228,16,11,67,
  5,38,67,22,8,3,228,16,11,129,0,9,19,24,6,3,202,67,79,50,
  0,129,0,8,38,24,6,3,202,84,101,109,112,46,0,129,0,6,59,24,
  6,3,202,77,111,105,115,116,117,114,101,0,3,131,21,62,22,8,2,8,
  0,70,24,39,80,9,9,0,24,6,1,129,0,23,62,16,6,1,228,49,
  57,51,48,55,0,129,0,20,39,27,6,1,144,77,111,105,115,116,117,114,
  101,32,0,129,0,16,47,31,6,1,144,38,32,67,79,50,32,66,111,120,
  0,131,0,18,90,29,7,3,16,1,84,101,120,116,32,80,97,103,101,0,
  67,5,0,71,33,8,2,228,16,251,67,5,33,71,30,8,2,228,16,251,
  129,0,4,82,14,6,0,201,67,79,50,32,0,129,0,19,82,17,6,0,
  201,65,108,97,114,109,0 };
  
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
  char ST_1[251];  // string UTF8 end zero 
  char ST_2[251];  // string UTF8 end zero
  uint8_t Led; // led state 0 .. 1 
 

    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop)

/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////

OneWire oneWire(SENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

float moisture_sensor_pin = 35;
float soil_moisture;
float tempC;  // temperature in Celsius

void setup() 
{
  RemoteXY_Init();
  pinMode(anInput, INPUT);  //MQ135 analog feed set for input
  Serial.begin(9600);       //serial comms for debuging
  DS18B20.begin();          // initialize the DS18B20 sensor  
}

void loop() 
{ 
  RemoteXY_Handler ();
  
  
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
  co2ppm = map(co2ppm, 1150, 2060, 100, 0);
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

  if (co2ppm > 1578 ) 
    RemoteXY.Led = 255;   // then turn on red light
  else                        // else
    RemoteXY.Led = 0;     // turn off red

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
    strcpy  (RemoteXY.ST_1, "↑CO2 -->");   
    strcpy  (RemoteXY.ST_2, " ↑T & ↓M");     
    /*  current position A */
  }
  else if (RemoteXY.S==1) {
        strcpy  (RemoteXY.ST_1, "stable CO2--> "); 
        strcpy  (RemoteXY.ST_2, " stable T & M");    
    /*  current position B */
  }
  else if (RemoteXY.S==2) {
    strcpy  (RemoteXY.ST_1, "↓CO2--> ");  
    strcpy  (RemoteXY.ST_2, "↓T & ↑M ");      /*  current position C */
  }  


  // TODO you loop code
  // use the RemoteXY structure for data transfer
  // do not call delay() 


}