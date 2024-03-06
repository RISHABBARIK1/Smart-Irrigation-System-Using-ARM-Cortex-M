#include <stdio.h>
#include "lcd.h"

#define VREF 3.3			 // Reference Voltage at VREFP pin, given VREFN = 0V(GND)
#define ADC_CLK_EN (1 << 12) // enable power for ADC in  PCONP[12]
#define SEL_AD0_1 (1 << 1)	 // Select Channel AD0.1  in ADCR[1]
#define CLKDIV (3 << 8)		 // set ADC clk using clock-divider bits of ADCR [15:8] :  (ADC_CLOCK=PCLK/CLKDIV+1) = 1Mhz @ 4Mhz PCLK
#define PWRUP (1 << 21)		 // Enable ADC for operation mode in ADCR[21] bit
#define START_CNV (1 << 24)	 // set bits to 001 for starting the conversion immediately in ADCR[26:24]
#define ADC_DONE (1U << 31)	 // monitor end of ADC conversion in  ADCR[31] bit

#define ROW_PINS (0x0F << 4) // selecting (4 to 7)
#define COL_PINS (0x0F << 0) // selecting  (0 to 4)

#define ALL_LED_PIN (0xFF << 19)
#define LED_PIN(x) (1 << x)
#define SWITCH (1 << 11)
#define BUZZER(x) (1 << x)

char key_pressed();

int main(void)
{
	int result = 0;
	float volts = 0;
	float ans = 0;
	char svolts[20];
	char key = 0;

	LPC_GPIO2->FIODIR |= ROW_PINS;	// making output pins (4 to 7)
	LPC_GPIO2->FIODIR &= ~COL_PINS; // making input pins	  (0 to 3)
	LPC_GPIO1->FIODIR |= ALL_LED_PIN;
	LPC_GPIO2->FIODIR &= ~SWITCH;
	LPC_GPIO1->FIODIR |= BUZZER(27);

	LPC_GPIO1->FIOCLR = 0xFF << 19;				// Turn LED off
	LPC_PINCON->PINSEL1 |= (0x01 << 16);		// select alternate functionality for pin- P0.24  as ADC input channel AD0.1  in PINSEL1[17:16]
	LPC_SC->PCONP |= ADC_CLK_EN;				// Enable ADC clock, default its off
	LPC_ADC->ADCR = PWRUP | CLKDIV | SEL_AD0_1; // select channel1,adc clocK,software mode and power-on for ADC

	lcd_init();
	lcd_cmd_write(0x0C); // cursor off

	lcd_str_write("Enter Crop Type ");
	delay(3000);
	lcd_cmd_write(0x01);

	key = key_pressed();

	lcd_str_write("Crop Type: ");
	lcd_dat_write(key);
	delay(1000);
	lcd_cmd_write(0x01);

	while (1)
	{
		if ((LPC_GPIO2->FIOPIN & SWITCH) != 0)
		{
			LPC_GPIO1->FIOSET = BUZZER(27);
		}

		LPC_ADC->ADCR |= START_CNV; // Start new Conversion

		while ((LPC_ADC->ADDR1 & ADC_DONE) == 0) // Wait untill conversion is finished while monitoring the DONE (bit 31)
		{
		}

		result = (LPC_ADC->ADDR1 >> 4) & 0xFFF; // 12 bit Mask to extract result[15:4] from addr1

		volts = (result * VREF) / 4096.0; // Convert result to Voltage
		ans = volts * 1000;
		sprintf(svolts, "Moisture: %.1f %", ans);
		if (key == '0')
		{
			lcd_str_write("Wrong Crop Input");
		}
		else if (key == '1')
		{
			lcd_str_write(svolts);
			lcd_cmd_write(0xC0);
			lcd_str_write("Threshold: 30%");
			delay(500);

			if (volts < 0.03)
			{
				LPC_GPIO1->FIOSET = LED_PIN(19);
			}

			else
			{
				LPC_GPIO1->FIOCLR = LED_PIN(19);
			}
		}

		else if (key == '2')
		{
			lcd_str_write(svolts);
			delay(500);
			lcd_cmd_write(0xC0);
			lcd_str_write("Threshold: 50%");			

			if (volts < 0.05)
			{
				LPC_GPIO1->FIOSET = LED_PIN(20);
			}

			else
			{
				LPC_GPIO1->FIOCLR = LED_PIN(20);
			}
		}

		else if (key == '3')
		{
			lcd_str_write(svolts);
			delay(500);
			lcd_cmd_write(0xC0);
			lcd_str_write("Threshold: 70%");			

			if (volts < 0.07)
			{
				LPC_GPIO1->FIOSET = LED_PIN(21);
			}

			else
			{
				LPC_GPIO1->FIOCLR = LED_PIN(21);
			}
		}

		else if (key == '4')
		{
			lcd_str_write(svolts);
			delay(500);
			lcd_cmd_write(0xC0);
			lcd_str_write("Threshold: 90%");			

			if (volts < 0.09)
			{
				LPC_GPIO1->FIOSET = LED_PIN(22);
			}

			else
			{
				LPC_GPIO1->FIOCLR = LED_PIN(22);
			}
		}

		delay(500);
		lcd_cmd_write(0x01);
	}
}

char key_pressed()
{
	char keypressed = 0;
	uint8_t i, j, val;
	uint8_t scan[4] = {0xE, 0xD, 0xB, 0x7}; // set the value to check further which row is slected
	char key[4][4] = {{'0', '1', '2', '3'}, // char array as we have to display on lcd which takes only ASCII value
					  {'4', '5', '6', '7'},
					  {'8', '9', 'A', 'B'},
					  {'C', 'D', 'E', 'F'}};

	while (1)
	{
		for (i = 0; i < 4; i++) // for each row
		{
			LPC_GPIO2->FIOCLR |= ROW_PINS;		// set pin as zero
			LPC_GPIO2->FIOSET |= scan[i] << 4;	// jb tk button not pressed tbtk all high else it takes value form array scan
			val = LPC_GPIO2->FIOPIN & COL_PINS; // masking the vlaue of 0-7 with col pins 0-4 to find columns pins value (0 to 3 pins)

			for (j = 0; j < 4; j++) // for colummn selection
			{
				if (val == scan[j]) // is any key pressed
					break;
			}

			if (val != 0x0F) // if  any key is pressed in the upper scanning
			{
				keypressed = key[i][j];

				if(keypressed == '1' || keypressed == '2' || keypressed == '3' || keypressed == '4')
					return keypressed;
				
				else
					return '0';
			}
		}
	}
}