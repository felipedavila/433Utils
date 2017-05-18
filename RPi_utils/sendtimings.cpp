/*
Usage: ./codesend decimalcode [protocol] [pulselength]
decimalcode - As decoded by RFSniffer
protocol    - According to rc-switch definitions
pulselength - pulselength in microseconds

 'codesend' hacked from 'send' by @justy
 
 - The provided rc_switch 'send' command uses the form systemCode, unitCode, command
   which is not suitable for our purposes.  Instead, we call 
   send(code, length); // where length is always 24 and code is simply the code
   we find using the RF_sniffer.ino Arduino sketch.

(Use RF_Sniffer.ino to check that RF signals are being produced by the RPi's transmitter 
or your remote control)
*/
#include "../rc-switch/RCSwitch.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sstream>

int main(int argc, char *argv[]) {
    
    // This pin is not the first pin on the RPi GPIO header!
    // Consult https://projects.drogon.net/raspberry-pi/wiringpi/pins/
    // for more information.
    int PIN = 0;
    
    // Parse the first parameter to this command as a string

    // If no command line argument is given, print the help text
    if (argc == 1) {
        printf("Usage: %s \"space separated timings\"\n", argv[0]);
        printf("space separated timings\t- As provided by generatetimings or SimpleScanner (allows commas)\n");
        return -1;
    }

    std::string strtimings = argv[1];

    //Checking for white spaces inside the string
    int count = 0;
    for (int i = 0; i < strtimings.length(); i++)
    {
      if (isspace(strtimings.at(i))) 
      {
        count++;
      }
    }

    unsigned int * transmitarray = new unsigned int[count+2];


    std::istringstream iss(strtimings);
    int i = 0;

    do
    {
        std::string sub;
        iss >> sub;
        transmitarray[i] = atoi(sub.c_str());
        i++;
    } while (iss);


    transmitarray[i-1] = 0;
    
    if (wiringPiSetup () == -1) return 1;
    printf("sending [%i] timings\n", i-1);
    RCSwitch mySwitch = RCSwitch();
    mySwitch.enableTransmit(PIN);
    

    
    unsigned int currenttiming = 0;
    while( transmitarray[currenttiming] && currenttiming < count+2 ) {
      printf("%i, ", transmitarray[currenttiming] );
      currenttiming++;
    }
    printf("\n");

    mySwitch.transmittimings = transmitarray;
    mySwitch.send(transmitarray);

    return 0;

}
