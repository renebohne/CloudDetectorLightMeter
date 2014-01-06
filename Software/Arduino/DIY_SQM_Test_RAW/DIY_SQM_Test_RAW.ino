/**
* This sketch reads three sensors:
*  DS18B20
*  MLX90614
*  TSL237
* It calculates the temperatures (DS18B20 and MLX90614) and the light level (TSL237) converted to magnitudes/arcSecond^2.
* The foundation of this sketch are here:
* DS18B20: http://playground.arduino.cc/Learning/OneWire
* MLX90614: http://bildr.org/2011/02/mlx90614-arduino/
* TSL237: http://stargazerslounge.com/topic/183600-arduino-sky-quality-meter-working/
*/

#include <Math.h>

//get it here: http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <OneWire.h>

//get it here: http://www.pjrc.com/teensy/td_libs_FreqMeasure.html
#include <FreqMeasure.h>

//get it here: http://jump.to/fleury
#include <i2cmaster.h>




const float A = 21.0;//edit this value for the Frequency to magnitudes/arcSecond2 calculation, depending on your sensor!!!

OneWire  ds(10);  // DS18B20 on Arduino pin 10
byte ds18b20_addr[8];



void setup(void) 
{
  Serial.begin(9600);

  init_DS18B20();
  init_MLX90614();
}

void init_DS18B20()
{
  Serial.println("Initializing DS18B20 sensor...");
  while( !ds.search(ds18b20_addr)) 
  {
    ds.reset_search();
    delay(250);
  }
}


void init_MLX90614()
{
  Serial.println("Initializing MLX90614 sensor...");
  i2c_init(); //Initialise the i2c bus
  //	PORTC = (1 << PORTC4) | (1 << PORTC5);//enable pullups if you use 5V sensors and don't have external pullups in the circuit
}


void loop(void) 
{
  float ds18b20_celsius = read_DS18B20();
  
  Serial.println();

  Serial.print("DS18B20  Temperature:\t");
  Serial.print(ds18b20_celsius);
  Serial.println("\tCelsius");

  float MLX90614_celsius = read_MLX90614();
  Serial.print("MLX90614 Temperature:\t");
  Serial.print(MLX90614_celsius);
  Serial.println("\tCelsius");  

  float delta_celsius = ds18b20_celsius - MLX90614_celsius;
  Serial.print("DELTA Temperature:\t");
  Serial.print(delta_celsius);
  Serial.println("\tCelsius");

  float TSL237_Msqm = read_TSL237();
  Serial.print("TSL237 Light level:\t");
  Serial.print(TSL237_Msqm);
  Serial.println("\tMag/As2");
  
  Serial.print("*");
  Serial.print(ds18b20_celsius);
  Serial.print(";");
  Serial.print(MLX90614_celsius);
  Serial.print(";");
  Serial.println(TSL237_Msqm);

  //delay(1000);
  delay(250);
}


float read_TSL237() 
{
  float Msqm = 0.0f;
  int val = 0;
  int reading = 1;
  double sum=0;
  int count=0;

  FreqMeasure.begin();	
  while(reading) 
  {
    if (FreqMeasure.available()) 
    {
      // average several reading together
      sum = sum + FreqMeasure.read();
      count +=1;

      if (count > 30) 
      {
        double frequency = F_CPU / (sum / count);
        sum = 0;
        count = 0;

        Msqm = A - 2.5*log10(frequency); //Frequency to magnitudes/arcSecond2 formula

        reading = 0;
        FreqMeasure.end();
      }
    }
  }
  return Msqm;
}




float read_DS18B20()
{
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];

  float celsius;

  if (OneWire::crc8(ds18b20_addr, 7) != ds18b20_addr[7]) 
  {
    Serial.println("CRC is not valid!");
    return -300.0f;
  }


  // the first ROM byte indicates which chip
  switch (ds18b20_addr[0]) 
  {
  case 0x10:
    type_s = 1;
    break;
  case 0x28:
    type_s = 0;
    break;
  case 0x22:
    type_s = 0;
    break;
  default:
    return -301.0f;
  } 

  ds.reset();
  ds.select(ds18b20_addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(ds18b20_addr);    
  ds.write(0xBE);         // Read Scratchpad

  //read 9 data bytes
  for ( i = 0; i < 9; i++) 
  {
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) 
  {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } 
  else 
  {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;

  return celsius;
}


float read_MLX90614()
{
  int dev = 0x5A<<1;
  int data_low = 0;
  int data_high = 0;
  int pec = 0;

  i2c_start_wait(dev+I2C_WRITE);
  i2c_write(0x07);

  // read
  i2c_rep_start(dev+I2C_READ);
  data_low = i2c_readAck(); //Read 1 byte and then send ack
  data_high = i2c_readAck(); //Read 1 byte and then send ack
  pec = i2c_readNak();
  i2c_stop();

  //This converts high and low bytes together and processes temperature, MSB is a error bit and is ignored for temps
  double tempFactor = 0.02; // 0.02 degrees per LSB (measurement resolution of the MLX90614)
  double tempData = 0x0000; // zero out the data
  int frac; // data past the decimal point

  // This masks off the error bit of the high byte, then moves it left 8 bits and adds the low byte.
  tempData = (double)(((data_high & 0x007F) << 8) + data_low);
  tempData = (tempData * tempFactor)-0.01;

  float celcius = tempData - 273.15;
  return celcius;
}


