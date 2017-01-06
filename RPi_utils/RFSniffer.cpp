/*
  RFSniffer

  Usage: ./RFSniffer [<pulseLength>]
  [] = optional

  Hacked from http://code.google.com/p/rc-switch/
  by @justy to provide a handy RF code sniffer
*/

#include "../rc-switch/RCSwitch.h"
#include <stdlib.h>
#include <stdio.h>
// required to compute squareroot to get a standard deviation of timings
#include <math.h>     
     
RCSwitch mySwitch;


int main(int argc, char *argv[]) {
  
     // This pin is not the first pin on the RPi GPIO header!
     // Consult https://projects.drogon.net/raspberry-pi/wiringpi/pins/
     // for more information.
     int PIN = 2;
     
     if(wiringPiSetup() == -1) {
       printf("wiringPiSetup failed, exiting...");
       return 0;
     }

     int pulseLength = 0;
     if (argv[1] != NULL) pulseLength = atoi(argv[1]);

     mySwitch = RCSwitch();
     if (pulseLength != 0) mySwitch.setPulseLength(pulseLength);
     mySwitch.enableReceive(PIN);  // Receiver on interrupt 0 => that is pin #2
     
    
     while(1) {
  
      if (mySwitch.available()) {
    
        int value = mySwitch.getReceivedValue();
    
        if (value == 0) {
          printf("Unknown encoding\n");
        } else {    
   
          printf("Received %i\n", mySwitch.getReceivedValue() );

          if ((argc > 1 && strcmp(argv[1],"-v") == 0) || (argc > 2 && strcmp(argv[2],"-v") == 0)) {
		  unsigned int *databuffer = mySwitch.getReceivedRawdata(); 
		  int nreceived = mySwitch.getReceivedValue();
		  int nbitlength = mySwitch.getReceivedBitlength();
		  int nprotocol = mySwitch.getReceivedProtocol();
		  int npulselength = mySwitch.getReceivedDelay();
		
		  printf("\n=============================================\n");
		  printf("=============================================\n");
		  printf("\n");
		  printf("GENERAL\n");
		  printf("=======\n");
		  printf("Received %i\n", nreceived );
		  printf("Bitlength %i\n", nbitlength );
		  printf("Protocol %i\n", nprotocol );
                  // (or long/short ratio in protocoless detection) 
		  printf("Delay(short pulse length in us) %i\n", npulselength );
		  printf("(delay from rc-switch is calculated from the sync gap and requires previous knowledge of the protocol)\n");
		  if(2*nbitlength+2 < 64) { //prevent bufferoverrun
		  printf("(The following assumes protocols that detect the low period of sync)\n");
		  printf("(The next information is probably wrong for inverted protocols)\n");
		  printf("Sync bit high-low durations (in us) %i %i\n", databuffer[2*nbitlength+1], databuffer[0] );
		  printf("\n");
		  printf("RAW SIGNAL (us)\n");
		  printf("===============\n");
		  for (int transmitlevels = 0; transmitlevels < 2*nbitlength+2; transmitlevels++) {
		    printf("%i ", databuffer[transmitlevels] );
		    if(transmitlevels % 16 == 0) printf("\n");
		  }
		  printf("\n");
		  printf("\n");
		  printf("STATISTICS\n");
		  printf("==========\n");
		
		  // compute duration of data bits
		  // compute databit duration variability
		  unsigned int dataduration = 0;
		  for (unsigned int i = 1; i < 2*nbitlength; i += 2) {
		    dataduration += databuffer[i]+databuffer[i + 1];
		  }
		
		  // compute average databit duration
		  unsigned int averagebitduration = 0;
		  if(nbitlength != 0) {
			  averagebitduration = (int) (0.5 + (double)dataduration/(double)nbitlength); // warning, ensure avoiding 0 division (should not happen because rc-switch rejects < 4 bits)
		  }
		  int variancebitduration = 0;
		  for (unsigned int i = 1; i < 2*nbitlength; i += 2) {
		    variancebitduration += (databuffer[i]+databuffer[i + 1]-averagebitduration)*(databuffer[i]+databuffer[i + 1]-averagebitduration);
		  }
		  if((nbitlength-1) != 0) {
			  variancebitduration /= (nbitlength-1); // warning, ensure avoiding 0 division (should not happen because rc-switch rejects < 4 bits)
		  } else {
			  variancebitduration = 0;
		  }
		  double sdbitduration = sqrt(variancebitduration);
		
		  printf("Data bit duration = %i and standard deviation (should be less than 10%) = %.2f\n", averagebitduration, sdbitduration );
		  printf("Do not use the rest of the information if big standard deviation\n");
		  int longtoshortratio = (int)(0.5 + (double)(averagebitduration-npulselength)/(double)npulselength);
		  printf("Long-to-short duration ratio for data bits (rounded) %i\n", longtoshortratio );
		  int normalizedpulselength = (int)(0.5 + (double)averagebitduration/(double)(longtoshortratio+1));
		  printf("Short duration (delay) recalculated from rounded ratio and average bit duration %i\n", normalizedpulselength );
		  // sync bit
		  printf("Sync bit (in multiples of the previous short duration) %i %i\n", (int) (0.5 + (double)databuffer[2*nbitlength+1]/(double)normalizedpulselength) ,(int) (0.5 + (double)databuffer[0]/(double)normalizedpulselength) );
		  printf("\n");
		  printf("Proposed protocol for RCswitch\n");
		  printf("{ %i, { %i, %i }, { 1, %i }, { %i, 1 }, false }\n",normalizedpulselength,(int) (0.5 + (double)databuffer[2*nbitlength+1]/(double)normalizedpulselength),(int) (0.5 + (double)databuffer[0]/(double)normalizedpulselength),longtoshortratio,longtoshortratio);
		  printf("Note: inverted protocol is not implemented\n");
		
		  printf("\n");
		  printf("STATISTICS OF VARIATION BY LEVELS\n");
		  printf("=================================\n");
		  printf("(These are probably artifacts from detection delays or signal creation)\n");
		  printf("(might be completely ignored, but pay attention to them if big differences are present and emission does not work)\n");

		  int longup = 0;
		  int shortup = 0;
		  int longdown = 0;
		  int shortdown = 0;
		
		  // high level long and short timings
		  for (unsigned int i = 1; i < 2*nbitlength; i += 2) {
		    if(databuffer[i]>databuffer[i + 1]){
		    longup += databuffer[i];
		    shortdown += databuffer[i + 1];
		    } else {
		    shortup += databuffer[i];
		    longdown += databuffer[i + 1];
		    }
		  }
		  // low level long and short timings
		  if(nreceived != 0) {
			  int numberofsetbits = __builtin_popcount(nreceived);
			  printf("number of bits set (in %i): %i\n",nreceived,numberofsetbits);
			  longup /= numberofsetbits; // warning, ensure avoiding 0 division
			  shortdown /= numberofsetbits; // warning, ensure avoiding 0 division
			  shortup /= nbitlength-numberofsetbits; // warning, ensure avoiding 0 division
			  longdown /= nbitlength-numberofsetbits; // warning, ensure avoiding 0 division
			  printf("longup, longdown, shortup, shortdown\n");
			  printf("%i, %i, %i, %i\n",longup, longdown, shortup, shortdown);
			  printf("this might be useful for tweaking emmitting algorithms\n");
			  }
		  printf("\n=============================================\n");
		  printf("=============================================\n");
		  printf("\n");
		  printf("\n");
		  printf("\n");
		  }
          }
        }
    
        mySwitch.resetAvailable();
    
      }
      
  
  }

  exit(0);


}

