//
// Aquarium Automation Demo
// DJZ September 2017
// Originally written for the Milwaukee Makerfair
//
// The basic idea is that a 10 gallon aquarium is divided into two chambers.
// Each side has a 120v pump controlled by the arduino through SSR's.
// The right side has a capacitive sensor pads at about 1.5 inch increments,
// forming a 6 level water height indicator.  The left side has a single float
// sensor measureing only "full".
// The software moves the water back and forth, demonstrating the sensors, 120v
// controll and some logic.
// People will be amazed.
//
// *****************
// The following text (until the ***) is from the Arduino Example provided by Adafruit:
// This is a library for the MPR121 12-channel Capacitive touch sensor
//
// Designed specifically to work with the MPR121 Breakout in the Adafruit shop 
//  ----> https://www.adafruit.com/products/
//
// These sensors use I2C communicate, at least 2 pins are required 
// to interface.
//
// Written by Limor Fried/Ladyada for Adafruit Industries.  
// BSD license, all text above must be included in any redistribution
//*********************
//
// v0.5: Initial version demonstrating state machine and capacitive sensor
// v0.6: 
//   pending:
//   o switch direction of the state machine so that the float sensor can
//     be on the left
//   o integrate the new float sensor (probably just change the sense of the logic)
//   o build an initialization routine with manual L, R buttons to get
//     the water to the right side before establishing the baseline
//   o code some time outs (using timers) between state changes
//     (in case of sensor failure)
//   o add some MQTT messages via wifi (after adding wifi)
//

#include <Wire.h>
#include "Adafruit_MPR121.h"

// *** Hardware Configuration ***

// Analog i/o
//#define UNUSED  A0
//#define UNUSED  A1
//#define UNUSED  A2
//#define UNUSED  A3
#define I2C_SDA A4  // i2c for Capacitive Sensor, default, notation only
#define I2C_SCL A5  // i2c for Capacitive Sensor, default, notation only

// Digital i/o
#define HW_SERIAL_OUT 0
#define HW_SERIAL_IN  1
#define PUMP_L        2  // left pump SSR
#define PUMP_R        3  // right pump SSR
#define FLOAT_TOP     5  // float sensor on the left side top
#define BUT_PUMP_L    6  // left pump manual control button
#define BUT_PUMP_R    7  // right pump manual control button
#define BUT_START     8  // start button

// abstract the pump i/o sense
#define PUMP_ON       HIGH
#define PUMP_OFF      LOW

// abstract the float sensor i/o sense
// float switches are normally closed; will use arduino pull-up
#define FLOAT_TRIP    HIGH
#define FLOAT_DRY     LOW

// abstract the button i/o sense
#define BUT_PRESSED   LOW
#define BUT_RELEASED  HIGH

// *** Logical Configuration ***

// Capacitive Sensor Board channel assignments
// MPR121 has 12 channels designated 0 - 11
#define CAP_START_CH  6    // Lowest capacitive channel used
#define CAP_END_CH    11   // Highest capacitive channel used
#define CAP_NUM_CH    12   // Total number of channels

// Water level is divided into six, 1.5 inch increments
#define CAP_LVL_1     6   // Bottom most water level ~1.5 inches
#define CAP_LVL_2     7
#define CAP_LVL_3     8
#define CAP_LVL_4     9
#define CAP_LVL_5     10
#define CAP_LVL_6     11

// *** Behavior/Operational ***

// uncomment to send debug information to the serial monitor
#define DEBUG

// Capacitive Touch empty/full threshold in difference between
// baseline reading and the current reading
#define CAP_DRY_THRESHOLD  20  // above this value is DRY; WET is near zero

// States for the state maching
#define STATE_INIT   0  // setting up timers, etc
#define STATE_L_6D   1  // waiting for level 6, empty to be reached
#define STATE_L_5D   2  // waiting for level 5, empty to be reached
#define STATE_L_4D   3  // waiting for level 4, empty to be reached
#define STATE_L_3D   4  // waiting for level 3, empty to be reached
#define STATE_L_2D   5  // waiting for level 2, empty to be reached
// won't empty past level 2 to insure pump remains covered
#define STATE_L_1D   6  // unused
#define STATE_L_FILL 7  // waiting for the float sensor on the other side to trip

// strings representing the numerical state
// NOTE: YOU MUST MANUALLY KEEP THIS IN SYNC WITH THE #define's above
static char *state_phrase[] = {
  "STATE_INIT",
  "STATE_L_6D",
  "STATE_L_5D",
  "STATE_L_4D",
  "STATE_L_3D",
  "STATE_L_2D",
  "STATE_L_1D",
  "STATE_L_FILL",
  "\0"
};

// pace that the process runs
#define LOOP_DELAY   500  // delay at the end of the main loop
#define L_R_DELAY    2000 // time to wait before starting the opposite pump

// timeouts for the steps of the process
// (usually just shutdown if the timer runs out)
#define BETWEEN_L_INTERVAL  15000   // Timeout for betwen level shifts, mS
#define L_R_INTERVAL        60000   // Total time for a L-R, R-L water transfer, mS

// *** Global declarations and variables

// State of the state maching
int state = STATE_INIT;

// Instantiate the Capacitive Sensor board
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

// place to remember the baseline at startup
int  remember_bl[CAP_NUM_CH];
int  cap_current[CAP_NUM_CH];


void setup() {
  // We'll be using the serial monitor to display data
  Serial.begin(9600);

  while (!Serial) { // needed to keep leonardo/micro from starting too fast!
    delay(10);
  }

#ifdef DEBUG
  Serial.println("Aquarium Automation MF Demo w/ Adafruit MPR121 Capacitive Touch sensor test"); 
#endif

  // Setup the pump pins and start with the pumps off
  pinMode(PUMP_L, OUTPUT);
  pinMode(PUMP_R, OUTPUT);
  digitalWrite(PUMP_L, PUMP_OFF);
  digitalWrite(PUMP_R, PUMP_OFF);

  // Setup input pin for the float sensor
  pinMode(FLOAT_TOP, INPUT_PULLUP);

  // Setup input pin for the manual buttons
  pinMode(BUT_PUMP_L, INPUT_PULLUP);
  pinMode(BUT_PUMP_R, INPUT_PULLUP);
  pinMode(BUT_START, INPUT_PULLUP);

  // Verify that the Capacitive Sensor board can be found
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
#ifdef DEBUG
    Serial.println("MPR121 not found, check wiring?");
    Serial.println("... suspending execution");
#endif
    while (1);
  }
#ifdef DEBUG
  Serial.println("MPR121 found!");  // normal behavior
#endif
  
  // Discovered that a little settling time stabilized the initial readings
  delay(2000);

  // Get the water in the right place to start the cycle
#ifdef DEBUG
  Serial.println("Use the manual pump control buttons to move the water");
  Serial.println("to the right side, just above the top-most copper tape.");
  Serial.println("Press the <<Start>> button when done.");
#endif
  while(digitalRead(BUT_START) == BUT_RELEASED)  {
    if(digitalRead(BUT_PUMP_L) == BUT_PRESSED)
      digitalWrite(PUMP_L, PUMP_ON);
    else
      digitalWrite(PUMP_L, PUMP_OFF);
      
    if(digitalRead(BUT_PUMP_R) == BUT_PRESSED)
      digitalWrite(PUMP_R, PUMP_ON);
    else
      digitalWrite(PUMP_R, PUMP_OFF);

    delay(500);  // not too fast
  }

  // in case the while() exited with either pump on
  digitalWrite(PUMP_L, PUMP_OFF);
  digitalWrite(PUMP_R, PUMP_OFF);
  
  delay(1000);  // let the waves settle
    
  // Make the first baseline measurement
  for (uint8_t i=0; i < CAP_NUM_CH; i++)
    remember_bl[i] = cap.baselineData(i);
}

void loop() {
  // read the cap sensor values and perform the diff with the baseline
  for (uint8_t i = 0; i < CAP_NUM_CH; i++) {
    cap_current[i] = cap.filteredData(i) - remember_bl[i];
  }
  
#ifdef DEBUG
  Serial.print("Filt diffs: ");
  for (uint8_t i = CAP_START_CH; i <= CAP_END_CH; i++) {
    Serial.print(cap_current[i]); Serial.print("   ");
  }
  Serial.println();
  Serial.print("Float Status: ");
  Serial.print(digitalRead(FLOAT_TOP));
  Serial.println();

  Serial.print("State: ");
  Serial.print(state_phrase[state]);
  Serial.println();
#endif

//
// The state machine.
// The state machine (after the INIT), like the water, fills from the bottom to the top :-)
//
  switch(state)  {

    case STATE_INIT:  // setting up timers, etc
#ifdef DEBUG
      Serial.print("L->R Water movement commencing in 3 seconds ... ");
      delay(1000);
      Serial.print("2 ... ");
      delay(1000);
      Serial.println("GO !");
#else
      delay(L_R_DELAY);
#endif
      digitalWrite(PUMP_R, PUMP_ON);
      state = STATE_L_FILL;
      break;
      
    case STATE_L_6D:  // waiting for level 6, empty to be reached
      if(cap_current[CAP_LVL_6] < CAP_DRY_THRESHOLD)  {
        digitalWrite(PUMP_L, PUMP_OFF);
        delay(L_R_DELAY);
        digitalWrite(PUMP_R, PUMP_ON);
        state = STATE_L_FILL;
      }
      break;
        
    case STATE_L_5D:  // waiting for level 5, empty to be reached
        if(cap_current[CAP_LVL_5] < CAP_DRY_THRESHOLD)  {
        state = STATE_L_6D;
        }
        break;
        
    case STATE_L_4D:  // waiting for level 4, empty to be reached
        if(cap_current[CAP_LVL_4] < CAP_DRY_THRESHOLD)  {
        state = STATE_L_5D;
        }
        break;
        
    case STATE_L_3D:  // waiting for level 3, full to be reached
        if(cap_current[CAP_LVL_3] < CAP_DRY_THRESHOLD)  {
        state = STATE_L_4D;
        }
        break;
        
    case STATE_L_2D:  // waiting for level 2, full to be reached
        if(cap_current[CAP_LVL_2] < CAP_DRY_THRESHOLD)  {
        state = STATE_L_3D;
        }
        break;
          
    // won't empty past level 2 to insure pump remains covered
    
    case STATE_L_FILL: // waiting for the float sensor on the other side to trip
        if(digitalRead(FLOAT_TOP) == FLOAT_TRIP) {
          digitalWrite(PUMP_R, PUMP_OFF);
          delay(L_R_DELAY);

          state = STATE_L_2D;
        
          digitalWrite(PUMP_L, PUMP_ON);
        }
        break;
    
    default:         // shouldn't ever get here
      Serial.println("Unknown state encountered ... suspending operation");
      digitalWrite(PUMP_L, PUMP_OFF);
      digitalWrite(PUMP_R, PUMP_OFF);
      while(1);
  }
  
  // put a delay so it isn't overwhelming
  delay(LOOP_DELAY);
}
