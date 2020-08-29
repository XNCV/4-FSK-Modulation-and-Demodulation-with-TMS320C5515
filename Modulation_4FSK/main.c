/*
 * 4-FSK Modulation
 * Bui Thanh Tinh
 */

#include "stdio.h"
#include "usbstk5515.h"

void main( void )
{
    /* Initialize BSL */
    USBSTK5515_init( );
    
    SYS_EXBUSSEL = 0x6100;         // Enable I2C bus
	USBSTK5515_I2C_init( );        // Initialize I2C
	// Test connection
    printf("EXBUSSEL = %02x\n", SYS_EXBUSSEL);
    printf("Test the connection\n");
    aic3204_tone_headphone();
    USBSTK5515_wait( 100 );
    printf("Testing the connection is done.\n 4-FSK Modulation Mode\n");
    // Main loop
    aic3204_loop_stereo_in1();

    printf( "\n***END***\n" );
    SW_BREAKPOINT;
}
