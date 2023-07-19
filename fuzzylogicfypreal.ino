//----------------------------------------Include the NodeMCU ESP8266 Library
//----------------------------------------see here: https://www.youtube.com/watch?v=8jMr94B8iN0 to add NodeMCU ESP12E ESP8266 library and board (ESP8266 Core SDK)
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <BlynkSimpleEsp8266.h>
#define BLYNK_PRINT Serial
#include <math.h>
#include <Fuzzy.h>
//----------------------------------------
const int RXPin = D4, TXPin = D5;
const uint32_t GPSBaud = 9600; 
SoftwareSerial gps_module(RXPin, TXPin);

TinyGPSPlus gps;
WidgetMap myMap(V0);
BlynkTimer timer;
WidgetLED led1(V15);
WidgetLED led2(V16);
WidgetLED led3(V17);
//----------------------------------------Include the DHT Library
// Fuzzy
Fuzzy *fuzzy = new Fuzzy();

// FuzzyInput Distance
FuzzySet *small = new FuzzySet(0, 0, 50, 100);
FuzzySet *moderate = new FuzzySet(100, 150, 150, 200);
FuzzySet *large = new FuzzySet(200, 250, 300, 300);

// FuzzyInput Speed
FuzzySet *low = new FuzzySet(0, 0, 10, 15);
FuzzySet *medium= new FuzzySet(15, 25, 25, 35);
FuzzySet *high = new FuzzySet(35, 40, 50, 50);

// FuzzyOutput correction
FuzzySet *low2 = new FuzzySet(0, 0, 0.1625, 0.33);
FuzzySet *medium2 = new FuzzySet(0.33, 0.5, 0.5, 0.66);
FuzzySet *high2 = new FuzzySet(0.66, 0.825, 1, 1);

/////////////////////////////////
#define ON_Board_LED 2  //--> Defining an On Board LED, used for indicators when the process of connecting to a wifi router
float gps_speed;
int sats;
String satellite_orientation;
float satellite_accuracy;
float velocity;

float previouslat = 0.0;
float previouslong = 0.0;



char auth[] = "Ef0f7h3GrXqD6EHdNcKaOFpi5fggJyv6";              
char ssid[] = "cb";
char pass[] = "arief123";

//unsigned int move_index;         
unsigned int move_index = 1; 
//----------------------------------------SSID and Password of your WiFi router.
const char* ssids = "cb"; //--> Your wifi name or SSID.
const char* password = "arief123"; //--> Your wifi password.
//----------------------------------------

//----------------------------------------Host & httpsPort
const char* host = "script.google.com";
const int httpsPort = 443;

//----------------------------------------

WiFiClientSecure client; //--> Create a WiFiClientSecure object.

String GAS_ID = "AKfycbwVAFcf86NtN4KnH5UJFCs4p-n7on8QNWFowS_pnCnsXjC28OCuOmgwFAc9CeEDWGKc"; //--> spreadsheet script ID

//============================================================================== void setup
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  WiFi.begin(ssids, password); //--> Connect to your WiFi router
  Serial.println("");
    
  pinMode(ON_Board_LED,OUTPUT); //--> On Board LED port Direction output
  digitalWrite(ON_Board_LED, HIGH); //--> Turn off Led On Board

  //----------------------------------------Wait for connection
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    //----------------------------------------Make the On Board Flashing LED on the process of connecting to the wifi router.
    digitalWrite(ON_Board_LED, LOW);
    delay(250);
    digitalWrite(ON_Board_LED, HIGH);
    delay(250);
    //----------------------------------------
  }
  //----------------------------------------
  digitalWrite(ON_Board_LED, HIGH); //--> Turn off the On Board LED when it is connected to the wifi router.
  //----------------------------------------If successfully connected to the wifi router, the IP Address that will be visited is displayed in the serial monitor
  Serial.println("");
  Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  //----------------------------------------
  Blynk.begin(auth, ssid, pass, IPAddress(139,59,224,74), 8080);
  timer.setInterval(5000L, checkGPS);
  gps_module.begin(GPSBaud);
  fuzzytest();
  client.setInsecure();
}

void checkGPS(){
  if (gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
      Blynk.virtualWrite(V4, "GPS ERROR");  
  }
}
//==============================================================================
//============================================================================== void loop
void loop() {

  while (gps_module.available() > 0) 
  {
    //displays information every time a new sentence is correctly encoded.
    if (gps.encode(gps_module.read()))
    displayInfo();
  }
  Blynk.run();
  timer.run();
}
//==============================================================================
//============================================================================== void sendData
// Subroutine for sending data to Google Sheets
void sendData(float latitude, float longitude) {

  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);
  
  //----------------------------------------Connect to Google host
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
  //----------------------------------------

  //----------------------------------------Processing data and sending data
  String string_latitude =  String(latitude, 5);
  // String string_temperature =  String(latitude, DEC); 
  String string_longitude =  String(longitude, 5); 
  String url = "/macros/s/" + GAS_ID + "/exec?longitude=" + string_longitude + "&latitude=" + string_latitude;
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
         "Connection: close\r\n\r\n");

  Serial.println("request sent");
  //----------------------------------------

  //----------------------------------------Checking whether the data was sent successfully or not
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successfull!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.print("reply was : ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println("==========");
  Serial.println();
  delay(10000);
} 
//==============================================================================


void displayInfo()
{
  int r = 6371;
  float pi = 3.1415926535897932384626433832795;

  if (gps.location.isValid()) 
  {
    float lati = (gps.location.lat());
    float lon = (gps.location.lng());
    
    Blynk.virtualWrite(V1, String(lati,5));   
    Blynk.virtualWrite(V2, String(lon,5)); 

    Serial.println("LAT:  ");
    Serial.println(lati,5);  // float to x decimal places
    Serial.println("LONG: ");
    Serial.println(lon,5);
    
    //get speed
    gps_speed = gps.speed.kmph();
    Blynk.virtualWrite(V3, gps_speed);

    //get satellites
    sats = gps.satellites.value();
    Blynk.virtualWrite(V9, sats);

    delay(5000);

    float comparelat =  (lati - previouslat);
    float comparelong =  (lon - previouslong);

    previouslat = lati;
    previouslong = lon;
    
    Blynk.virtualWrite(V4, String(previouslat,5));
    Blynk.virtualWrite(V5, String(previouslong,5));
    Blynk.virtualWrite(V11, String(comparelat,5));
    Blynk.virtualWrite(V12, String(comparelong,5));


    float latia1 = previouslat * (pi/180);
    float latia2 = lati * (pi/180);
    float latimi = comparelat * (pi/180);
    float longimi = comparelong * (pi/180);

    float a = sin(latimi/2) * sin(latimi/2) + cos(latia1) * cos (latia2) * sin(longimi/2) * sin(longimi/2);
    float b = 2 * atan2(sqrt(a), sqrt(1-a));
    float d = r * b * 100;

    //float dt = (acos(sin(latia1) * sin(latia2) + cos(latia1) * cos(latia2) * cos(longimi))) * 6371;

    //float distance = (acos(cos(radians(90-lati))*cos(radians(90-previouslat))+sin(radians(90-lati))*sin(radians(90-previouslat))*cos(radians(lon-previouslong)))*6371)*1000;
    float timee = d / gps_speed;

    Blynk.virtualWrite(V13, String(d,5));
    Blynk.virtualWrite(V14, String(timee));

    delay(5000);

    float input1 = gps_speed;
    float input2 = d;
      
    fuzzy->setInput(1, input1);
    fuzzy->setInput(2, input2);
  
    fuzzy->fuzzify();

    Serial.println("\nInput: ");
    Serial.print("\nspeed: Small-> ");
    Serial.print(small->getPertinence());
    Serial.print(", Moderate-> ");
    Serial.println(moderate->getPertinence());
    Serial.print(", Large-> ");
    Serial.println(large->getPertinence());

    Serial.print("\ndistance: Low-> ");
    Serial.print(low->getPertinence());
    Serial.print(",  Medium-> ");
    Serial.print(medium->getPertinence());
    Serial.print(", High-> ");
    Serial.println(high->getPertinence());

    float output = fuzzy->defuzzify(1);

    Serial.println("\nOutput: ");
    Serial.print("\tcorrection: Low-> ");
    Serial.print(low2->getPertinence());
    Serial.print(", Medium-> ");
    Serial.print(medium2->getPertinence());
    Serial.print(", High-> ");
    Serial.println(high2->getPertinence());

    Serial.println("Result: ");
    Serial.print("\t\t\tCorrection: ");
    Serial.print(output);
    Serial.print("\n--------------------------------------------------------------------");
    
    float latis;
    float longis;



    if ( 0.66 < output && output < 1)
    {
     latis = lati - 0.00028;
     longis = lon - 0.00024;
     Serial.print("besar");
     led3.on();
     led2.off();
     led1.off();
    delay(1000);
    }

    else if ( 0.33 < output && output < 0.66)
    {
     latis = lati - 0.00014;
     longis = lon - 0.00012;
     Serial.print("sederhana");
     led3.off();
     led2.on();
     led1.off();
    delay(1000);
    }

    else
    {
     latis = lati - 0.00004;
     longis = lon - 0.00002;
     Serial.print("kecil");
     led3.off();
     led2.off();
     led1.on();
    delay(1000);
    }
    Blynk.virtualWrite(V7, String(latis,5));
    Blynk.virtualWrite(V8, String(longis,5));
    myMap.location(move_index, latis, longis, "GPS_Location");
  }
}

void fuzzytest()
{

  // FuzzyInput
  FuzzyInput *distance = new FuzzyInput(1);

  distance->addFuzzySet(small);
  distance->addFuzzySet(moderate);
  distance->addFuzzySet(large);
  fuzzy->addFuzzyInput(distance);

  // FuzzyInput
  FuzzyInput *speedd = new FuzzyInput(2);

  speedd->addFuzzySet(low);
  speedd->addFuzzySet(medium);
  speedd->addFuzzySet(high);
  fuzzy->addFuzzyInput(speedd);

  // FuzzyOutput
  FuzzyOutput *correction = new FuzzyOutput(1);

  correction->addFuzzySet(low2);
  correction->addFuzzySet(medium2);
  correction->addFuzzySet(high2);
  fuzzy->addFuzzyOutput(correction);

  // Building FuzzyRule 1
  FuzzyRuleAntecedent *fr1 = new FuzzyRuleAntecedent();
  fr1->joinWithAND(large, high);
 
  FuzzyRuleConsequent *o1 = new FuzzyRuleConsequent();
  o1->addOutput(high2);
  
  FuzzyRule *fuzzyRule1 = new FuzzyRule(1, fr1, o1);
  fuzzy->addFuzzyRule(fuzzyRule1);

  
  // Building FuzzyRule 2
  FuzzyRuleAntecedent *fr2 = new FuzzyRuleAntecedent();
  fr2 ->joinWithAND(large, medium);
 
  FuzzyRuleConsequent *o2 = new FuzzyRuleConsequent();
  o2->addOutput(medium2);
  
  FuzzyRule *fuzzyRule2 = new FuzzyRule(2, fr2, o2);
  fuzzy->addFuzzyRule(fuzzyRule2);

  // Building FuzzyRule 3
  FuzzyRuleAntecedent *fr3 = new FuzzyRuleAntecedent();
  fr3 ->joinWithAND(large, low);
 
  FuzzyRuleConsequent *o3 = new FuzzyRuleConsequent();
  o3->addOutput(medium2);
  
  FuzzyRule *fuzzyRule3 = new FuzzyRule(3, fr3, o3);
  fuzzy->addFuzzyRule(fuzzyRule3);

  
  // Building FuzzyRule 4
  FuzzyRuleAntecedent *fr4 = new FuzzyRuleAntecedent();
  fr4->joinWithAND(moderate, high);
 
  FuzzyRuleConsequent *o4 = new FuzzyRuleConsequent();
  o4->addOutput(medium2);
  
  FuzzyRule *fuzzyRule4 = new FuzzyRule(4, fr4, o4);
  fuzzy->addFuzzyRule(fuzzyRule4);

    // Building FuzzyRule 5
  FuzzyRuleAntecedent *fr5 = new FuzzyRuleAntecedent();
  fr5 ->joinWithAND(moderate, medium);
 
  FuzzyRuleConsequent *o5 = new FuzzyRuleConsequent();
  o5->addOutput(medium2);
  
  FuzzyRule *fuzzyRule5 = new FuzzyRule(5, fr5, o5);
  fuzzy->addFuzzyRule(fuzzyRule5);

  // Building FuzzyRule 6
  FuzzyRuleAntecedent *fr6 = new FuzzyRuleAntecedent();
  fr6 ->joinWithAND(moderate, low);
 
  FuzzyRuleConsequent *o6 = new FuzzyRuleConsequent();
  o6->addOutput(medium2);
  
  FuzzyRule *fuzzyRule6 = new FuzzyRule(6, fr6, o6);
  fuzzy->addFuzzyRule(fuzzyRule6);

  
  // Building FuzzyRule 7
  FuzzyRuleAntecedent *fr7 = new FuzzyRuleAntecedent();
  fr7 ->joinWithAND(small, high);
 
  FuzzyRuleConsequent *o7 = new FuzzyRuleConsequent();
  o7->addOutput(medium2);
  
  FuzzyRule *fuzzyRule7 = new FuzzyRule(7, fr7, o7);
  fuzzy->addFuzzyRule(fuzzyRule7);

  // Building FuzzyRule 8
  FuzzyRuleAntecedent *fr8 = new FuzzyRuleAntecedent();
  fr8 ->joinWithAND(small, medium);
 
  FuzzyRuleConsequent *o8 = new FuzzyRuleConsequent();
  o8->addOutput(low2);
  
  FuzzyRule *fuzzyRule8 = new FuzzyRule(8, fr8, o8);
  fuzzy->addFuzzyRule(fuzzyRule8);

  // Building FuzzyRule 9
  FuzzyRuleAntecedent *fr9 = new FuzzyRuleAntecedent();
  fr9 ->joinWithAND(small, low);
 
  FuzzyRuleConsequent *o9 = new FuzzyRuleConsequent();
  o9->addOutput(low2);
  
  FuzzyRule *fuzzyRule9 = new FuzzyRule(9, fr9, o9);
  fuzzy->addFuzzyRule(fuzzyRule9);
  
}
