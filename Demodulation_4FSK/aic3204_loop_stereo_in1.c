/*
 *  AIC3204 Tone _4-FSK Demodulation code
 *  Bui Thanh Tinh
 */
#include "stdio.h"
#include "usbstk5515.h"
extern Int16 AIC3204_rset( Uint16 regnum, Uint16 regval);
#define Rcv 0x08
#define Xmit 0x20
#include "math.h"
#include "ptsig.h"
#include <stdio.h>
#include <Dsplib.h>
#include "low_pass_200_800.h"


#define Fs 48000
#define F0 1000
#define F1 2000
#define F2 3000
#define F3 4000
#define PI 3.14159264

Int16 count=-1;
Int16 state=0;
Int16 level=0;

extern void aic3204_codec_read(Int16* left_input, Int16* right_input)
{
      while((Rcv & I2S0_IR) == 0);  // Wait for interrupt pending flag
      *left_input = I2S0_W0_MSW_R;  // 16 bit left channel received audio data
      //data1 = I2S0_W0_LSW_R;
      *right_input = I2S0_W1_MSW_R; // 16 bit right channel received audio data
      //data2 = I2S0_W1_LSW_R;       
}

extern void aic3204_codec_write(Int16 left_output, Int16 right_output)
{
		/* Write Digital audio */
      	while((Xmit & I2S0_IR) == 0);  // Wait for interrupt pending flag
		I2S0_W0_MSW_W = left_output;  // 16 bit left channel transmit audio data
      	I2S0_W0_LSW_W = 0;
      	I2S0_W1_MSW_W = right_output;  // 16 bit right channel transmit audio data
      	I2S0_W1_LSW_W = 0;
}


Int16 aic3204_loop_stereo_in1( )
{
    /*  */
    Int16 j, i, ii;
    Int16 data0, data1, data2, data3;
    Int16 sample;
    Int16 left_input, right_input;
    Int16 pre_value, current_value, current_level;

    /* Configure AIC3204 */
	Int16 left_input_matrix[1];
	Int16 dbuffer0[LPL+2]={0};
	Int16 dbuffer1[LPL+2]={0};
	Int16 dbuffer2[LPL+2]={0};
	Int16 dbuffer3[LPL+2]={0};
	Int16 left_output_matrix[1];
	
	Int16 temp_output_matrix0[1];
	Int16 temp_output_matrix1[1];
	Int16 temp_output_matrix2[1];
	Int16 temp_output_matrix3[1];
	
	Int16 signal_before_filter[1];
	long int mul0, mul1, mul2, mul3;
    Int16 nsample = 96;

	// Initially set up for AIC3204
    AIC3204_rset( 0, 0 );          // Select page 0
    AIC3204_rset( 1, 1 );          // Reset codec
    AIC3204_rset( 0, 1 );          // Select page 1
    AIC3204_rset( 1, 8 );          // Disable crude AVDD generation from DVDD
    AIC3204_rset( 2, 1 );          // Enable Analog Blocks, use LDO power
    AIC3204_rset( 0, 0 );          // Select page 0
    /* PLL and Clocks config and Power Up  */
    AIC3204_rset( 27, 0x0d );      // BCLK and WCLK is set as o/p to AIC3204(Master)
    AIC3204_rset( 28, 0x00 );      // Data ofset = 0
    AIC3204_rset( 4, 3 );          // PLL setting: PLLCLK <- MCLK, CODEC_CLKIN <-PLL CLK
    AIC3204_rset( 6, 7 );          // PLL setting: J=7
    AIC3204_rset( 7, 0x06 );       // PLL setting: HI_BYTE(D=1680)
    AIC3204_rset( 8, 0x90 );       // PLL setting: LO_BYTE(D=1680)
    AIC3204_rset( 30, 0x88 );      // For 32 bit clocks per frame in Master mode ONLY
                                   // BCLK=DAC_CLK/N =(12288000/8) = 1.536MHz = 32*fs
    AIC3204_rset( 5, 0x91 );       // PLL setting: Power up PLL, P=1 and R=1
    AIC3204_rset( 13, 0 );         // Hi_Byte(DOSR) for DOSR = 128 decimal or 0x0080 DAC oversamppling
    AIC3204_rset( 14, 0x80 );      // Lo_Byte(DOSR) for DOSR = 128 decimal or 0x0080
    AIC3204_rset( 20, 0x80 );      // AOSR for AOSR = 128 decimal or 0x0080 for decimation filters 1 to 6
    AIC3204_rset( 11, 0x82 );      // Power up NDAC and set NDAC value to 2
    AIC3204_rset( 12, 0x87 );      // Power up MDAC and set MDAC value to 7
    AIC3204_rset( 18, 0x87 );      // Power up NADC and set NADC value to 7
    AIC3204_rset( 19, 0x82 );      // Power up MADC and set MADC value to 2
    /* DAC ROUTING and Power Up */
    AIC3204_rset(  0, 0x01 );      // Select page 1
    AIC3204_rset( 12, 0x08 );      // LDAC AFIR routed to HPL
    AIC3204_rset( 13, 0x08 );      // RDAC AFIR routed to HPR
    AIC3204_rset(  0, 0x00 );      // Select page 0
    AIC3204_rset( 64, 0x02 );      // Left vol=right vol
    AIC3204_rset( 65, 0x00 );      // Left DAC gain to 0dB VOL; Right tracks Left
    AIC3204_rset( 63, 0xd4 );      // Power up left,right data paths and set channel
    AIC3204_rset(  0, 0x01 );      // Select page 1
    AIC3204_rset( 16, 0x00 );      // Unmute HPL , 0dB gain
    AIC3204_rset( 17, 0x00 );      // Unmute HPR , 0dB gain
    AIC3204_rset(  9, 0x30 );      // Power up HPL,HPR
    AIC3204_rset(  0, 0x00 );      // Select page 0
    USBSTK5515_wait( 500 );        // Wait
    
    /* ADC ROUTING and Power Up */
    AIC3204_rset( 0, 1 );          // Select page 1
    AIC3204_rset( 0x34, 0x30 );    // STEREO 1 Jack
		                           // IN2_L to LADC_P through 40 kohm
    AIC3204_rset( 0x37, 0x30 );    // IN2_R to RADC_P through 40 kohmm
    AIC3204_rset( 0x36, 3 );       // CM_1 (common mode) to LADC_M through 40 kohm
    AIC3204_rset( 0x39, 0xc0 );    // CM_1 (common mode) to RADC_M through 40 kohm
    AIC3204_rset( 0x3b, 0 );       // MIC_PGA_L unmute
    AIC3204_rset( 0x3c, 0 );       // MIC_PGA_R unmute
    AIC3204_rset( 0, 0 );          // Select page 0
    AIC3204_rset( 0x51, 0xc0 );    // Powerup Left and Right ADC
    AIC3204_rset( 0x52, 0 );       // Unmute Left and Right ADC
    
    AIC3204_rset( 0, 0 );    
    USBSTK5515_wait( 200 );        // Wait
    /* I2S settings */
    I2S0_SRGR = 0x0;
    I2S0_CR = 0x8010;    // 16-bit word, slave, enable I2C
    I2S0_ICMR = 0x3f;    // Enable interrupts
    USBSTK5515_wait( 50 );
    
    /* Play Tone */
    for ( i = 0 ; i < 50 ; j++ )
    {
        for ( j = 0 ; j < 1000 ; j++ )
        {
            for ( sample = 0 ; sample < nsample ; sample++ )
            {
      	        aic3204_codec_read(left_input_matrix, &right_input);			

				
				mul0 = (long int)left_input_matrix[0] * (long int)ptsig0[sample];
				mul1 = (long int)left_input_matrix[0] * (long int)ptsig1[sample];
				mul2 = (long int)left_input_matrix[0] * (long int)ptsig2[sample];
				mul3 = (long int)left_input_matrix[0] * (long int)ptsig3[sample];
				for ( ii = 0; ii < 4; ii++ )
				{
					if(ii==0) 
					{
						signal_before_filter[0] = (Int16)(mul0/666);
						fir(signal_before_filter,   		// input
						LP,		// coef
						temp_output_matrix0,		// output
						dbuffer0,	// Z-1 blocks and more
						1,		// number to process
						LPL		// number of parameters
						);
					}
					if(ii==1)
					{
						signal_before_filter[0] = (Int16)(mul1/666);
						fir(signal_before_filter,   		// input
						LP,		// coef
						temp_output_matrix1,		// output
						dbuffer1,	// Z-1 blocks and more
						1,		// number to process
						LPL		// number of parameters
						);
					}
					if(ii==2)
					{
						signal_before_filter[0] = (Int16)(mul2/666);
						fir(signal_before_filter,   		// input
						LP,		// coef
						temp_output_matrix2,		// output
						dbuffer2,	// Z-1 blocks and more
						1,		// number to process
						LPL		// number of parameters
						);
					}
					if(ii==3)
					{
						signal_before_filter[0] = (Int16)(mul3/666);
						fir(signal_before_filter,   		// input
						LP,		// coef
						temp_output_matrix3,		// output
						dbuffer3,	// Z-1 blocks and more
						1,		// number to process
						LPL		// number of parameters
						);
					}
				}
				data0 = abs(temp_output_matrix0[0]); data1 = abs(temp_output_matrix1[0]); data2 = abs(temp_output_matrix2[0]); data3 = abs(temp_output_matrix3[0]);
				if((data0>data1)&&(data0>data2)&&(data0>data3)) level = 0; //left_output_matrix[0] = ptsig0[sample];
				if((data1>data0)&&(data1>data2)&&(data1>data3)) level = 1; //left_output_matrix[0] = ptsig1[sample];
				if((data2>data0)&&(data2>data1)&&(data2>data3)) level = 2; //left_output_matrix[0] = ptsig2[sample];
				if((data3>data0)&&(data3>data1)&&(data3>data2)) level = 3; //left_output_matrix[0] = ptsig3[sample];
				
				switch(state)
      	        {
      	        	case 0:
      	        		pre_value = level;
      	        		state = 1;
      	        		break;
      	        	case 1:
      	        		current_value = level;
      	        		// Catch up the change of level
      	        		if(pre_value!=current_value)
						{
							state = 2;
						}
						break;
					case 2:
						count++;
						if (count == 240) current_level = level;
						if (current_level == 0) left_output_matrix[0] = 0;
						else if (current_level == 3) left_output_matrix[0] = 1333;
						else if (current_level == 2)
						{
							if(count < 240) left_output_matrix[0] = 0;
							else left_output_matrix[0] = 1333;
						}
						else
						{
							if(count < 240) left_output_matrix[0] = 1333;
							else left_output_matrix[0] = 0;
						}
						if (count==480) count = 0;
						break;
      	        }
      	        aic3204_codec_write(left_output_matrix[0], left_output_matrix[0]);
            }
        }
    }
    /* Disble I2S */
    I2S0_CR = 0x00;
   
    return 0;
}
