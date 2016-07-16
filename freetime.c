/*********************************************************/
/******************* FREETIME SYSTEM *********************/
/*********************************************************/
/******************** EME154 UCDAVIS *********************/
/*********************************************************/
# include <8052.h>
# include <stdio.h>
# include <stdlib.h>
# include <MORPH.h>

/***** FUNCTION PROTOTYPES *******************************/
void OCSfunction();
void MOSfunction();
void ACSfunction();
void MSDfunction();

/*********************************************************/
/***** START OF FTS **************************************/
/*********************************************************/

/***** REMARKS SECTION ***********************************
  FTS - Freetime System
  INZ - Initialization
  DIG - Diagnostics
  ERH - Error Treatment Supervisor
  MSS - Machine Status Scan Supervisor
  MCS - Mode Control Supervisor 
  ACS - Automatic Control Supervisor
  MOS - Manual Operation Supervisor
  PGS - Programming Supervisor 
  MSD - Machine Setup Data Supervisor
  OCS - Output Control Supervisor
 ***** END OF REMARKS SECTION *****************************/


/***** START of MESSAGE AND CONSTANTS DEFINITION *********/
void printHeaderAndMenu(){
	clrPC();
	printf("EME 154 Mechatronics\n");
	printf("Freetime System\n");
	printf("Egan McComb\n\n");
	printf("****************************************\n");
	printf("            OPERATION MENU\n");
	printf("****************************************\n\n");
	printf("1. Manual Operation\n");
	printf("2. Automatic Operation\n");
	printf("3. Machine Data Setup\n");
	printf("4. Idle\n");
	printf("5. Exit\n\n\n\n");
}
/***** END of MESSAGE AND CONSTANTS DEFINITION ***********/

/*********************************************************/
/***** FTS PROGRAM SECTION *******************************/
/*********************************************************/

/***** START OF INZ **************************************/
char *machineMessage;
unsigned char OMD, ERR1, FFRA;
unsigned char MOMD, MOMH, MOMM, MOMS, MOMMS;
unsigned char ACMD, ACMH, ACMM, ACMS, ACMMS;
unsigned char MSMD, MSMH, MSMM, MSMS, MSMMS;
__data __at (0x21) unsigned volatile char counter;

void INZfunction(){
	/* Print output to the MPS console. */
	printMode = 0;
	/* Operation Mode is Idle. */
	OMD = 0;
	/* Level 1 error flag for the system. */
	ERR1 = 0;
	/* Set First Run Flag. */
	FFRA = 1;
	/* Initialize the time variables. */
	MOMD = MOMH = MOMM = MOMS = MOMMS = 0;
	ACMD = ACMH = ACMM = ACMS = ACMMS = 0;
	MSMD = MSMH = MSMM = MSMS = MSMMS = 0;

	/* Run the Realtime System. */
	__asm
		lcall 0x9600;
	__endasm;
}
/***** END OF INZ ****************************************/

/***** START OF DIG **************************************/
int diagnostics(){
	if (ERR1) {
		/* Error. */
		return 0;
	} else {
		/* Success. */
		return 1;
	}
}
/***** END OF DIG ****************************************/

/***** ERH ***********************************************/
void ERHfunction(){
	/* Take actions to treat errors here. */
}
/***** END OF ERH ****************************************/

/***** START OF MSS **************************************/
void MSSfunction(){
	char selection;
	selection = key();

	/* Skip on first run. */
	if (FFRA) OCSfunction();
	/* Scan keyboard for input. */
	switch (selection) {
		case '1':
			OMD = 1;
			break;
		case '2':
			OMD = 2;
			break;
		case '3':
			OMD = 3;
			break;
		case '4':
			OMD = 0;
			machineMessage = "Idle Command Accepted";
			break;
		case '5':
			OMD = 5;
			/* Stop the timer. */
			IE = 0;
			TR1 = 0;
			LEDs = 0;
			machineMessage = "Exit Command Accepted";
			break;
		default:
			break;
	}	
}
/***** END OF MSS ****************************************/

/***** START OF MCS **************************************/
void MCSfunction(){
	if (OMD == 1) {
		MOSfunction();
	} else if (OMD == 2) {
		ACSfunction();
	} else if (OMD == 3) {
		MSDfunction();
	}
}
/***** END OF MCS ****************************************/

/***** START OF MOS **************************************/
void MOSfunction(){	
	/* Count time properly. */
	if (MOMH > 23) {
		MOMD++;
		MOMH = 0;
	}
	if (MOMM > 59) {
		MOMH++;
		MOMM = 0;
	}
	if (MOMS > 59) {
		MOMM++;
		MOMS = 0;
	}
	if (MOMMS > 10){
		MOMS++;
	}
	/* 0xFF is the initial. value and 50*20ms is 1s. */
	if (counter < (0xFF-50)) {
		/* Prevent counter overflow, count time instead. */
		MOMS++;
		counter = 0xFF;
	}

	machineMessage = "Manual Mode Accepted";
}
/***** END OF MOS ****************************************/

/***** START OF ACS **************************************/
void ACSfunction(){
	/* Count time properly. */
	if (ACMH > 23) {
		ACMD++;
		ACMH = 0;
	}
	if (ACMM > 59) {
		ACMH++;
		ACMM = 0;
	}
	if (ACMS > 59) {
		ACMM++;
		ACMS = 0;
	}
	if (ACMMS > 10){
		ACMS++;
	}
	/* 0xFF is the initial. value and 50*20ms is 1s. */
	if (counter < (0xFF-50)) {
		/* Prevent counter overflow, count time instead. */
		ACMS++;
		counter = 0xFF;
	}

	machineMessage = "Automatic Mode Accepted";

}
/***** END OF ACS ****************************************/


/***** START OF MSD **************************************/
void MSDfunction(){
	/* Count time properly. */
	if (MSMH > 23) {
		MSMD++;
		MSMH = 0;
	}
	if (MSMM > 59) {
		MSMH++;
		MSMM = 0;
	}
	if (MSMS > 59) {
		MSMM++;
		MSMS = 0;
	}
	if (MSMMS > 10){
		MSMS++;
	}
	/* 0xFF is the initial. value and 50*20ms is 1s. */
	if (counter < (0xFF-50)) {
		/* Prevent counter overflow, count time instead. */
		MSMS++;
		counter = 0xFF;
	}

	machineMessage = "Setup Mode Accepted";

}
/***** END OF MSD ****************************************/

/***** Function gets mode name from OMD ******************/
char *getCurrentMode() {
	switch (OMD) {
		case 1:
			return "Manual";
		case 2:
			return "Automatic";
		case 3:
			return "Setup";
		case 5:
			return "System Turned Off";
		default:
			return "Idle";
	}
}

/***** START OF OCS **************************************/
void OCSfunction() {
	if (FFRA) {
		printHeaderAndMenu();
		/* Clear first run flag. */
		FFRA = 0;
	} else {
		setCur(0,14);
		printf("%s        \n", machineMessage);
		printf("Current Mode: %s        \n\n", getCurrentMode());
		printf("*** RUN TIME LOG ***\n");
		printf("Manual %d D %d H %d M %d S \n", MOMD, MOMH, MOMM, MOMS);
		printf("Auto   %d D %d H %d M %d S \n", ACMD, ACMH, ACMM, ACMS);
		printf("Setup  %d D %d H %d M %d S \n\n", MSMD, MSMH, MSMM, MSMS);
	}
}
/***** END OF OCS ****************************************/

/***** START OF MAIN *************************************/
void main(){
	INZfunction();

	while(OMD != 5 && !microButtons) {
		/* Run diagnostics. */
		if (diagnostics()) {
			/* If success, continue. */
			MSSfunction();
			MCSfunction();
			OCSfunction();
		} else {
			/* Else go to the error handler. */
			ERHfunction();
			OCSfunction();
		}
	}
}
/***** END OF MAIN ***************************************/

