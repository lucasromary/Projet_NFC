/**
 *
 * HX711 library for Arduino - example file
 * https://github.com/bogde/HX711
 *
 * MIT License
 * (c) 2018 Bogdan Necula
 *
**/
#include "HX711.h"
#include <Arduino.h>
#include <avr/power.h>
#include <EEPROM.h> 
#include "LowPower.h"
#include <Wire.h>

int E2_0 = 0x53;
int E2_1 = 0x57;

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 8;
const int LOADCELL_SCK_PIN = 7;
float tare_weight = 0;
bool runOnce = true;
float average_reading;
float res;
int tare_address = 0;
float f = 0.00;

struct MyObject{
  long production_date;
  int expiration_date;
  char serial_number[7];
};

MyObject specs = {
  110121,
  1221,
  "AXBT56"
};
  

HX711 scale;

void scan_i2c(){
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

void write(uint8_t dev_addr, uint8_t reg_addr_msb, uint8_t reg_addr_lsb, uint8_t *data, uint8_t len) {
  Wire.beginTransmission(dev_addr);
  Wire.write(reg_addr_msb);
  Wire.write(reg_addr_lsb);
  for (int i=0; i < len; i++) {
    if(Wire.write(data[i])){
      Serial.println("OK");
    }
  }
  Wire.endTransmission(true);
  delay(100);  // TODO: polling
}

void setup() {
  //////////////////////////////// Optimize energy ////////////////////////////////
  /*
  for (byte i = 0; i <= A5; i++){
    pinMode (i, OUTPUT);    // changed as per below
    digitalWrite (i, LOW);  //     ditto
  }
  */
  
  //ADCSRA = 0;   // disable ADC
  //power_all_disable ();   // turn off all modules
  //power_usart0_enable(); // Serial (USART)
  //power_timer0_enable(); // Timer 0
  //power_timer1_enable(); // Timer 1
  power_twi_enable(); // TWI (I2C)
  
  /*
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  */
  //////////////////////////////// Starting HX711 ////////////////////////////////

  Serial.begin(9600);
  Wire.begin();
  Wire.setClock(100000);
  delay(100);
  scan_i2c();
  Serial.println("Initializing the scale");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  pinMode(6,INPUT_PULLUP);
  delay(1000);
  //int pinTare = digitalRead(6);
  /*
  float test = 0.24;
  int int_average_reading = int(test);
  Serial.println(int_average_reading);
  */
  //////////////////////////////// Verifying EEPROM ////////////////////////////////
  
  EEPROM.get(tare_address,f);
  if(f==0){
    scale.power_down();             // put the ADC in sleep mode and wake up for better readings
    delay(200);
    scale.power_up();

    Serial.println("No tare in EEPROM");
    Serial.print("tare weight value: \t\t");
    tare_weight = scale.read_average(20);
    Serial.println(tare_weight); 
    EEPROM.put(0, tare_weight);
    Serial.println("Written float data type in EEPROM!");
    EEPROM.put(sizeof(float)+sizeof(float), specs);
    Serial.println("Written specs in EEPROM!");
  }
  else{

    Serial.print("Value in EEPROM = ");
    Serial.println(f);
    tare_weight = f;
    EEPROM.get(sizeof(float), res);
    Serial.print( "resultat: " );
    Serial.println(res);
    EEPROM.get(sizeof(float)+sizeof(float), specs );
    
    Serial.println( "Read custom object from EEPROM: " );
    Serial.print( "Production date (JJMMAA): " );
    Serial.println( specs.production_date );
    Serial.print( "expiration date (MMAA): " );
    Serial.println( specs.expiration_date );
    Serial.print( "Serial number: " );
    Serial.println( specs.serial_number );

    
  }
  scale.set_scale(2280.f);       
  Serial.println("Readings:");
}

void loop() {

  //////////////////////////////// Read one weight data ////////////////////////////////
if(runOnce){
  scale.power_down();             // put the ADC in sleep mode and wake up for better readings
  delay(200);         
  scale.power_up();
  
  average_reading = ((scale.read_average(30)-tare_weight)/2280)/10;
  int int_average = int(average_reading);
  Serial.print("average:\t");
  Serial.print(average_reading);
  Serial.print("average int:\t");
  Serial.print(int_average);
  Serial.println(" kg ");
  runOnce = false;

  // write in ATMEGA EEPROM for backup.
  EEPROM.put(sizeof(float), average_reading);
  
  // Write in NFC EEPROM.
  // Write 95%, we can write "average_reading" instead.
  uint8_t buf[] = {int_average, int_average};
  write(E2_0, 0x00, 0X09, buf, 1 );

  // Sleep
  scale.power_down();
  delay(2000);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
}


}
