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
#define ONE_REG_SIZE		6
#define RX_SIZE				8

u8 startCellMeasure[2] = {0x02, 0xF0}; // Hard coding for now // This should poll adc too actually
// bit 10: 	0
// bit 9:  	1
// bit 8:	0 		// Not using S adc RD bit
// byte: 0x02		//bit[3:7] have to be zero

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
		 * Data frame structure: 7* (6830config + 2950config). Because Dasiy Chain
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
		buff_6830_a[5] = 0x00; // bits [2:0] is for filter.
		// We need to set bit 3 in the last byte for the last one in the daisy chain. We'll do it when we create the final buffer that is sent

		buff_6830_b[0] = 0x71; // Under voltage value. Not sure right now
		buff_6830_b[1] = 0x52; // Undervoltage and overvoltage
		buff_6830_b[2] = 0x46; // Over voltage. Both are set to 1.5V when these registers are set to 0x00
		buff_6830_b[3] = 0b00111111; // Setting DTMEN and DTRNG. DCTO to full

		buff_6830_b[4] = 0x00; // DCC to zero. Not sure what to set here
		buff_6830_b[5] = 0x00; // DCC to zero

		// TODO: Add 2950 config
		/*
		 * can't test config for 2950 because we don't know what they do.
		 */

		// Put the reg data into the buffer that will be sent
		for(int x = 0  ; x < TOTAL_IC; x ++){

			//if(x == (TOTAL_IC-1)) buff_6830_a[5] |= 0b1<<3; // COMM_BK flag set for the last chain
			// The do not set this bit in the example code

			memcpy(buff_data+(x*6 ), buff_6830_a, 6); // Index for 6830 for the segment.
	//		memcpy(buff_data+(x*12), buff_2950_a, 6); // Index for 2950 for the segment.
		}

		// Final data buffer will have 6*TOTAL_IC bytes for each config register
		spiWriteData(TOTAL_IC, WRCFGA, buff_6830_a); // Note this function reverses the data buffer while sending.

		for(int x = 0 ; x < TOTAL_IC; x ++){
			memcpy(buff_data+(x*6), buff_6830_b, 6); // Index for 6830 for the segment.
	//		memcpy(buff_data+(x*12 + 6), buff_2950_b, 6); // Index for 2950 for the segment.
		}
		// Final data buffer will have 14*6 bytes for each config register
		spiWriteData(TOTAL_IC, WRCFGB, buff_6830_b);
		printf("\n\rWRCFGA: ");
		FORIN(_x , 6){
			printf("%02x ", buff_6830_a[_x]);
		}

		printf("\n\rWRCFGB: ");
		FORIN(_x , 6){
			printf("%02x ", buff_6830_b[_x]);
		}


		return 0;


}

int requestCellVotlage(void){
	int retval = 0;

	return retval;
}

/// Making this do everything lowkey
int pollCellVoltage(u8* rxdata){
	int retval = 0;
	u8 _dumpbyte = 0;
	uint32_t pecerr = 0;
	u8 cmd_cnt[TOTAL_IC];
	wakeup_chain(TOTAL_IC);

	spiSendCmd(startCellMeasure); // This should
	spiSendCmd(startCellMeasure);
	spiCSHigh(); // spiSendCmd pulls CS low before sending. It doesn't pull it back up high so we don't use another function for polling
	spiSendCmd(PLADC);
	while(_dumpbyte != 0xFF){
		spi_read(&_dumpbyte, 1);
	}
	spiCSHigh();

	FORIN(__j, 1){
//	spiReadData(TOTAL_IC, RDCVA, rxdata, &pecerr, cmd_cnt, RDCALL_SIZE);
	spiReadData(TOTAL_IC, RDCVB, rxdata, &pecerr, cmd_cnt, 8);
	spiReadData(TOTAL_IC, RDCVB, rxdata + 6, &pecerr, cmd_cnt, 8);
	spiReadData(TOTAL_IC, RDCVC, rxdata + 12, &pecerr, cmd_cnt, 8);
	spiReadData(TOTAL_IC, RDCVD, rxdata + 18, &pecerr, cmd_cnt, 8);
	spiReadData(TOTAL_IC, RDCVE, rxdata + 24, &pecerr, cmd_cnt, 8);
	spiReadData(TOTAL_IC, RDCVF, rxdata + 30, &pecerr, cmd_cnt, 8);
	retval = pecerr;
	}
	return retval;
}

int requestAuxVoltage(void){
	int retval = 0;

	return retval;
}

int pollAuxVoltage(u8* rxdata){
	int retval = 0;
	u8 _dumpbyte = 0;
	uint32_t pecerr = 0;
	u8 cmd_cnt[TOTAL_IC];


	spiSendCmd(startAux2Measurement); // This should work :)
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


int readCFG(void){
	u8 cfgBuffer[TOTAL_IC*RX_SIZE];
	uint32_t pecerr = 0;
	u8 cmd_cnt[TOTAL_IC];
	memset(cfgBuffer, 0x00, TOTAL_IC*RX_SIZE);


//	spiReadData(TOTAL_IC, RDCFGA, cfgBuffer, &pecerr, cmd_cnt,  RX_SIZE);
	wakeup_chain(TOTAL_IC);


	spiReadData(TOTAL_IC, RDCFGA, cfgBuffer, &pecerr, cmd_cnt,  RX_SIZE);

	printf("\n\rCFGA: ");

	FORIN(i, 6){
		printf("%02x ", cfgBuffer[i]);
	}
	printf("\n\rPEC:%lu", pecerr);
	memset(cfgBuffer, 0x00, TOTAL_IC*6);


	spiReadData(TOTAL_IC, RDCFGB, cfgBuffer, &pecerr, cmd_cnt,  RX_SIZE);

	printf("\n\rCFGB: ");

	FORIN(i, 6){
		printf("%02x ", cfgBuffer[i]);
	}
	printf("\n\rPEC:%lu", pecerr);

	printf("\n\r %d", *cmd_cnt);

	return 0;
}


int readSID(void){
	u8 cfgBuffer[TOTAL_IC*6];
	uint32_t pecerr = 0;
	u8 cmd_cnt[TOTAL_IC];


	spiReadData(TOTAL_IC, RDSID, cfgBuffer, &pecerr, cmd_cnt,  ONE_REG_SIZE);

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

	spiReadData(TOTAL_IC, RDSTATC, statBuffer, &pecerr, cmd_cnt,  ONE_REG_SIZE);

	printf("\n\rSTATERR: ");

	FORIN(i, 6){
			printf("%02x ", statBuffer[i]);
		}

	printf("\n\rPEC:%lu", pecerr);

	return 0;
}

