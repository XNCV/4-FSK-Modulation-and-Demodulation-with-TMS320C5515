/*
 *  AIC3204 Tone
 *  Bui Thanh Tinh
 */
#include "stdio.h"
#include "usbstk5515.h"
extern Int16 AIC3204_rset( Uint16 regnum, Uint16 regval);
#define Rcv 0x08
#define Xmit 0x20
#include "math.h"

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  AIC3204 Loop                                                            *
 *      Output input from STEREO IN 1 through the HEADPHONE jack            *
 *                                                                          *
 * ------------------------------------------------------------------------ */

#define Fs 48000
#define F0 1000
#define F1 2000
#define F2 3000
#define F3 4000
#define PI 3.14159264

Int16 count=0;
Int16 state=0;


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
    /* Pre-generated sine wave data, 16-bit signed samples */
    Int16 level, j, i = 0;
    //Int16 sample, data1, data2, data3, data4;
    Int16 sample, left_output, right_output;
    Int16 left_input, right_input;
    Int16 pre_value, current_value;
    /* Configure AIC3204 */
    //signal = (double*) malloc( nsample * sizeof(double));
    short ptsig0[96];
	short ptsig1[96];
	short ptsig2[96];
	short ptsig3[96];
	short twobit[2];
	
    /* Create 4 sine signals*/
    Int16 nsample = 96; int x;
	for (x=0; x < nsample; x++){
		ptsig0[x] = 1333*sin(2*PI*x*F0/Fs);
	}
	for (x=0; x < nsample; x++){
		ptsig1[x] = 1333*sin(2*PI*x*F1/Fs);
	}
	for (x=0; x < nsample; x++){
		ptsig2[x] = 1333*sin(2*PI*x*F2/Fs);
	}
	for (x=0; x < nsample; x++){
		ptsig3[x] = 1333*sin(2*PI*x*F3/Fs);
	}
	
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
      	        aic3204_codec_read(&left_input, &right_input);
      	        switch(state)
      	        {
      	        	case 0:
      	        		pre_value = left_input;
      	        		if(pre_value!=0)
      	        		{
      	        			state = 1;
      	        		}
      	        		break;
      	        	case 1:
      	        		current_value = left_input;
      	        		// Catch up the change of bits
      	        		if(((pre_value < 0)&&(current_value > 0))||((pre_value > 0)&&(current_value < 0)))
						{
							state = 2;
							twobit[0]=0;twobit[1]=0;
						}
						break;
					case 2:
						count++;
						if(count == 120) twobit[0] = left_input;
						if(count == 360) twobit[1] = left_input;
						if((twobit[0] < 0)&&(twobit[1] < 0)) level = 3;
						else if ((twobit[0] < 0)&&(twobit[1] > 0)) level = 2;
						else if ((twobit[0] > 0)&&(twobit[1] < 0)) level = 1;
						else if ((twobit[0] > 0)&&(twobit[1] > 0)) level = 0;
						if (count==480) {
							count = 0; twobit[0]=0;twobit[1]=0;}
						break;
      	        }

      	        /*if(left_input>0){
					left_output = ptsig0[sample]; //16383
				}
				if(left_input<0){
					left_output = ptsig3[sample];
				}*/
      	        if(level == 0){
					left_output = ptsig0[sample];
				}
				if(level == 1){
					left_output = ptsig1[sample];
				}
				if(level == 2){
					left_output = ptsig2[sample];
				}
				if(level == 3){
					left_output = ptsig3[sample];
				}
				right_output = left_output;
				//left_output = 0;
				//right_output = 0;
      	        aic3204_codec_write(left_output, right_output);

            }
        }
    }
    /* Disble I2S */
    I2S0_CR = 0x00;
   
    return 0;
}
