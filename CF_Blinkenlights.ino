/*
  LED Meter - sweeps a row of LEDs
 
Uses a series of pins to illuminate one LED like a VU meter.
Allows two LEDs to blink to indicate intermediate values.
 
 The circuit:
 * Line of LEDs connected to a 560R resistor (approx)
 * LED other legs connected to a series of pins as defined below
 
 created 19th Feb 2015
 (C) Vik Olliver
 
 This code is licenced under the GPLv3
 
 */

// These constants won't change.  They're used to give names
// to the pins used for the probe:
const int alphaOut = A0;
const int alphaAnalogue = A1;
const int betaAnalogue = A2;
const int betaOut = A3;


// These bits you change to configure the sensor
const int baseValue=340;  // Value of weak 0.1CF solution
const int cfTable[]={
340,0,
595,5,
749,10,
840,15,
860,20,
923,34};
#define CF_TABLE_ENTRIES  6

// These bits you change to configure the LEDs. If the
// LIGHT_RANGE is a multiple of LED_TABLE_ENTRIES things look
// a lot smoother.
const int pinTable[]={2,3,4,5,6,7,8,9};
#define LED_TABLE_ENTRIES  8  // Length of above list
#define LIGHT_RANGE  32    // Maximum CF value we care about
#define FLASH_TIME    40   // Shortest flash interval

int lastPinLit=0;  // Make sure we extinguish the last pin!
int lastSwipeLed=0;  // Used to create a "scanning" effect

int alphaSensor;
int betaSensor;

// Stop the outputs from doing anything electrolytic.
void neutral() {
  pinMode(alphaAnalogue,INPUT);
  pinMode(betaAnalogue,INPUT);
  pinMode(alphaOut,OUTPUT);
  pinMode(betaOut,OUTPUT);
  digitalWrite(alphaOut,LOW);
  digitalWrite(betaOut,LOW);
}

void setup() {
  int i;
  neutral();
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  for (i=0; i<LED_TABLE_ENTRIES; i++) pinMode(pinTable[i],OUTPUT);
}

// Use the table to average out the sensor data and calculate the corresponding CF.
// This may not be the best way, but it is the most legible!

int calcCf(int sens) {
  int i;
  int sensDiff;
  int sensDelta;
  int cfDiff;
  // Spot very low values
  if (sens <= cfTable[0]) return cfTable[1];
  // Spot very high values
  if (sens >= cfTable[(CF_TABLE_ENTRIES*2)-2]) return cfTable[(CF_TABLE_ENTRIES*2)-1];
  // OK, we're in range. Trot through the table until we overtake our sensor reading
  i=1;
  while (cfTable[(i*2)]<sens) i++;
  // Find the difference between the last 2 table entries;
  sensDiff=cfTable[(i*2)]-cfTable[(i*2)-2];
  // Find the difference between the last 2 CF readings;
  cfDiff=cfTable[(i*2)+1]-cfTable[(i*2)-1];
  // Now the amount our sensor passed the previous table entry by
  sensDelta=sens-cfTable[(i*2)-2];
  // Now we can work out a ratio,and calculate the corresponding CF
  return cfTable[(i*2)-1]+((cfDiff*sensDelta+1)/sensDiff);
}

void lightPin(int x) {
  digitalWrite(lastPinLit,LOW);
  // Only light it up if it is within range
  if ((x<LED_TABLE_ENTRIES) && (x>=0)) {
    digitalWrite(pinTable[x],HIGH);
    lastPinLit=pinTable[x];
  }
}

// Turn out the lights.
void blankPin() {
  digitalWrite(lastPinLit,LOW);
}

// Just track LEDs back and forth.
void swipeLed() {
  lightPin(lastSwipeLed);
  if (++lastSwipeLed == LED_TABLE_ENTRIES) lastSwipeLed=0;
  delay(FLASH_TIME);
}

// Here we range-limit and blink the LED pro-rata
void blinkPin(int x) {
  int litPin;
  int remainder;
  if (x<0) x=0;
  if (x>=LIGHT_RANGE) x=LIGHT_RANGE-1;
  // If we shouldn't fully light a pin, then don't light one!
  litPin=(LED_TABLE_ENTRIES*x)/LIGHT_RANGE;
  lightPin(litPin-1);
  // OK, basic LED lit up. Now work out the fractional part.
  remainder=x-((litPin*LIGHT_RANGE)/LED_TABLE_ENTRIES);
  // Leave existing LED lit for a while
  delay(FLASH_TIME*((LIGHT_RANGE/LED_TABLE_ENTRIES)-remainder));
  // If there was a remainder, we light up the next light
  if (remainder>0) {
    lightPin(litPin);
    delay(FLASH_TIME*remainder);
  }
  delay(250);
}

void loop() {
  int cf;
  
  // Reset for the analogue alpha pin
  pinMode(alphaAnalogue,INPUT);
  pinMode(betaAnalogue,OUTPUT);
  digitalWrite(alphaOut,LOW);
  digitalWrite(betaAnalogue,HIGH);
  digitalWrite(betaOut,HIGH);
  delay(2);  // Wait for the ADC to settle.
  alphaSensor = analogRead(alphaAnalogue);
  
  // Reset for the analogue beta pin
  pinMode(betaAnalogue,INPUT);
  pinMode(alphaAnalogue,OUTPUT);
  digitalWrite(betaOut,LOW);
  digitalWrite(alphaAnalogue,HIGH);
  digitalWrite(alphaOut,HIGH);
  delay(2);  // Wait for the ADC to settle.
  betaSensor = analogRead(betaAnalogue);

  // Now stop buggering around.
  neutral();

  // Calculate the CF from the average sensor value
  cf=calcCf((alphaSensor+betaSensor)/2);
  // If we have a reading, show it. Otherwise display "scanning"
  if (cf>0)
    blinkPin(cf);
  else
    swipeLed();

  // print the results to the serial monitor:
  Serial.print("Sensor = " );                       
  Serial.print((alphaSensor+betaSensor)/2);
  if (alphaSensor>baseValue) {
    Serial.print("\tCF = ");
    Serial.print(cf);
  }
  Serial.println("");  

}
