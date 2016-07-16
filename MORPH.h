/*    C:\Program Files\SDCC\include\mcs51\MORPH.h  */

/* 			ADDRESS 			I/O Modules    */
__xdata __at (0xFF00) unsigned char LEDs;
__xdata __at (0xFF01) unsigned char slideButtons;
__xdata __at (0xFF02) unsigned char microButtons;
__xdata __at (0xFE03) unsigned char sevenSegPower;
__xdata __at (0xFE04) unsigned char sevenSeg;


//this function ensures that initialization of data memory does not occour 
unsigned char _sdcc_external_startup (void) __nonbanked{
	return 1;
}

//define the print mode as an accessable address
__xdata __at (0xFB00) unsigned int printMode;

//for printf functionality we will be using the SDCC implementation printf_tiny
#define printf printf_tiny

//This is the putchar function which is invoked by printf functions
void putchar (char c) {
	__xdata __at (0xFBA0) unsigned int charLoc;

	switch (printMode){
		case 0: //output to the MPS PC console

			//MPS console does not respect \n (0x0A) so replace with \r (0x0D) MPS will update also update
			if (c == 0x0A){
				c = 0x0D;
			} else if (c == 0x0D){ //USE \r to tell MPS TO update since it doesn't do anything now that it is replaced with \n
				c = 0x0A;
			}

			charLoc = c; //set the character to an accessable memory address
			/*
			   __asm
			   mov 	dptr, #0xFBA0 //Set the dptr to the temp ram address
			   movx    A,@dptr //Move external data to A
			   mov		r5, a	//Move a to r5
			   ljmp	#0x0709 //PC output takes r5 as input
			   __endasm;*/

			__asm
				mov 	dptr, #0xFBA0 //Set the dptr to the temp ram address
				movx    A,@dptr //Move external data to A
				lcall	#0x301B //PC output takes A as input


				//mov		A,#0xFF //Move external data to A
				//lcall	#0x301B //PC output takes A as input
				//ljmp	#0x6152

				//mov		A,#0x50 //Move external data to A
				//ljmp	#0x301B //PC output takes A as input

				__endasm;


			break;

		case 1: //output to the MORPH LCD
			charLoc = c; //set the character to an accessable memory address

			__asm
				mov 	dptr, #0xFBA0 //Set the dptr to the temp ram address
				ljmp	#0x3012 //LCD output takes dptr as input
				__endasm;
			break;
	}
}

/********** Get Char Code From Console ***************/
char getcharAndEcho(){
	__xdata __at (0x0007) unsigned char charLoc;

	__asm
		lcall	#0x06D0 //gets PC user input from MPS console and save to 0x0007
		__endasm;

	return charLoc;
}


char getchar(){
	__xdata __at (0xFBA0) unsigned char charLoc;

	__asm
		lcall	#0x5BB5 //gets PC user input from MPS console (wait if necessary) and puts it into A (see F189)
		mov 	dptr, #0xFBA0
		movx    @dptr,A
		__endasm;


	return charLoc;
}


char key(){
	//gets PC user input from MPS console don't wait for it though
	__xdata __at (0xFBA0) unsigned char charLoc;

	//check for a key press and set charLoc to 1 if key is pressed
	__asm
		lcall	#0x5BA4 //probably Key(0)
		mov 	dptr, #0xFBA0
		movx    @dptr,A
		__endasm;

	//if key is pressed grab it and set the value to charLoc
	if (charLoc == 1){
		__asm
			lcall	#0x5BB5 //probably Key(1)
			mov 	dptr, #0xFBA0
			movx    @dptr,A
			__endasm;

		return charLoc;
	} else return 0; //if no key pressed return 0
}

void setCur(char xCur, char yCur){
	__xdata __at (0xFBA0) unsigned char xCurLoc;
	__xdata __at (0xFBA1) unsigned char yCurLoc;

	xCurLoc = xCur;
	yCurLoc = yCur;

	// Assembly code to move the cursor to 1,1 (could be usefull later)
	__asm
		mov 	dptr, #0xFBA0 ;set xCur value
		movx    A, @dptr
		mov		R1, A
		mov 	dptr, #0xFBA1 ;set yCur value
		movx    A, @dptr		
		mov		R2, A
		ljmp	#0x5B65
		__endasm;
}

void clrPC(){
	putchar(0x07); //MPS will clear the MPS console output when it recieves the bell character
}

void clrLCD () {
	__asm
		ljmp	#0x300F //Clear the LCD
		__endasm;
}

int readUDCounter(){
	__xdata __at (0x0FF05) int counter;

	__asm
		mov	dptr,#0x0FF05 //enable start read
		mov	A, #0x00
		movx	@dptr,A
		__endasm;

	return(counter);
}

void clrUDCounter(){
	__asm	
		mov dptr, #0x0FF06	//Counter1 _UDCNT1_HIGH
		mov a,#0x00
		movx @dptr,a
		__endasm;	
}

void setSpeed(int speed){
	__xdata __at (0x0FE18) unsigned int highByte; //_DA1_LOW
	highByte = speed - 32768; //the offset is the signed int overflow

	__asm
		mov dptr, #0x0FE1F   ;start conversion DA_CNVT
		movx @dptr,a
		__endasm;
}

void delay(int ms){
	int i, k;
	/*	for (i=0;i < ms;i++){
		TH1 = 0xF9;
		TL1 = 0x8F;//88 too slow 99 too fast
		TMOD |= #0x10;
		TF1 = 0;
		TR1 = 1;
		while (TF1 != 1);
		}*/
	for (i=0; i < ms; i++){
		//nop takes 1 cycle and MORPH is 15142300 Hz
		for(k=0; k < 151; k++){
			__asm
				nop
				__endasm;
		}
	}

}

unsigned char readRAM(unsigned int addr){
	__xdata __at (0xFBA0) unsigned int address;
	__xdata __at (0xFBA2) unsigned char value;	

	address = addr;

	__asm
		mov 	dptr, #0xFBA0; 	set dptr to external ram location lowbyte where the value that is to be read is
		movx    A, @dptr; 		move the address of the data to be read into A
		mov 	r4,a;

	mov 	dptr, #0xFBA1; 	set dptr to external ram location highbyte where the value that is to be read is
		movx    A, @dptr; 		move the address of the data to be read into A
		mov 	r5,a;

	mov 	dpl,r4;
	mov 	dph,r5

		movx 	A, @dptr;

	mov		dptr, #0xFBA2; 	set dptr to output address
		movx    @dptr, A;		move A to the ouput address

		__endasm;


	return value;
}

void allSevenSeg(volatile unsigned char num){
	/*
	   0 -> 01000000
	   1 -> 11111001
	   2 -> 10100100
	   3 -> 10110000
	   4 -> 10011001
	   5 -> 10010010
	   6 -> 10000010
	   7 -> 11111000
	   8 -> 10000000
	   9 -> 10010000
	   */

	switch (num){
		case 0:
			sevenSeg = 0xC0;//40 with dot       C0 without
			break;
		case 1:
			sevenSeg = 0xF9;
			break;
		case 2:
			sevenSeg = 0xA4;
			break;
		case 3:
			sevenSeg = 0xB0;
			break;
		case 4:
			sevenSeg = 0x99;
			break;
		case 5:
			sevenSeg = 0x92;
			break;
		case 6:
			sevenSeg = 0x82;
			break;
		case 7:
			sevenSeg = 0xF8;
			break;
		case 8:
			sevenSeg = 0x80;
			break;
		case 9:
			sevenSeg = 0x90;
			break;
	}
}

void sevenSegDisp(unsigned char num){

	if(num > 10 ){
		sevenSegPower = 0x03;
		allSevenSeg((num/10)%10);
	} else{
		sevenSegPower = 0x07;
		allSevenSeg(num%10);
	}

	/*else if(num < 1000 ){
	  sevenSegPower = 0x01;
	  }else{
	  sevenSegPower = 0x00;
	  }*/



}

void copySDCCInterruptVectorToMonitorInterruptVector(){
	__asm

		mov dptr, #0x8103; External Intterupt 0
		movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

	mov dph, #0x81;
	inc dpl
		movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

	mov dph, #0x81;
	inc dpl		
		movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

	/*
	//BASIC USES ME
	mov dptr, #0x810B; Timer 0 overflow Intterupt
	movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

	mov dph, #0x81;
	inc dpl
	movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

	mov dph, #0x81;
	inc dpl
	movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

*/

	mov dptr, #0x8113; External Intterupt 1
		movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

	mov dph, #0x81;
	inc dpl
		movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

	mov dph, #0x81;
	inc dpl
		movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;		


	mov dptr, #0x811B; Timer 1 overflow Intterupt
		movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

	mov dph, #0x81;
	inc dpl
		movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

	mov dph, #0x81;
	inc dpl
		movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;


	mov dptr, #0x8123; Serial Port Intterupt
		movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

	mov dph, #0x81;
	inc dpl
		movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

	mov dph, #0x81;
	inc dpl
		movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

	/*
	//JM-552 USES ME
	mov dptr, #0x812B; Timer 2 Interrupt
	movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

	mov dph, #0x81;
	inc dpl
	movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;

	mov dph, #0x81;
	inc dpl
	movx 	A, @dptr;
	mov dph, #0xFA;
	movx 	@dptr, A;
	*/

	__endasm;
}

