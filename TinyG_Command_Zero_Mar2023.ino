
// code to enter gcode commands to position the probe before beginning fracture cycles
//define buttons (these are optional)
const int buttonD8 = 8 ; //button with pin
int buttonStateD8 = 0 ;

const int buttonD5 = 5 ; //button with pin
int buttonStateD5 = 0 ;

const int buttonD6 = 6 ; //button with pin
int buttonStateD6 = 0 ;

const int buttonD7 = 7 ; //button with pin
int buttonStateD7 = 0 ;


void setup() {
  // initialize both serial ports:
  SerialUSB.begin(115200);
  Serial1.begin(115200);

pinMode(buttonD8, INPUT);
pinMode(buttonD5, INPUT);
pinMode(buttonD6, INPUT);
pinMode(buttonD7, INPUT);


delay (500);
Serial1.println("?");

Serial1.println("{\"sr\":{\"posx\":t, \"posy\":t,\"posz\":t,\"posa\":t}}");

Serial1.println("$qv=0"); // setting the Queue Report Verboisity to zero (disable)to prevent qr:xx from showing up in the serial feed
Serial1.println("$sv=2"); //setting the status report to "verbose" so all postions show all the time

Serial1.println("$1"); //display setting for X axis
Serial1.println("$2"); //display settings for Y axis

Serial1.println("?");

}

void loop() {
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&BUTTONS

//###########################################################################
buttonStateD5 = digitalRead(buttonD5) ; // stop button

if(buttonStateD5 == LOW) {
    Serial1.write("!");
  Serial1.write('\r');
   delay(50);
}
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
buttonStateD6 = digitalRead(buttonD6) ; // resume button

if(buttonStateD6 == LOW) {
  Serial1.write("~");
  Serial1.write('\r');
   delay(50);
}
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
buttonStateD7 = digitalRead(buttonD7) ; // button to return to 0

if(buttonStateD7 == LOW) {
  Serial1.write("g28.2 z0");
  Serial1.write('\r');
   delay(50);
}
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//
//buttonStateD8 = digitalRead(buttonD8) ; // can add more buttons if wanted
//
//if(buttonStateD8 == LOW) {
//  Serial1.write("g28.2 z0");
//  Serial1.write('\r');
//   delay(50);
//}
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

  
  // read from port 1, send to port 0:
  if (Serial1.available()) {
    int inByte = Serial1.read();
    SerialUSB.write(inByte);
  }

  // read from port 0, send to port 1:
 while (SerialUSB.available()>0) {
int inByte = SerialUSB.read();
//Serial1.write(inByte);

switch(inByte){

// define commonly used commands as cases for easy manipulation
// otherwise type gcode commands in serial window
// need to zero x and y before starting the next code g28.3x0y0, check with '?'
  case 'p':
  Serial1.print("g28.2 z0");
  Serial1.print('\r');
  break;


  case 'q':
  Serial1.print("g1x10f50");
  Serial1.print('\r');
  Serial1.print("g1x0f50");
  Serial1.print('\r');

  break;
}
Serial1.write(inByte);

}
  
//  if (Serial.available()) {
//    int inByte = Serial.read();
//    Serial1.write(inByte);
//  }
}
