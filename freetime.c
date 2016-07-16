/*********************************************************/
/******************* FREETIME SYSTEM *********************/
/*********************************************************/
/******************** EME154 UCDAVIS *********************/
/*********************************************************/
# include <8052.h>
# include <stdio.h>
# include <stdlib.h>
# include <MORPH.h>
# define MAXL 6

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
  ERT - Error Treatment Supervisor
  MSS - Machine Status Scan Supervisor
  MCS - Mode Control Supervisor
  ACS - Automatic Control Supervisor
  MOS - Manual Operation Supervisor
  PGS - Programming Supervisor
  MSD - Machine Setup Data Supervisor
  OCS - Output Control Supervisor
 ***** END OF REMARKS SECTION *****************************/


/***** START of MESSAGE AND CONSTANTS DEFINITION *********/
/* Clear one line. */
void clrLine() {
	printf("\t\t\t\t\t\t\t\t");
}

/* Clear n lines. */
void clrSCR(unsigned char lines) {
	unsigned char i;

	for (i = 0; i < lines; i++) {
		clrLine();
		printf("\n");
	}
}

void printHeader() {
	clrPC();
	printf("EME 154 Mechatronics\n");
	printf("Branding\n");
	printf("Team Name\n\n");
}

void printMSD(unsigned char submode) {
	setCur(0,8);
	/* Update current initial position. */
	printf("Current Initial Position: %d\t\t\n", initp);
	if (submode == 0) {
		/* Display mode. */
		printf("Press Enter to Change\n");
	} else {
		/* Edit mode. */
		clrLine();
		printf("Enter Initial Value: \n");
	}
}

void printACSidle(unsigned char submode) {
	setCur(0,8);
	/* Sequence of combination number inputs. */
	switch (submode) {
		case 0:
			printf("Press R to Reset Fields\n");
			printf("Press Enter to Change 1st Number\n");
			break;
		case 1:
			setCur(0,9);
			clrLine();
			printf("Enter 1st Dial Value: \n");
			break;
		case 2:
			setCur(0,10);
			printf("Press Enter to Change 2nd Number\n");
			break;
		case 3:
			setCur(0,10);
			clrLine();
			printf("Enter 2nd Dial Value: \n");
			break;
		case 4:
			setCur(0,11);
			printf("Press Enter to Change 3rd Number\n");
			break;
		case 5:
			setCur(0,11);
			clrLine();
			printf("Enter 3rd Dial Value: \n");
			break;
		case 6:
			setCur(0,12);
			printf("Press Enter to Start RTS\n");
			break;
		default:
			break;
	}
}

void printACSactive(unsigned char submode) {
	setCur(0,8);
	printf("Elapsed time: XXX\n");
	/* Update completion status. */
	switch (submode) {
		case 0:
			printf("1st Dial: XXX\n");
			printf("2nd Dial: XXX\n");
			printf("3rd Dial: XXX\n");
			break;
		case 1:
			setCur(13,9);
			printf("(Completed)\n");
			break;
		case 2:
			setCur(13,10);
			printf("(Completed)\n");
			break;
		case 3:
			setCur(13,11);
			printf("(Completed)\n");
			break;
		default:
			break;
	}
}

void printMOS() {
	if (submode == 0) {
		/* Display commands. */
		setCur(0,8);
		printf("Current Position: XXX\t\t\n");
		printf("Q/W        10 ticks CCW/CW\n");
		printf("A/S         5 ticks CCW/CW\n");
		printf("Z/X         1 ticks CCW/CW\n");
		printf("Bksp/Enter 40 ticks CCW/CW\n");
		printf("Spacebar      fire solenoid\n");
	} else {
		/* Update position. */
		setCur(0,8);
		printf("Current Position: XXX\t\t\n");
	}
}

void printMenu() {
	setCur(0,8);
	/* Display commands. */
	printf("1    Machine Setup Data\n");
	printf("2    Automatic Operation\n");
	printf("3    Manual Operation\n");
}
/***** END of MESSAGE AND CONSTANTS DEFINITION ***********/

/*********************************************************/
/***** FTS PROGRAM SECTION *******************************/
/*********************************************************/

/***** START OF INZ **************************************/
/* Machine message field. */
char *machineMessage;
/* Initial position MSD. */
unsigned char initp = 0;
/* Keep track of HMI state inside of a mode. */
unsigned char submode = 0;
/* Operational mode, error flag, first run flag. */
unsigned char OMD, ERR1, FFRA;
/* Timer values. */
unsigned char MD, MH, MM, MS;
/* UD counter from RTS. */
__data __at (0x21) unsigned volatile char counter;

/* Calculate relative number of pulses to achieve desired position. */
/* XXX: Implement when ASC details are known. */
unsigned int gotoPosition(unsigned char position) {
	unsigned int pulses;
	return pulses;
}

void INZfunction() {
	/* Print output to the MPS console. */
	printMode = 0;
	/* Operation Mode is Idle. */
	OMD = 0;
	/* Level 1 error flag for the system. */
	ERR1 = 0;
	/* Set First Run Flag. */
	FFRA = 1;
	/* Initialize the time variables. */
	MD = MH = MM = MS = 0;

	/* Run the Realtime System. */
	__asm
		lcall 0x9600;
	__endasm;
}
/***** END OF INZ ****************************************/

/***** START OF DIG **************************************/
int diagnostics() {
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
void ERHfunction() {
	/* Take actions to treat errors here. */
}
/***** END OF ERH ****************************************/

/***** START OF MSS **************************************/
void MSSidle(char selection) {
	/* Select mode or quit. */
	switch (selection) {
		case '1':
			/* MSD */
			OMD = 1;
			break;
		case '2':
			/* ACS */
			OMD = 2;
			break;
		case '3':
			/* MOS */
			OMD = 4;
			break;
		case 0x03:	// ^C.
			/* Exit. */
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

void MSSMSD(char selection) {
	/* Edit or quit. */
	switch (selection) {
		case 0x0D:	// CR.
			submode = 1;
			break;
		case 0x03:	// ^C.
			/* Return to menu. */
			OMD = 0;
			break;
		default:
			break;
	}
}

void MSSACSidle(char selection) {
	/* Reset, edit, or quit. */
	switch (selection) {
		case 'r':
			submode = 0;
			break;
		case 0x0D:	// CR.
			submode++;
			break;
		case 0x03:	// ^C.
			OMD = 0;
			break;
		default:
			break;
	}
}

/* XXX: Setup key commands. */
void MSSMOS(char selection) {
	/* Rotate, unlock, or quit. */
	switch (selection) {
		case 'q':
			break;
		case 'w':
			break;
		case 'a':
			break;
		case 's':
			break;
		case 'z':
			break;
		case 'x':
			break;
		case 0x08:	// BS.
			break;
		case 0x0D:	// CR.
			break;
		case 0x20:	// Space.
			break;
		case 0x03:	// ^C.
			OMD = 0;
			break;
	}
}

void MSSfunction(unsigned char OMD) {
	char selection;

	/* Skip on first run. */
	if (FFRA) OCSfunction();

	/* Scan keyboard for input. */
	selection = key();

	/* Mode-specific MSS. */
	switch (OMD) {
		case 0:
			MSSidle(selection);
			break;
		case 1:
			MSSMSD(selection);
			break;
		case 2:
			MSSACSidle(selection);
			break;
		case 3:
			break;
		case 4:
			MSSMOS(selection);
			break;
		default:
			break;
	}
}
/***** END OF MSS ****************************************/

/***** START OF MCS **************************************/
void MCSfunction() {
	if (OMD == 0) {
		/* Reset submode. */
		submode = 0;
	} else if (OMD == 1) {
		MSDfunction();
	} else if (OMD == 2 || OMD == 3) {
		ACSfunction();
	} else if (OMD == 4) {
		MOSfunction();
	}
}
/***** END OF MCS ****************************************/

/***** START OF MOS **************************************/
void MOSfunction() {	
	machineMessage = "Manual Mode Accepted";
}
/***** END OF MOS ****************************************/

/***** START OF ACS **************************************/
void ACSfunction() {
	machineMessage = "Automatic Mode Accepted";

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
	if (ACMMS > 10) {
		ACMS++;
	}
	/* 0xFF is the initial. value and 50*20ms is 1s. */
	if (counter < (0xFF-50)) {
		/* Prevent counter overflow, count time instead. */
		ACMS++;
		counter = 0xFF;
	}
}
/***** END OF ACS ****************************************/


/***** START OF MSD **************************************/
void MSDfunction() {
	machineMessage = "Setup Mode Accepted";
}
/***** END OF MSD ****************************************/

/***** Function gets mode name from OMD ******************/
char *getCurrentMode() {
	switch (OMD) {
		case 1:
			return "Setup";
		case 2:
			return "Automatic Idle";
		case 3:
			return "Automatic Active";
		case 4:
			return "Manual";
		case 5:
			return "System Turned Off";
		default:
			return "Idle";
	}
}

/***** START OF OCS **************************************/
void OCSfunction() {
	if (FFRA) {
		printHeader();
		/* Clear first run flag. */
		FFRA = 0;
	} else {
		setCur(0,4);
		printf("Current Mode: %s\t\t\n", getCurrentMode());
		printf("%s\t\t\n", machineMessage);
		/* Print mode appropriate HMI. */
		if (OMD == 0) {
			/* Display quitting information and menu. */
			printf("Press ^C to Exit\t\t\n");
			setCur(0,8);
			clrSCR(MAXL);
			printMenu();
		} else {
			/* Display escape information and HMI. */
			printf("Press ^C to Return to Menu\t\t\n");
			switch (OMD) {
				case 1:
					clrSCR(MAXL);
					printMSD(submode);
					break;
				case 2:
					clrSCR(MAXL);
					printACSidle(submode);
					break;
				case 3:
					clrSCR(MAXL);
					printACSactive(submode);
					break;
				case 4:
					clrSCR(MAXL);
					printMOS();
					break;
				default:
					break;
			}
		}
	}
}
/***** END OF OCS ****************************************/

/***** START OF MAIN *************************************/
void main() {
	INZfunction();

	while(OMD != 5 && !microButtons) {
		/* Run diagnostics. */
		if (diagnostics()) {
			/* If success, continue. */
			MSSfunction(OMD);
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

