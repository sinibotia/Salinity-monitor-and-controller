//Programmer: Oliver Newbold
//Acknowledgements: This project is based on sample code from Atlas Scientific, SparkFun Example 4B (LCD Display), and the OneWire Temperature Sensor project by Konstantin Dimitrov(found on the Arduino website), as well as the salinity algorithm from:
//                  Fofonoff, N.P., Millard Jr, R.C., UNESCO (1983): Algorithms for computation of fundamental properties of seawater. UNESCO technical papers in marine science 44:1-55.
//Function: This code reads temperature and conductivity using a DS18B20 waterproof thermometer and an Atlas Scientific K 1.0 conductivity meter. These values are used to calculate
//          salinity; the salinity and temperature are then displayed on an LCD screen. An arduino outlet is hooked up to the board and when the salinity reaches above a certain threshhold, the outlet is turned on
//          (ideally in order to turn on a pump from a freshwater source, to maintain constant salinity)
//Input: Temperature and conductivity from environmental sensors
//Output: Salinity and temperature display on an LCD screen, on/off signal to an arduino outlet


#include <OneWire.h>                                  //This library is needed to read the temperature sensor
#include <DallasTemperature.h>                        //This library is also needed to read the temperature sensor
#include <SoftwareSerial.h>                           //we have to include the SoftwareSerial library, or else we can't use it
#include <LiquidCrystal.h>                            //This library allows output to the LCD display

#define rx 2                                          //define what pin rx is going to be
#define tx 3                                          //define what pin tx is going to be
#define ONE_WIRE_BUS 4                                //this pin will read the temperature from the DS18B20

OneWire oneWire(ONE_WIRE_BUS);
SoftwareSerial myserial(rx, tx);                      //define how the soft serial port is going to work
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);              // tell the RedBoard what pins are connected to the display
DallasTemperature sensors(&oneWire);

String inputstring = "";                              //a string to hold incoming data from the PC
String sensorstring = "";                             //a string to hold the data from the Atlas Scientific product
double tempC=0;                                       //a double to hold temperature in degrees C
int count=0;                                         //this variable will hold a time count from the last time the outlet was turned on to keep the pump from turning on too often and allow the salinity to stabilize
boolean input_string_complete = false;                //have we received all the data from the PC
boolean sensor_string_complete = false;               //have we received all the data from the conductivity meter




void setup(void) {                                        //set up the hardware
  pinMode(7, OUTPUT);                                 //sets 7 as the output pin for the outlet
  Serial.begin(9600);                                 //set baud rate for the hardware serial port_0 to 9600
  myserial.begin(9600);                               //set baud rate for the software serial port to 9600
  inputstring.reserve(10);                            //set aside some bytes for receiving data from the PC
  sensorstring.reserve(30);                           //set aside some bytes for receiving data from Atlas Scientific product
  sensors.begin();                                    //start the temperature sensor
  lcd.begin(16, 2);                                   //tell the lcd library that we are using a display that is 16 characters wide and 2 characters high
  lcd.clear();                                        //clear the display 
}


void serialEvent() {                                  //if the hardware serial port_0 receives a char
  inputstring = Serial.readStringUntil(13);           //read the string until we see a <CR>
  input_string_complete = true;                       //set the flag used to tell if we have received a completed string from the PC
}


void loop() {                                         //here we go...
  if (input_string_complete == true) {                //if a string from the PC has been received in its entirety
    myserial.print(inputstring);                      //send that string to the Atlas Scientific product
    myserial.print('\r');                             //add a <CR> to the end of the string
    inputstring = "";                                 //clear the string
    input_string_complete = false;                    //reset the flag used to tell if we have received a completed string from the PC
  }

  if (myserial.available() > 0) {                     //if we see that the EC probe has sent a character
    char inchar = (char)myserial.read();              //get the char we just received
    sensorstring += inchar;                           //add the char to the var called sensorstring
    if (inchar == '\r') {                             //if the incoming character is a <CR>
      sensor_string_complete = true;                  //set the flag
    }
  }


  if (sensor_string_complete == true) {               //if a string from the EC probe has been received in its entirety
    if (isdigit(sensorstring[0]) == false) {          //if the first character in the string is a digit
      Serial.println(sensorstring);                   //send that string to the PC's serial monitor
    }
    else                                              //if the first character in the string is NOT a digit
    {
      print_data();                                //then call this function 
    }
    sensorstring = "";                                //clear the string
    sensor_string_complete = false;                   //reset the flag used to tell if we have received a completed string from the EC probe
  }
}



void print_data(void) {                            //this function will get EC and temperature data, print them to the serial monitor, then calculate salinity and print salinity and temperature to the LCD. In addition, it will tell the pump to turn on if the salinity is too high
  char sensorstring_array[30];                        //we make a char array
  char *EC;                                           //char pointer used in string parsing
  char *TDS;                                          //char pointer used in string parsing
  char *SAL;                                          //char pointer used in string parsing
  char *GRAV;                                         //char pointer used in string parsing
  float f_ec;                                         //used to hold a floating point number that is the EC
  
  sensorstring.toCharArray(sensorstring_array, 30);   //convert the string to a char array 
  EC = strtok(sensorstring_array, ",");               //let's pars the array at each comma
  TDS = strtok(NULL, ",");                            //let's pars the array at each comma
  SAL = strtok(NULL, ",");                            //let's pars the array at each comma
  GRAV = strtok(NULL, ",");                           //let's pars the array at each comma

  Serial.print("EC:");                                //we now print each value we parsed separately
  Serial.println(EC);                                 //print the conductivity to the command window
  sensors.requestTemperatures();                      //request temperature data from the temperature sensor
  tempC=(sensors.getTempCByIndex(0)+1);                   //assign the temperature in degrees C to a variable and adjust it by +1(calibration)
  Serial.println(tempC);         //print the temperature to the command line
  
 f_ec= atof(EC);                                      //this line converts the conductivity char to a float
 
 //the following equations are transcribed from "Algorithms for computation of fundamental properties of seawater."(see citation in the header).
 double conductivity=((f_ec/1000)/42.914);
 double XT=tempC-15.0;
 double RT35=(((1.0031E-9 * tempC - 6.9698E-7) * tempC + 1.104259E-4) * tempC + 2.00564E-2) * tempC + 0.6766097;
 double B = (4.464E-4 * tempC + 3.426E-2) * tempC + 1.0;
 double A = -3.107E-3 * tempC + 0.4215;
 double C=0.00020962563;
 double RT = conductivity / (RT35 * (1.0 + C / (B + A * conductivity)));
 RT = sqrt(abs(RT));
 double salinity=((((2.7081 * RT - 7.0261) * RT + 14.0941) * RT + 25.3851) * RT - 0.1692) * RT + 0.0080 + (XT / (1.0 + 0.0162 * XT)) * (((((-0.0144 * RT + 0.0636) * RT - 0.0375) * RT - 0.0066) * RT - 0.0056) * RT + 0.0005);
 Serial.println(salinity);                            //print the salinity to the command line

 lcd.clear();                                         //clear the LCD display
 lcd.setCursor(0,0);                                  //set the cursor to the upper left position
 lcd.print("Degrees C: ");                            //Print a label for the data
 lcd.print(tempC);               //Print the temperature

 lcd.setCursor(0,1);                                  //set the cursor to the lower left position
 lcd.print("Salinity: ");                             //Print a label for the data
 lcd.print(salinity);                                 //Print the salinity

 if (salinity>33.5 && count>60){                      //if the salinity is higher than 33.5(a mid range value for the proper salinity of a coral reef aquarium)
  digitalWrite(7, HIGH);                              //turn on the outlet
  delay(4000);                                        //wait for 4 seconds
  count=0;                                            //reset the counter for the pump
  digitalWrite(7, LOW);                               //turn off the pump
  }
  count=count+1;                                      //add one to the pump counter
  Serial.println(count);                              //print the pump counter to the command window
}
