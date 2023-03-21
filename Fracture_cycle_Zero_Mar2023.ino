//code to run fracture probe once positioned at sediment surface

#include <Wire.h>
#include <Adafruit_ADS1X15.h>

//FOR SD CARD SHIELD
#include <SPI.h>
#include <SD.h>


// how many milliseconds between grabbing data and logging it. 
#define LOG_INTERVAL  10 // mills between entries (reduce to take more/faster data)

// how many milliseconds before writing the logged data permanently to disk
// set it to the LOG_INTERVAL to write each time (safest)
// set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to
// the last 10 reads if power is lost but it uses less power and is much faster!
#define SYNC_INTERVAL 1000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()

#define ECHO_TO_SERIAL   1 // echo data to serial port
#define WAIT_TO_START    0 // Wait for serial input in setup()


// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;  //THIS NEEDS TO BE JUMPED FROM THE NORMAL PIN 10 ON THE SD CARD SHIELD

// the logging file
File logfile;
 File logfileP;


int val = 0;             //####################################################################TESTING ARDUINO ARDUINO COMMS
void error(char *str)
{
  SerialUSB.print("error: ");
  SerialUSB.println(str);
  while(1);
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@  Capture Serial data from TinyG
const byte numChars = 43;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing

      // variables to hold the parsed data
char messageFromPC[numChars] = {0};
int integerFromPC = 0;
float floatFromPC = 0.0;
float Xpos = 0.000;
float Ypos = 0.000;
float Zpos = 0.000;
float Apos = 0.000;
boolean newData = false;
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@  END Capture Serial Data



//LOAD CELL
const int LoadCellIn = A4;  // Use A4 with the load cell/Zero
int sensorValueLoad = 0;        // value read from the load cell
float Load = 0;        // value output to the PWM (analog out)  


Adafruit_ADS1015 ads;     /* Use this for the 12-bit version */


//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&BUTTONS!!&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
// buttons wired in for troubleshooting, to stop and resume if there are issues

const int buttonD5 = 5 ; //button with pin, STOP button
int buttonStateD5 = 0 ;

const int buttonD6 = 6 ; //button with pin, RESUME button
int buttonStateD6 = 0 ;

const int buttonD7 = 7 ; //button with pin, REPOSITION button (don't use)
int buttonStateD7 = 0 ;

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

//count number of runs to keep track of depth
int noRuns = 0;


void setup()
{  
  SerialUSB.begin (115200);
  Serial1.begin (115200);  


  pinMode(buttonD5, INPUT);
  pinMode(buttonD6, INPUT);
  pinMode(buttonD7, INPUT);

  delay (500); //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Setting TinyG position output

  Serial1.println("?");

  Serial1.println("{\"sr\":{\"posx\":t, \"posy\":t,\"posz\":t,\"posa\":t}}");
  Serial1.println("$qv=0");  // setting the Queue Report Verboisity to zero (disable)to prevent qr:xx from showing up in the serial feed
  Serial1.println("$sv=2"); //setting the status report to "verbose" so all postions show all the time


  Serial1.println("?");
  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@  END Setting TinyG position output

// The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
//   ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
   //ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
   //ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  
//  ads.begin();

 // initialize the SD card
  SerialUSB.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
 
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");
 
  // create a new file - files will need to be renamed after copying from the SD card to computer
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE);
      break;  // leave the loop!
    }
  }



 
  if (! logfile) {
    error("couldnt create file");
  }
 
  SerialUSB.print("Logging to: ");
  SerialUSB.println(filename);


  logfile.println("Millis,Load(g), RawLoad, X,Y,noRuns");  //CHANGE THESE HEADERS TO WHATEVER YOU WANT 
  // note we are calling X up and down and Y rotating (through a gear connected to the stepper motor)
  delay (50);
  }
 

void loop () {
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&BUTTONS

//###########################################################################
buttonStateD5 = digitalRead(buttonD5); //this is the stop button

if(buttonStateD5 == LOW) {
    Serial1.write("!");
  Serial1.write('\r');
   delay(50);
}


//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
buttonStateD6 = digitalRead(buttonD6); //this is the resume button

if(buttonStateD6 == LOW) {
  Serial1.write("~");
  Serial1.write('\r');
   delay(50);
}


//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
buttonStateD7 = digitalRead(buttonD7); //this returns to original 0 like limit switch (don't use)

if(buttonStateD7 == LOW) {
  Serial1.write("g28.2 z0");
  Serial1.write('\r');
   delay(50);
}



//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&


  
   analogReadResolution(12); //Changing the resolution (from 8bit to 12bit) of the analog read


//$$$$$$$$$$$$$$$$$$LOAD CELL%%%%%%%%%%%%%%%%%%%%%%%%

sensorValueLoad = analogRead(LoadCellIn); //raw data from force sensor

Load=(sensorValueLoad * 0.009);  // When you calibrate the load cell, change the multiplier here!!!


 uint32_t m = millis();
 
  while (Serial1.available()>0){  //###############

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
   recvWithStartEndMarkers();
    if (newData == true) {
        strcpy(tempChars, receivedChars);
            // this temporary copy is necessary to protect the original data
            //   because strtok() used in parseData() replaces the commas with \0
        parseData();
      //  showParsedData();
        newData = false;
    
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

    }
 
  }

  // read from port 0, send to port 1:
  while (SerialUSB.available()>0) {
    char inByte = SerialUSB.read();

switch (inByte){


  case 'q'://@@@@@@@@@@@@@@@@@@@@@@@@@@ BEGIN Q FULL cycle at a depth

noRuns = noRuns + 1; //increase number of runs each time hit q

//send commands in gcode

Serial1.print("g1x10y10f50"); // rotate in and translate 10mm at 50 mm/min
  Serial1.print('\r');


  
  Serial1.print("g4p.5"); // pause
  Serial1.print('\r');
  Serial1.print("g1x0f100");  // PULL up, no rotation at 100 mm/min
  Serial1.print('\r');

  Serial1.print("g4p.5"); // pause
  Serial1.print('\r');

  
  Serial1.print("g28.3y0"); // rezero y
  Serial1.print('\r');

  Serial1.print("g1x10y10f50");  //friction test START
  Serial1.print('\r');

  

  
  Serial1.print("g4p.5");  //pause
  Serial1.print('\r');

  Serial1.print("g1x0f100"); //friction test PULL
  Serial1.print('\r');
  Serial1.print("g4p.5");
  Serial1.print('\r');

  
  Serial1.print("g28.3y0"); // rezero y
  Serial1.print('\r');


  Serial1.print("g1x10y10f50"); //rotate back in to test depth
  Serial1.print('\r');

  
  Serial1.print("g28.3x0y0"); //rezero x and y before next cycle
  Serial1.print('\r');

  
  break;
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@  END Q
}  
Serial1.write(inByte);
}
  
     logfile.print(m); //millis
    logfile.print(",");


    logfile.print(Load,4); //load from calibration
    logfile.print(",");
    logfile.print (sensorValueLoad); // raw load
    logfile.print(",");

  
    logfile.print (Xpos); //x position in mm
    logfile.print(",");
    logfile.print (Ypos); // y position in mm
    logfile.print(",");

    logfile.print(noRuns); // depth increment
    logfile.println(",");

 
#if ECHO_TO_SERIAL
  SerialUSB.print(m);         // milliseconds since start
  SerialUSB.print(",    "); 
  SerialUSB.print (Load,4);
  SerialUSB.print(",   ");
  SerialUSB.print(sensorValueLoad);
  SerialUSB.print(",   ");
  SerialUSB.print (Xpos);
  SerialUSB.print(",   ");
  SerialUSB.print (Ypos);
  SerialUSB.print(",   ");

  SerialUSB.print(noRuns);
  SerialUSB.println(",  ");
  
  
#endif
  
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
  logfile.flush();

}
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@END VOID LOOP
void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = ':';
    char endMarker = '\n';
    char rc;

  //  while (Serial1.available()>0){//0 && newData == false) {
        rc = Serial1.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    //}
}

//============

void parseData() {      // split the data into its parts

    char * strtokIndx; // this is used by strtok() as an index

   // strtokIndx = strtok(tempChars,":");      // get the first part - the string
    strtokIndx = strtok(tempChars, ":"); // this continues where the previous call left off
    Xpos = atof(strtokIndx);     // convert this part to an integer
    //strcpy(messageFromPC, strtokIndx); // copy it to messageFromPC
 
    strtokIndx = strtok(NULL, ":"); // this continues where the previous call left off
    Ypos = atof(strtokIndx);     // convert this part to an integer

   strtokIndx = strtok(NULL, ":");
    Zpos = atof(strtokIndx);     // convert this part to a float

    
   strtokIndx = strtok(NULL, ":");
    Apos = atof(strtokIndx);     // convert this part to a float

}

//============

void showParsedData() {
//    Serial.print("Xpos ");
//    Serial.print(Xpos);
//    Serial.print("    Ypos ");
//    Serial.print(Ypos);
//    Serial.print("    Zpos ");
//    Serial.print(Zpos);
//    Serial.print("    Apos ");
//    Serial.println(Apos);
}
