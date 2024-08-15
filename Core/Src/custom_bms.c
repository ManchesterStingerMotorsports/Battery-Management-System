/*
 * custom_6830.c
 *
 *  Created on: Feb 8, 2024
 *      Author: malhaar-k
 */

#include "custom_bms.h"
#include <stdio.h>
#include <string.h>
#include "adBms6830CmdList.h"
#include "adBms6830GenericType.h"
#include "adBms6830Data.h"
#include "mcu_wrapper.h"
#include "main.h"

#define RDCALL_SIZE 		34
#define ONE_REG_SIZE		8
#define RX_SIZE				8

u8 startCellMeasure[2] = {0x03, 0x60}; // Hard coding for now // This should poll adc too actually
// bit 10: 	0
// bit 9:  	1
// bit 8:	1 		// Setting the redundant measurement bit
// byte: 0x03		//bit[3:7] have to be zero

// bit 7:	0 		// Single shot measurement, 1 For continuous
// bit 6:	1
// bit 5:	1
// bit 4:	0 		// DCP bit is set so cell discharging for balancing is possible during measurement
// bit 3:	0
// bit 2:	0		// Not resetting the filter since it's not required yet
// bit 1:	0
// bit 0:	0		// OW[1:0] Set to not check for open wire for now
// byte: 0x60

u8 startAux2Measurement[2] = {0x04, 0x00}; //
// bit 10: 	1		// This is correct. Trust
// bit 9:  	0
// bit 8:	0 		// Not using S adc
// byte: 0x04

// bit 7:	0 		// Second byte is all zero if we are reading all gpios
// bit 6:	0
// bit 5:	0
// bit 4:	0
// bit 3:	0
// bit 2:	0
// bit 1:	0
// bit 0:	0		// CH[3:0] is set to 0000 so everything is measured
// byte: 0x40






int configBMS(void){
	wakeup_chain(TOTAL_IC);
		/*
		 * Write config register A. Use WRCFGA.
		 * Write config register B. Use WRCFGB.
		 * Data frame structure: 2950config + (TOTAL_IC-1)*(6830config)
		 */
		uint8_t buff_6830_a[6];
		uint8_t buff_6830_b[6];
		uint8_t buff_2950_a[6];
		uint8_t buff_2950_b[6];
		uint8_t buff_data[TOTAL_IC*6];


		// Init CONFIG REGISTER A constants. All magic numbers, but we will add the meaning in readme or docs
		buff_6830_a[0] = (0x01<<7); // Reference powered on
		buff_6830_a[1] = 0x00; // All flags = 0

		// TODO: Find out what SOAK does
		buff_6830_a[2] = 0x00; // Control reg for soak functions. Cleared for now. Will set when soak is understood
		buff_6830_a[3] = 0xFF; // GPIOs [8:0] are all pulled down.(For ADC measurements)
		buff_6830_a[4] = 0x03; // GPIOs 10 and 9 are not pulled down.
		buff_6830_a[5] = (0x01<<3); // bits [2:0] is for filter.
		// We need to set bit 3 in the last byte for the last one in the daisy chain. We'll do it when we create the final buffer that is sent

		buff_6830_b[0] = 0x71; // Under voltage value. Not sure right now
		buff_6830_b[1] = 0x52; // Undervoltage and overvoltage
		buff_6830_b[2] = 0x46; // Over voltage. Both are set to 1.5V when these registers are set to 0x00
		buff_6830_b[3] = 0b00111111; // Setting DTMEN and DTRNG. DCTO to full

		buff_6830_b[4] = 0x00; // DCC to zero. Not sure what to set here
		buff_6830_b[5] = 0x00; // DCC to zero

		buff_2950_a[0] = 0x80; // Enable overcurrent ADC
		buff_2950_a[1] = 0;
		buff_2950_a[2] = 0;
		buff_2950_a[3] = 0;
		buff_2950_a[4] = 0;
		buff_2950_a[5] = 0x10; // Ref enable

		buff_2950_b[0] = 0;
		buff_2950_b[1] = 0;
		buff_2950_b[2] = 0;
		buff_2950_b[3] = 0;
		buff_2950_b[4] = 0;
		buff_2950_b[5] = 0;
		/* ======================= End of config definitiom =============================== */

		// Put the reg data into the buffer that will be sent. We're gonna do manually while testing.
		memcpy(buff_data, buff_2950_a, 6); // Index for 2950 for the segment.
		memcpy(buff_data+6, buff_6830_a, 6); // Index for 6830 for the segment.

		// Change this to a for loop later

		// Final data buffer will have 6*TOTAL_IC bytes for each config register
		spiWriteData(TOTAL_IC, WRCFGA, buff_data); // Note this function reverses the data buffer while sending.

		memcpy(buff_data, buff_2950_b, 6); // Index for 2950 for the segment.
		memcpy(buff_data+6, buff_2950_b, 6); // Index for 6830 for the segment.


		// Final data buffer will have 14*6 bytes for each config register
		spiWriteData(TOTAL_IC, WRCFGB, buff_data);

		/* ============================ Debugging statements ====================================*/
		printf("\n\r6830 WRCFGA: ");
		FORIN(_x , 6){
			printf("%02x ", buff_6830_a[_x]);
		}

		printf("\n\r6830 WRCFGB: ");
		FORIN(_x , 6){
			printf("%02x ", buff_6830_b[_x]);
		}

		printf("\n\r2950 WRCFGA: ");
		FORIN(_x , 6){
			printf("%02x ", buff_2950_a[_x]);
		}

		printf("\n\r2950 WRCFGB: ");
		FORIN(_x , 6){
			printf("%02x ", buff_2950_b[_x]);
		}

		/* ============================ End of Debugging statements ==============================*/

		return 0;


}

/* @brief Initiates ADC measurement for all devices in the daisy chain.
 *
 * TODO: Switch to reading the status register and reading the values only if any error flags
 * 		 are set.
 *
 */
int pollCellVoltage(u8* rxdata){
	int retval = 0;
	u8 _dumpbyte = 0;
	uint32_t pecerr = 0;
	u8 cmd_cnt[TOTAL_IC];
	wakeup_chain(TOTAL_IC);

	spiSendCmd(startCellMeasure);
	spiCSHigh();
	spiSendCmd(startCellMeasure);
	spiCSHigh(); // spiSendCmd pulls CS low before sending. It doesn't pull it back up high so we don't use another function for polling
	// Sending the command twice works more relibaly without affecting it's function.

	spiSendCmd(PLADC);

	while(_dumpbyte != 0xFF){
		spi_read(&_dumpbyte, 1);
	}
	spiCSHigh();

	FORIN(__j, 1){
	// The pointer arithmetic needs to change based on the number of ICs
	spiReadData(TOTAL_IC, RDCVA, rxdata, &pecerr, cmd_cnt, RX_SIZE);
	spiReadData(TOTAL_IC, RDCVB, rxdata + (6*TOTAL_IC), &pecerr, cmd_cnt, RX_SIZE); // Pointer maths might be wrong
	spiReadData(TOTAL_IC, RDCVC, rxdata + (12*TOTAL_IC), &pecerr, cmd_cnt, RX_SIZE);
	spiReadData(TOTAL_IC, RDCVD, rxdata + (18*TOTAL_IC), &pecerr, cmd_cnt, RX_SIZE);
	spiReadData(TOTAL_IC, RDCVE, rxdata + (24*TOTAL_IC), &pecerr, cmd_cnt, RX_SIZE);
	spiReadData(TOTAL_IC, RDCVF, rxdata + (30*TOTAL_IC), &pecerr, cmd_cnt, RX_SIZE);
	retval = pecerr;
	}
	return retval;
}


int pollAuxVoltage(u8* rxdata){
	int retval = 0;
	u8 _dumpbyte = 0;
	uint32_t pecerr = 0;
	u8 cmd_cnt[TOTAL_IC];


	spiSendCmd(startAux2Measurement); // This should work :)
	spiCSHigh();
	spiSendCmd(startAux2Measurement);
	spiCSHigh(); // spiSendCmd pulls CS low before sending. It doesn't pull it back up high so we don't use another function for polling
	spiSendCmd(PLAUX2);
	while(_dumpbyte != 0xFF){
		spi_read(&_dumpbyte, 1);
		printf("Polling\n\r");
	}
	spiCSHigh();

	// No read aux2 all command unfort
	spiReadData(TOTAL_IC, RDRAXA, rxdata, &pecerr, cmd_cnt,  ONE_REG_SIZE);
	spiReadData(TOTAL_IC, RDRAXB, rxdata + 6, &pecerr, cmd_cnt,  ONE_REG_SIZE);
	spiReadData(TOTAL_IC, RDRAXC, rxdata + 12, &pecerr, cmd_cnt,  ONE_REG_SIZE);
	spiReadData(TOTAL_IC, RDRAXD, rxdata + 18, &pecerr, cmd_cnt,  ONE_REG_SIZE);
	if(pecerr > 0){
		retval = pecerr;
	}
	return retval;
}

/* @brief Reads all the config registers.
 * Not needed during normal operation.
 * Mostly needed for debugging
 */
int readCFG(void){
	u8 cfgBuffer[TOTAL_IC*6];
	uint32_t pecerr = 0;
	u8 cmd_cnt[TOTAL_IC];
	memset(cfgBuffer, 0x00, TOTAL_IC*RX_SIZE); // TODO: Warning - 'memset' writing 16 bytes into a region of size 12 overflows the destination [-Wstringop-overflow=]

	wakeup_chain(TOTAL_IC);

	spiReadData(TOTAL_IC, RDCFGA, cfgBuffer, &pecerr, cmd_cnt,  RX_SIZE);

	printf("\n\rCFGA: \n\r");

	FORIN(i, TOTAL_IC*6){
		printf("%02x ", cfgBuffer[i]);
		(void)((i+1) % 6 ? 0 : printf("\n\r"));
	}
	printf("\n\rPEC:%lu", pecerr);
	memset(cfgBuffer, 0x00, TOTAL_IC*6);


	spiReadData(TOTAL_IC, RDCFGB, cfgBuffer, &pecerr, cmd_cnt,  RX_SIZE);

	printf("\n\rCFGB: \n\r");

	FORIN(i, TOTAL_IC*6){
		printf("%02x ", cfgBuffer[i]);
		(void)((i+1) % 6 ? 0 : printf("\n\r"));
	}
	printf("\n\rPEC:%lu", pecerr);

	printf("\n\r %d", *cmd_cnt);

	return 0;
}


int readSID(void){
	u8 cfgBuffer[TOTAL_IC*6];
	uint32_t pecerr = 0;
	u8 cmd_cnt[TOTAL_IC];


	spiReadData(TOTAL_IC, RDSID, cfgBuffer, &pecerr, cmd_cnt,  RX_SIZE);

	printf("\n\rSID: ");

	FORIN(i, 6){
		printf("%02x ", cfgBuffer[i]);
	}
	printf("\n\rPEC:%lu", pecerr);

	return 0;
}

int readStatErr(void){
	u8 statBuffer[TOTAL_IC*6];
	uint32_t pecerr = 0;
	u8 cmd_cnt[TOTAL_IC];
	memset(statBuffer, 0x00, TOTAL_IC*6);

	spiReadData(TOTAL_IC, RDSTATE, statBuffer, &pecerr, cmd_cnt,  RX_SIZE);

	printf("\n\rSTATERR: ");

	FORIN(i, TOTAL_IC*6){
			printf("%02x ", statBuffer[i]);
		}

	printf("\n\rPEC:%lu", pecerr);

	return 0;
}

