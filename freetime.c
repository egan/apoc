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
#define FEEDRATE 10	// RPM

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

/*********************************************************/
/***** FTS PROGRAM SECTION *******************************/
/*********************************************************/

/***** START OF INZ **************************************/
/* Machine message field. */
char *machineMessage;
/* Spaces for clearing. */
const char *clrstring;
/* Initial dial position MSD. */
unsigned char initp;
/* Reference encoder position. */
int encoderref;
/* Keep track of HMI state inside of a mode. */
unsigned char submode;
/* Whether to wait at OCS for user acknowledgement. */
unsigned char wait;
/* MSD Ready status: 0 = not ready. */
unsigned char ready;
/* Operational mode, error flag, first run flag. */
unsigned char OMD, ERR1, FFRA;
/* Timer values. */
unsigned char MD, MH, MM, MS;

/* Combination values. */
unsigned char Number1, Number2, Number3;
unsigned char ticks1, ticks2, ticks3;

/* Variable to hold time. */
unsigned int time;
/* Servo and dial parameters. */
unsigned int feedRate;
unsigned int encoderResolution;
unsigned char samplingTime;
unsigned char dialPositions;

/* ASC servo speed. */
__data __at (0x52) unsigned int speed;
/* ASC relative distance (pulses). */
__data __at (0x56) unsigned long distance;
/* ASC direction and go bit. 0th bit is go, 4th bit direction (0 = CW).
 * Conveniently set by: motionRegister = 1 + 0x10*direction;
 * Position achievement signaled by 1 in 1st bit.
 * Conveniently checked by: motionRegister & 0x02;
 */
__data __at (0x21) unsigned char motionRegister;
/* TIM counter: 1 tick = 10ms. */
__data __at (0x23) unsigned char counter;

/* Check user input: if valid store in &setting, otherwise throw error. */
void checkInput(unsigned char input, unsigned char *setting) {
	if (input > 39) {
		/* Invalid input. */
		machineMessage = "Error: Invalid input (outside 0--39)";
		wait = 1;
		/* Get input again. */
		submode--;
	} else {
		/* Store valid setting. */
		*setting = input;
		if (OMD == 1) {
			/* Submode needs to be reset for MSD. */
			submode = 0;
		} else if (OMD == 2) {
			/* Submode needs to be incremented for ACSidle. */
			submode++;
		}
	}
}

/* Function to return current encoder position. */
int getABSposition() {
	__data __at (0x60) unsigned long position;
	return position;
}

/* Function to return current dial position. */
/* XXX: This does not currently work. */
unsigned char currentDial() {
	int absPosition = getABSposition();
	int relPosition = absPosition - encoderref;
	unsigned char dial;
	dial = (initp + relPosition*dialPositions/encoderResolution) % dialPositions;
	return dial;
}

/* Move the servo a certain number of ticks in direction. */
void moveServo(char ticks, char direction) {
	distance = (float)ticks*(float)(encoderResolution/dialPositions);
	motionRegister = 0x01+0x10*direction;
	/* Wait until in position before continuing. */
	while (!(motionRegister & 0x02));
}

void INZfunction() {
	/* Print output to the MPS console. */
	printMode = 0;
	clrstring = "                       ";
	/* Operation Mode is Idle, default submode */
	OMD = 0;
	submode = 0;
	machineMessage = "System Ready";
	/* Level 1 error flag for the system. */
	ERR1 = 0;
	/* Set First Run Flag, MSD defaults. */
	FFRA = 1;
	ready = 0;
	wait = 0;
	initp = 0;
	/* Initialize encoder reference. */
	clrUDCounter();
	encoderref = getABSposition();

	/* Servo/dial parameters. */
	feedRate = FEEDRATE;
	encoderResolution = REVPULSES;
	dialPositions = DIALTICKS;
	samplingTime = SAMPLINGTIME;

	/* Set desired servo speed. */
	motionRegister = 0x00;
	speed = (feedRate*samplingTime*(float)encoderResolution)/60000+0.5;

	/* Solenoid port. */
	P1 = 0x00;

	/* Run the ACS RTS. */
	__asm
		lcall 0x9F00;
	__endasm;
}
/***** END OF INZ ****************************************/

/***** START of MESSAGE AND CONSTANTS DEFINITION *********/
void printHeader() {
	clrPC();
	printf("EME 154 Mechatronics\n");
	printf("Branding\n");
	printf("Team Name\n\n");
}

void printMSD() {
	setCur(0,8);
	/* Update current initial position. */
	printf("Current Initial Position: %u%s\n", initp, clrstring);
	if (submode == 0) {
		/* Display mode. */
		printf("Press E to Change%s\n", clrstring);
		printf("%s%s", clrstring, clrstring);
	} else {
		/* Edit mode. */
		setCur(0,9);
		printf("Enter Initial Value: ");
	}
}

void printACSidle() {
	setCur(0,8);
	/* Sequence of combination number inputs. */
	switch (submode) {
		case 0:
			printf("Press R to Reset Fields%s\n", clrstring);
			printf("Press E to Change 1st Number%s\n", clrstring);
			printf("%s%s\n", clrstring, clrstring);
			printf("%s%s\n", clrstring, clrstring);
			break;
		case 1:
			setCur(0,10);
			printf("Enter 1st Dial Value: ");
			break;
		case 2:
			setCur(0,9);
			printf("1st Dial: %u%s\n", Number1, clrstring);
			printf("Press E to Change 2nd Number%s\n", clrstring);
			break;
		case 3:
			setCur(0,11);
			printf("Enter 2nd Dial Value: ");
			break;
		case 4:
			setCur(0,10);
			printf("2nd Dial: %u%s\n", Number2, clrstring);
			printf("Press E to Change 3rd Number%s\n", clrstring);
			break;
		case 5:
			setCur(0,12);
			printf("Enter 3rd Dial Value: ");
			break;
		case 6:
			setCur(0,11);
			printf("3rd Dial: %u%s\n", Number3, clrstring);
			printf("Press E to Start RTS%s\n", clrstring);
			break;
		case 7:
			break;
		default:
			break;
	}
}

void printACSactive() {
	setCur(0,8);
	printf("%s%s\n", clrstring, clrstring);
	/* Update completion status. */
	switch (submode) {
		case 0:
			printf("1st Dial: %u%s\n", Number1, clrstring);
			printf("2nd Dial: %u%s\n", Number2, clrstring);
			printf("3rd Dial: %u%s\n", Number3, clrstring);
			printf("%s%s\n", clrstring, clrstring);
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
			setCur(0,8);
			printf("Elapsed Time: %d\n", time);
			break;
		default:
			break;
	}
}

void printMOS() {
	/* Display commands. */
	setCur(0,8);
	/* XXX XXX: Does not work with currentDial()! */
	printf("Current Position: %u%s\n", getABSposition(), clrstring);
	printf("Q/W        10 ticks CCW/CW%s\n", clrstring);
	printf("A/S         5 ticks CCW/CW%s\n", clrstring);
	printf("Z/X         1 ticks CCW/CW%s\n", clrstring);
	printf("C/V        40 ticks CCW/CW%s\n", clrstring);
	printf("B             fire solenoid%s\n", clrstring);
}

void printMenu() {
	setCur(0,8);
	/* Display commands. */
	printf("1        Machine Setup Data%s\n", clrstring);
	printf("2        Automatic Operation%s\n", clrstring);
	printf("3        Manual Operation%s\n", clrstring);
	printf("%s%s\n", clrstring, clrstring);
	printf("%s%s\n", clrstring, clrstring);
	printf("%s%s\n", clrstring, clrstring);
}
/***** END of MESSAGE AND CONSTANTS DEFINITION ***********/

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
		case '0':
			/* Exit. */
			OMD = 5;
			machineMessage = "Exit Command Accepted";
			break;
		default:
			break;
	}	
}

void MSSMSD(char selection) {
	/* Edit or quit. */
	switch (selection) {
		case 'E':
			submode = 1;
			break;
		case '0':
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
		case 'R':
			submode = 0;
			break;
		case 'E':
			submode++;
			break;
		case '0':
			OMD = 0;
			break;
		default:
			break;
	}
}

void MSSACSactive(char selection) {
	/* Reset, edit, or quit. */
	switch (selection) {
		case '0':
			OMD = 0;
			break;
		default:
			break;
	}
}

void MSSMOS(char selection) {
	/* Rotate, unlock, or quit. */
	switch (selection) {
		case 'Q':
			/* 10 CCW */
			submode = 1;
			break;
		case 'W':
			/* 10 CW */
			submode = 2;
			break;
		case 'A':
			/* 5 CCW */
			submode = 3;
			break;
		case 'S':
			/* 5 CW */
			submode = 4;
			break;
		case 'Z':
			/* 1 CCW */
			submode = 5;
			break;
		case 'X':
			/* 1 CW */
			submode = 6;
			break;
		case 'C':
			/* 40 CCW */
			submode = 7;
			break;
		case 'V':
			/* 40 CW */
			submode = 8;
			break;
		case 'B':
			/* Solenoid */
			submode = 9;
			break;
		case '0':
			OMD = 0;
			break;
		default:
			submode = 0;
			break;
	}
}

void MSSfunction() {
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
			if (submode == 3) {
				MSSACSactive(selection);
			}
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
		machineMessage = "System Ready";
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
			moveServo(10, 0);
			break;
		case 2:
			/* 10 CW */
			moveServo(10, 1);
			break;
		case 3:
			/* 5 CCW */
			moveServo(5, 0);
			break;
		case 4:
			/* 5 CW */
			moveServo(5, 1);
			break;
		case 5:
			/* 1 CCW */
			moveServo(1, 0);
			break;
		case 6:
			/* 1 CW */
			moveServo(1, 1);
			break;
		case 7:
			/* 40 CCW */
			moveServo(40, 0);
			break;
		case 8:
			/* 40 CW */
			moveServo(40, 1);
			break;
		case 9:
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
	char userinput[5];
	unsigned char parsed;
	
	machineMessage = "Automatic Mode Accepted";
	
	switch (OMD) {
		/* ACS Idle Mode */
		case 2: 
			if(submode == 0) {
				/* Wait for user to press enter to change 1st */
			} else if (submode == 1) {
				/* Receive 1st user input */
				gets(userinput);
				parsed = atoi(userinput);
				checkInput(parsed, &Number1);
			} else if (submode == 2) {
				/* Wait for user to press enter to change 2nd */
			} else if (submode == 3) {
				/* Receive 2nd user input */
				gets(userinput);
				parsed = atoi(userinput);
				checkInput(parsed, &Number2);
			} else if (submode == 4) {
				/* Wait for user to press enter to change 3rd */
			} else if (submode == 5) {
				/* Receive 3rd user input */
				gets(userinput);
				parsed = atoi(userinput);
				checkInput(parsed, &Number3);
			} else if (submode == 6) {
				/* Wait for user to press enter, then Start RTS */
				/* Calculations Here? */
				/* Determine ticks to move to get to 1st Number */
				if (initp >= Number1) {
					ticks1 = 80+(initp-Number1);
				} else if (initp < Number1) {
					ticks1 = 80+initp+(40-Number1);
				}
				/* Determine ticks to move to get to 2nd Number */
				if (Number1 > Number2) {
					ticks2 = 40+(40-(Number1-Number2));
				} else if (Number1 <= Number2) {
					ticks2 = 40+(Number2-Number1);
				}
				/* Determine ticks to move to get to 3rd Number */
				if (Number2 > Number3) {
					ticks3 = Number2-Number3;
				} else if (Number2 <= Number3) {
					ticks3 =  40-(Number3-Number2);
				} 
			} else if (submode == 7) {
				/* Move to Active Mode */
				OMD = 3;
				submode = 0;
			}
			break;
		/* ACS Active Mode */
		case 3: 
			if (submode == 0) {
				/* Clear timer. */
				counter = 0;
				/* Move to Number1. */
				moveServo(ticks1, 1);
				submode++;
			} else if (submode == 1) {
				/* Move to Number2. */
				moveServo(ticks2, 0);
				submode++;
			} else if (submode == 2) {
				/* Move to Number3. */
				moveServo(ticks3, 1);
				/* Record time. */
				time = ((unsigned int) counter)*10;
				submode++;
				/* Fire solenoid. */
				P1 = 0xFF;
			}
			break;
		default:
			break;
	}
} 
/***** END OF ACS ****************************************/


/***** START OF MSD **************************************/
void MSDfunction() {
	char userinput[5];
	unsigned char parsed;

	machineMessage = "Setup Mode Accepted";

	if (submode == 1) {
		/* Go on to print prompt. */
		submode++;
	} else if (submode == 2) {
		/* Receive user input, parse, and check. */
		/* XXX: Why does gets go to weird position? */
		setCur(20,9);
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
		printf("Current Mode: %s%s\n", getCurrentMode(), clrstring);
		printf("%s%s\n", machineMessage, clrstring);
		/* Print mode appropriate HMI. */
		if (OMD == 0) {
			/* Display quitting information and menu. */
			printf("Press 0 to Exit APOC System%s\n", clrstring);
			setCur(0,8);
			printMenu();
		} else {
			/* Display escape information and HMI. */
			printf("Press 0 to Return to Menu%s\n", clrstring);
			switch (OMD) {
				case 1:
					printMSD();
					break;
				case 2:
					printACSidle();
					break;
				case 3:
					printACSactive();
					break;
				case 4:
					printMOS();
					break;
				default:
					break;
			}
		}
	}
	/* Wait for user to acknowledge if necessary. */
	if (wait) {
		while (!key());
		wait = 0;
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

