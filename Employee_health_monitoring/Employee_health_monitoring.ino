
#include <LiquidCrystal_I2C.h>
#define I2C_ADDR     0x3F 
LiquidCrystal_I2C lcd(I2C_ADDR,2,1,0,4,5,6,7,3,POSITIVE);
#include <SoftwareSerial.h>
float pulse = 0;
float temp = 0;
float resp = 0;
float glue = 0;
float oxy = 0;
String apiKey = "NZKW5SLQSPXCW9IE"; 

int pulsePin = A0; 
int blinkPin = 7 ; 
int fadePin = 13; 
int fadeRate = 0; 
unsigned long prevRespTime = 0;
unsigned long currRespTime = 0;
respState currentRespState;
int movingAvgRespRate = 0;
volatile int BPM; 
volatile int Signal; 
volatile int IBI = 600; 
volatile boolean Pulse = false; 
volatile boolean QS = false; 
static boolean serialVisual = true;
volatile int rate[10]; 
volatile unsigned long sampleCounter = 0; 
volatile unsigned long lastBeatTime = 0; 
volatile int P = 512; 
volatile int T = 512; 
volatile int thresh = 525; 
volatile int amp = 100; 
volatile boolean firstBeat = true; 
volatile boolean secondBeat = false; 
 
void setup()
{
lcd.begin(16, 2);
pinMode(blinkPin,OUTPUT); 
pinMode(fadePin,OUTPUT); 
Serial.begin(115200); 
interruptSetup(); 
lcd.clear();
lcd.setCursor(0,0);
lcd.print("Employee Health");
lcd.setCursor(0,1);
lcd.print(" Monitoring ");
delay(4000);
lcd.clear();
lcd.setCursor(0,0);
lcd.print("Initializing....");
delay(5000);
lcd.clear();
lcd.setCursor(0,0);
lcd.print("Getting Data....");
Serial.begin(115200);
Serial.println("AT");
delay(1000);
Serial.println("AT+GMR");
delay(1000);
Serial.println("AT+CWMODE=3");
delay(1000);
Serial.println("AT+RST");
delay(5000);
Serial.println("AT+CIPMUX=1");
delay(1000);
 
String cmd="AT+CWJAP=\"Sowmi\",\"9486578255\"";
Serial.println(cmd);
delay(1000);
Serial.println("AT+CIFSR");
delay(1000);
}
void loop()
{
serialOutput();
if (QS == true)
{
fadeRate = 255; 
serialOutputWhenBeatHappens();
QS = false;
}
ledFadeToBeat();
delay(20);
read_temp();
read_res();
read_glu();
esp_8266();
}
void ledFadeToBeat()
{
fadeRate -= 15;
fadeRate = constrain(fadeRate,0,255);
analogWrite(fadePin,fadeRate);
}
void interruptSetup()
{
TCCR2A = 0x02;
TCCR2B = 0x06;
OCR2A = 0X7C;
TIMSK2 = 0x02;
sei();
}
void serialOutput()
{ 
if (serialVisual == true)
{
arduinoSerialMonitorVisual('-', Signal);
}
else
{
sendDataToSerial('S', Signal);
}
}
void serialOutputWhenBeatHappens()
{
if (serialVisual == true)
{
Serial.print("*** Heart-Beat Happened *** ");
Serial.print("BPM: ");
Serial.println(BPM);
}
else
{
sendDataToSerial('B',BPM);
sendDataToSerial('Q',IBI);
}
}
void arduinoSerialMonitorVisual(char symbol, int data )
{
const int sensorMin = 0;
const int sensorMax = 1024;
int sensorReading = data;
int range = map(sensorReading, sensorMin, sensorMax, 0, 11);
switch (range)
{
case 0:
Serial.println("");
break;
case 1:
Serial.println("---");
break;
case 2:
Serial.println("------");
break;
case 3:
Serial.println("---------");
break;
case 4:
Serial.println("------------");
break;
case 5:
Serial.println("--------------|-");
break;
case 6:
Serial.println("--------------|---");
break;
case 7:
Serial.println("--------------|-------");
break;
case 8:
Serial.println("--------------|----------");
break;
case 9:
Serial.println("--------------|----------------");
break;
case 10:
Serial.println("--------------|-------------------");
break;
case 11:
Serial.println("--------------|-----------------------");
break;
}
}
 
void sendDataToSerial(char symbol, int data )
{
Serial.print(symbol);
Serial.println(data);
}
ISR(TIMER2_COMPA_vect)
{
cli();
Signal = analogRead(pulsePin);
sampleCounter += 2;
int N = sampleCounter - lastBeatTime;
 
if(Signal < thresh && N > (IBI/5)*3)
{
if (Signal < T) 
{
T = Signal;
}
}
if(Signal > thresh && Signal > P)
{ 
P = Signal; // P is the peak
}
if (N > 250)
{ 
if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) )
{
Pulse = true; 
digitalWrite(blinkPin,HIGH); 
IBI = sampleCounter - lastBeatTime; 
lastBeatTime = sampleCounter;
 
if(secondBeat)
{ 
secondBeat = false; 
for(int i=0; i<=9; i++) 
{
rate[i] = IBI;
}
}
if(firstBeat)
{
firstBeat = false;
secondBeat = true;
sei(); 
return; 
}

word runningTotal = 0; 
for(int i=0; i<=8; i++)
{ 
rate[i] = rate[i+1]; 
runningTotal += rate[i]; 
}
rate[9] = IBI; 
runningTotal += rate[9]; 
runningTotal /= 10; 
BPM = 60000/runningTotal; 
QS = true; 
pulse = BPM;
}
}
if (Signal < thresh && Pulse == true)
{ 
digitalWrite(blinkPin,LOW); 
Pulse = false; 
amp = P - T; 
thresh = amp/2 + T; 
P = thresh; 
T = thresh;
}
if (N > 2500)
{ 
thresh = 512; 
P = 512; 
T = 512; 
lastBeatTime = sampleCounter; 
firstBeat = true; 
secondBeat = false; 
}
sei(); 
}
void esp_8266()
{
String cmd = "AT+CIPSTART=4,\"TCP\",\"";
cmd += "184.106.153.149"; // api.thingspeak.com
cmd += "\",80";
Serial.println(cmd);
Serial.println(cmd);
if(Serial.find("Error"))
{
Serial.println("AT+CIPSTART error");
return;
}
String getStr = "GET /update?api_key=";
getStr += apiKey;
getStr +="&field1=";
getStr +=String(temp);
getStr +="&field2=";
getStr +=String(pulse);
getStr +="&field3=";
getStr +=String(resp);
getStr +="&field4=";
getStr +=String(glue);
getStr +="&field5=";
getStr +=String(oxy);
getStr += "\r\n\r\n";
// send data length
cmd = "AT+CIPSEND=4,";
cmd += String(getStr.length());
Serial.println(cmd);
Serial.println(cmd);
delay(1000);
Serial.print(getStr);
Serial.println(getStr);
delay(3000);
}
void read_glu()
{
  int glu_val = analogRead(A3);
  float gl = (glu_val/1024.0);
  glue=gl/7;
  Serial.println(glue);
  oxy=gl/3;
  Serial.println(oxy);
  
}
void read_temp()
{
int temp_val = analogRead(A1);
float mv = (temp_val/1024.0)*5000;
float cel = mv/10;
temp = (cel*9)/5 + 32;
Serial.print("Temperature:");
Serial.println(temp);
lcd.clear();
lcd.setCursor(0,0);
lcd.print("BPM :");
lcd.setCursor(7,0);
lcd.print(BPM);
lcd.setCursor(0,1);
lcd.print("Temp.:");
lcd.setCursor(7,1);
lcd.print(temp);
lcd.setCursor(13,1);
lcd.print("F");
}
void read_res()
{
 int sensorValue = analogRead(A0);

    if ((sensorValue > 300) && (currentRespState == Exhaling))
    {
        currentRespState = Inhaling;
        prevRespTime = currRespTime;  // save time from last cycle
        currRespTime = millis();

        float period_respiration = currRespTime - prevRespTime;
        float resp = 60000 / period_respiration;
        Serial.print("Inhaling /tRespirations per minute: ");
        Serial.println(resp);
    }
    else if ((currentRespState == Inhaling) && (sensorValue < 200))
    {
        currentRespState = Exhaling;
        Serial.println("Exhaling");
    }
} 
