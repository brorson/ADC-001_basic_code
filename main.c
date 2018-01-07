#include <stdio.h>
#include <stdlib.h>
// #include <stdarg.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "spidriver_host.h"
#include "adcdriver_host.h"

//-----------------------------------------------------
// Called when Ctrl+C is pressed - triggers the program to stop.
void stopHandler(int sig) {
  adc_quit();
  exit(0);
}


//==========================================================
// This is main program which exercises the A/D.  You may use
// this program as a starting place for development of your
// own code.
int main (void)
{
  // Loop variables
  uint32_t i;
  uint32_t j;

  // Buffers for tx and rx data from A/D registers.
  uint32_t tx_buf[3];
  uint32_t rx_buf[4];

  // Measured voltages from A/D
  float volt;          // Single measurement.
  float volts[1024];   // Vector of measurements 

  // File ID
  FILE *fp;

  // Used in peak-peak measurement
  float vmax, vmin;

  // Stuff used with "hit return when ready..." 
  char dummy[8];

  printf("------------   Starting main.....   -------------\n");

  // Run until Ctrl+C pressed:
  signal(SIGINT, stopHandler);

  // Sanity check user.
  if(getuid()!=0){
     printf("You must run this program as root. Exiting.\n");
     exit(EXIT_FAILURE);
  }

  // Initialize A/D converter
  adc_config();

  // Test PRU RAM
  printf("--------------------------------------------------\n");
  i = 123456;
  printf("About to test PRU RAM.  Write value = %d\n", i);
  printf("Hit return when ready -->\n");
  fgets (dummy, 8, stdin);
  j = pru_test_ram(1, i);
  printf("Just wrote value to PRU RAM and then read it back.  Value read = %d\n", j);


  // Test PRU communication link
  printf("--------------------------------------------------\n");
  printf("About to test PRU communication link\n");
  printf("Hit return when ready -->\n");
  fgets (dummy, 8, stdin);
  j = pru_test_communication();
  printf("Just did communication test.  Number of cycles for read = %d\n", j);


  // Now check the A/D is alive by reading from its config reg.
  printf("--------------------------------------------------\n");
  printf("About to read A/D config register\n");
  printf("Hit return when ready -->\n");
  fgets (dummy, 8, stdin);
  rx_buf[0] = adc_get_id_reg();
  printf("Read ID reg.  Received ID = 0x%08x\n", rx_buf[0]);


  // Set sample rate to 32kSPS
  printf("--------------------------------------------------\n");
  printf("Set sample rate to 32kSPS and set channel 1\n");
  printf("Hit return when ready -->\n");
  fgets (dummy, 8, stdin);
  adc_set_samplerate(SAMP_RATE_31250);
  adc_set_chan1();


  // Now to multiple read, print out returned buffer and save to file
  printf("--------------------------------------------------\n");
  j = 64;
  printf("Now try to do multiple read of of %d values and write to file.\n", j);
  printf("Hit return when ready -->\n");
  fgets (dummy, 8, stdin);
  adc_read_multiple(j, volts);
  fp = fopen("measured_volts.dat", "w");
  printf("Values read = \n");
  for (i=0; i<j; i++) {
    printf("i = %d, V = %e\n", i, volts[i]);
    fprintf(fp, "%e\n", volts[i]);
  }
  fclose(fp);

  // Now change sample freq, do multiple read, and print out buf again.
  printf("--------------------------------------------------\n");
  j = 64;
  printf("Now change sample rate to 100.8 Hz and to do multiple read of of %d values\n", j);
  printf("Hit return when ready -->\n");
  fgets (dummy, 8, stdin);
  adc_set_samplerate(SAMP_RATE_1008);
  adc_read_multiple(j, volts);
  printf("Values read = \n");
  for (i=0; i<j; i++) {
    printf("i = %d, V = %e\n", i, volts[i]);
  }


  // Do a bunch of single reads in a loop.  Wait 500mS between each read.
  printf("--------------------------------------------------\n");
  printf("About to do a series of 8 single reads spaced by 500mS.\n");
  printf("Hit return when ready -->\n");
  fgets (dummy, 8, stdin);
  // Loop here getting ADC value every time a reading is ready.
  // This assumes continuous read mode.
  for (i=0; i<8; i++) {
    // Read A/D channels 01. 
    volt = adc_read_single();
    printf("In main, i = %d, voltage reading 1 = %f\n", i, volt);
    usleep(500000);
  }


  // Now change sample freq, change to chan 1, do multiple read, and print out buf again.
  printf("--------------------------------------------------\n");
  j = 64;
  printf("Now change chan to chan 1 and to do multiple read of of %d values\n", j);
  printf("Hit return when ready -->\n");
  fgets (dummy, 8, stdin);
  adc_set_chan1();
  adc_set_samplerate(SAMP_RATE_31250);
  adc_read_multiple(j, volts);
  printf("Values read = \n");
  for (i=0; i<j; i++) {
    printf("i = %d, V = %e\n", i, volts[i]);
  }


  // Now stay in this loop forever.
  j = 1024;
  printf("--------------------------------------------------\n");
  printf("Now do infinite loop of multiple read of %d values\n", j);
  printf("Hit return when ready -->\n");
  fgets (dummy, 8, stdin);
  adc_set_samplerate(SAMP_RATE_31250);
  while(1) {
    adc_read_multiple(j, volts);
    printf("Values read = \n");
    for (i=0; i<j; i++) {
      printf("i = %d, V = %e\n", i, volts[i]);
    }
 
    vmin = 0;
    vmax = 0;
    for (i=0; i<j; i++) {
      if (volts[i] > vmax) {
        vmax = volts[i];
      } else if (volts[i] < vmin) {
        vmin = volts[i];
      }
    }
    printf("Peak-peak voltage = %f\n", vmax-vmin);
    usleep(1000000);   // delay 1 sec.
  }

}

