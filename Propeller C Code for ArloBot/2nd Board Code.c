/* 2nd Propeller Board (QuickStart Board) Code for ArloBot

   This code should run on the 2nd board and send data to the primary
   Activity Board via two pins using fdserial.
*/

#include "simpletools.h"
#include "mcp3208.h" // MCP3208 8 Chanel ADC
#include "ping.h" // Include ping header
#include "fdserial.h" // http://learn.parallax.com/propeller-c-simple-protocols/full-duplex-serial
// and http://propsideworkspace.googlecode.com/hg-history/daf5de8bf52840e02d5615edaa6d814e59d1b0b0/Learn/Simple%20Libraries/Text%20Devices/libfdserial/html/fdserial_8h.html#ab14338477b0b96e671aed748e20ecf5e

fdserial *term;
fdserial *propterm;

int mcp3208_IR_cm(int); // Function to get distance in CM from IR sensor using MCP3208

const int numberOfPINGsensors = 10;
const int numberOfIRonMC3208 = 6; // Number of IR Sensors on the MCP3208 ADC to read
const int firtPINGsensorPIN = 2; // Which pin the first PING sensor is on
const int propRXpin = 0;
const int propTXpin = 1;
void pollPingSensors(void *par); // Use a cog to fill range variables with ping distances
static int pstack[128]; // If things get weird make this number bigger!
int isActive = 0;
int main()                    
{

  // Close the simpleterm half duplex connection
  simpleterm_close(); // The simpleterm_close function call shuts down the default half-duplex communication, which will cause print calls to stop working.  But that’s fine because we can use full-duplex serial.
  // Start a full duplex terminal session
  //term = fdserial_open(31, 30, 0, 115200);
    // Start the sensor cog(s)
	cogstart(&pollPingSensors, NULL, pstack, sizeof pstack);
    
    // Blinken Lights!
  const int pauseTime = 50;
  const int startLED = 17; // 16 is used for the pollPingSensors cog to indicate activity!
  const int endLED = 23;
  high(startLED);
  pause(pauseTime);
  low(startLED);
  while(1) {
  pause(5);
  if(isActive == 1) {
      for (int led = startLED + 1; led <= endLED; led++) {
        high(led);
        pause(pauseTime);
        low(led);
      }
      for (int led = endLED - 1; led >= startLED; led--) {
        high(led);
        pause(pauseTime);
        low(led);
      }
    isActive = 0;
    }
  }
}

void pollPingSensors(void *par) {
  const int pingDelay = 25;
  // The last IR sensor will be retagged with this position number,
  // in case there are more PINGs than IRs.
  const int lastIRposition = 7;
  propterm = fdserial_open(propRXpin, propTXpin, 0, 115200);
  //term = fdserial_open(31, 30, 0, 115200); // for Debugging
  int ping = 0, ir = 0;
 while(1)                                    // Repeat indefinitely
  {
    /* We wait for ANY input from the other side,
       Which lets us not activate the sensors if the other end is not working,
       and also lets the other end rate limit the input */
    char receivedChar = fdserial_rxChar(propterm);
    //char receivedChar = 'i'; // for Debugging - Cause it to always run instead of waiting for a signal
    if (receivedChar == 'i') { // Only send data when we get the expected "init" character, avoiding running on random garbage from an open connection
      high(16); // LEDs for debugging
      isActive = 1;
      for(int i=0; i < numberOfPINGsensors; i++ ) {
        ping = ping_cm(firtPINGsensorPIN + i);
        dprint(propterm, "p,%d,%d\n", i, ping);
        //dprint(term, "p,%d,%d", i, ping); // For Debugging
        if(i < numberOfIRonMC3208) { // If there is also an IR sensor at this number check it too
          ir = mcp3208_IR_cm(i);
          if(i ==  numberOfIRonMC3208 - 1) {// Last IR is actually in position 7, convert it here!
            dprint(propterm, "i,%d,%d\n", lastIRposition, ir);
            //dprint(term, "i,%d,%d", lastIRposition, ir); // For Debugging
          } else {
            dprint(propterm, "i,%d,%d\n", i, ir);
            //dprint(term, "i,%d,%d", i, ir); // For Debugging
          }
        }
        pause(pingDelay); // Otherwise it is just stupid fast!
      }
     //dprint(term, "\n"); // For Debugging - add a line break here and pull the above two
    }
    low(16); // LEDs for debugging
    fdserial_rxFlush(propterm); // In case we got a flood for some reason, flush it.
  }
}
/* TODO: Be sure to test that the data coming in is REAL TIME! */

int mcp3208_IR_cm(int channel) {
const int MCP3208_dinoutPIN = 27;
const int MCP3208_clkPIN = 25;
const int MCP3208_csPIN = 26;
const float referenceVoltage = 5.0; // MCP3208 reference voltage setting. I use 5.0v for the 5.0v IR sensors from Parallax
  int mcp3208reading  = readADC(channel, MCP3208_dinoutPIN, MCP3208_clkPIN, MCP3208_csPIN);
  float mcp3208volts = (float)mcp3208reading * referenceVoltage / 4096.0;
  int mcp3208cm = 27.86 * pow(mcp3208volts, -1.15); // https://www.tindie.com/products/upgradeindustries/sharp-10-80cm-infrared-distance-sensor-gp2y0a21yk0f/
  return(mcp3208cm);
}