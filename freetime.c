/*********************************************************/
/******************* FREETIME SYSTEM *********************/
/*********************************************************/
/******************** EME154 UCDAVIS *********************/
/*********************************************************/
#include <8052.h>
#include <stdio.h>
#include <stdlib.h>
#include <MORPH.h>

#define MAXL 6
#define REVPULSES 8192
#define DIALTICKS 40
#define SAMPLINGTIME 10	// ms
#define FEEDRATE = 50	// RPM

/***** FUNCTION PROTOTYPES *******************************/
void OCSfunction();
void MOSfunction();
void ACSfunction();
void MSDfunction();
unsigned char currentDial();

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
	printf("Current Initial Position: %u\t\t\n", initp);
	if (submode == 0) {
		/* Display mode. */
		printf("Press Enter to Change\n");
	} else {
		/* Edit mode. */
		clrLine();
		printf("Enter Initial Value: ");
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
			printf("Enter 1st Dial Value: ");
			break;
		case 2:
			setCur(0,10);
			printf("Press Enter to Change 2nd Number\n");
			break;
		case 3:
			setCur(0,10);
			clrLine();
			printf("Enter 2nd Dial Value: ");
			break;
		case 4:
			setCur(0,11);
			printf("Press Enter to Change 3rd Number\n");
			break;
		case 5:
			setCur(0,11);
			clrLine();
			printf("Enter 3rd Dial Value: ");
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
		printf("Current Position: %u\t\t\n", currentDial());
		printf("Q/W        10 ticks CCW/CW\n");
		printf("A/S         5 ticks CCW/CW\n");
		printf("Z/X         1 ticks CCW/CW\n");
		printf("Bksp/Enter 40 ticks CCW/CW\n");
		printf("Spacebar      fire solenoid\n");
	} else {
		/* Update position. */
		setCur(0,8);
		printf("Current Position: %u\t\t\n", currentDial());
	}
}

void printMenu() {
	setCur(0,8);
	/* Display commands. */
	printf("1        Machine Setup Data\n");
	printf("2        Automatic Operation\n");
	printf("3        Manual Operation\n");
}
/***** END of MESSAGE AND CONSTANTS DEFINITION ***********/

/*********************************************************/
/***** FTS PROGRAM SECTION *******************************/
/*********************************************************/

/***** START OF INZ **************************************/
/* Machine message field. */
char *machineMessage;
/* Initial dial position MSD. */
unsigned char initp;
/* Reference encoder position. */
int encoderref;
/* Keep track of HMI state inside of a mode. */
unsigned char submode;
/* MSD Ready status: 0 = not ready. */
unsigned char ready;
/* Operational mode, error flag, first run flag. */
unsigned char OMD, ERR1, FFRA;
/* Timer values. */
unsigned char MD, MH, MM, MS;

/* ASC servo speed. XXX: Units/desired value? */
__data __at (0x52) unsigned int servospeed;
/* ASC relative distance (pulses). */
__data __at (0x56) unsigned long servopulses;
/* ASC direction and go bit. 0th bit is go, 1st bit direction (0 = CW).
 * Conveniently set by: motionRegister = 1 + 0x10*direction;
 * Position achievement signaled by 1 in 1st bit after go signal.
 * Conveniently checked by: motionRegister & 0x10;
 */
__data __at (0x21) unsigned char motionRegister;

/* Check user input: if valid store in &setting, otherwise throw error. */
void checkInput(unsigned char input, unsigned char *setting) {
	/* XXX: Other conditions? */
	if (input > 39) {
		/* Invalid input. */
		machineMessage = "Error: Invalid input (outside 0--39)";
		/* Get input again. */
		submode--;
	} else {
		/* Store valid setting. */
		*setting = input;
	}
}

/* Function to return current encoder position. */
int getABSposition() {
	__data __at (0x60) unsigned long position;
	return position;
}

/* Function to return current dial position. */
unsigned char currentDial() {
	int absPosition = getABSposition();
	int relPosition = absPosition - encoderref;
	unsigned char dial;
	dial = (initp + relPosition*DIALTICKS/REVPULSES) % DIALTICKS;
	return dial;
}

/* Convert dial ticks into encoder pulses. */
unsigned int ticks2pulses(unsigned char ticks) {
	unsigned int pulses;
	pulses = ticks*REVPULSES/DIALTICKS;
	return pulses;
}

/* Move the servo a certain number of ticks in direction. */
void moveServo(unsigned char ticks, char direction) {
	servopulses = ticks2pulses(ticks);
	motionRegister = 0x01+0x10*direction;
}

void INZfunction() {
	/* Print output to the MPS console. */
	printMode = 0;
	/* Operation Mode is Idle, default submode */
	OMD = 0;
	submode = 0;
	machineMessage = "Ready";
	/* Level 1 error flag for the system. */
	ERR1 = 0;
	/* Set First Run Flag, MSD defaults. */
	FFRA = 1;
	ready = 0;
	initp = 0;
	/* Initialize the time variables. */
	MD = MH = MM = MS = 0;
	clrUDCounter();
	/* Initialize encoder reference. */
	encoderref = getABSposition();

	/* Set desired servo speed. */
	motionRegister = 0x00;
	servospeed = (FEEDRATE*SAMPLINGTIME*(float)REVPULSES)/60000+0.5;

	/* Solenoid port. */
	P1 = 0x00;

	/* Run the ACS RTS. */
	__asm
		lcall 0x9F00;
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
			encoderref = getABSposition();
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

void MSSMOS(char selection) {
	/* Rotate, unlock, or quit. */
	switch (selection) {
		case 'q':
			/* 10 CCW */
			submode = 1;
			break;
		case 'w':
			/* 10 CW */
			submode = 2;
			break;
		case 'a':
			/* 5 CCW */
			submode = 3;
			break;
		case 's':
			/* 5 CW */
			submode = 4;
			break;
		case 'z':
			/* 1 CCW */
			submode = 5;
			break;
		case 'x':
			/* 1 CW */
			submode = 6;
			break;
		case 0x08:	// BS.
			/* 40 CCW */
			submode = 7;
			break;
		case 0x0D:	// CR.
			/* 40 CW */
			submode = 8;
			break;
		case 0x20:	// Space.
			/* Solenoid */
			submode = 9;
			break;
		case 0x03:	// ^C.
			OMD = 0;
			break;
		default:
			submode = 0;
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
		/* Release solenoid. */
		P1 = 0x00;
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
	switch (submode) {
		case 1:
			/* 10 CCW */
			moveServo(10, 1);
			break;
		case 2:
			/* 10 CW */
			moveServo(10, 0);
			break;
		case 3:
			/* 5 CCW */
			moveServo(5, 1);
			break;
		case 4:
			/* 5 CW */
			moveServo(5, 0);
			break;
		case 5:
			/* 1 CCW */
			moveServo(1, 1);
			break;
		case 6:
			/* 1 CW */
			moveServo(1, 1);
			break;
		case 7:	// BS.
			/* 40 CCW */
			moveServo(40, 1);
			break;
		case 8:	// CR.
			/* 40 CW */
			moveServo(40, 0);
			break;
		case 9:	// Space.
			/* Solenoid */
			P1 = 0xFF;
			break;
		default:
			break;
	}
}
/***** END OF MOS ****************************************/

/***** START OF ACS **************************************/
void ACSfunction() {
	machineMessage = "Automatic Mode Accepted";

	/* Count time properly. */
	if (MH > 23) {
		MD++;
		MH = 0;
	}
	if (MM > 59) {
		MH++;
		MM = 0;
	}
	if (MS > 59) {
		MM++;
		MS = 0;
	}
	/* 0xFF is the initial. value and 50*20ms is 1s. */
	if (counter < (0xFF-50)) {
		/* Prevent counter overflow, count time instead. */
		MS++;
		counter = 0xFF;
	}
}
/***** END OF ACS ****************************************/


/***** START OF MSD **************************************/
void MSDfunction() {
	/* XXX: Is this dangerous? */
	char *userinput;
	unsigned char parsed;

	machineMessage = "Setup Mode Accepted";

	if (submode == 1) {
		/* Go on to print prompt. */
		submode++;
	} else if (submode == 2) {
		/* Receive user input, parse, and check. */
		gets(userinput);
		/* Attempts to cast no matter what:
		 * there is no detection of obviously erroneous
		 * input like letters, or negative numbers.
		 */
		parsed = atoi(userinput);
		checkInput(parsed, &initp);
	}
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

