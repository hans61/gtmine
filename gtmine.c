#include <stdlib.h>
#include <string.h>
#include <gigatron/console.h>
#include <gigatron/sys.h>
#include <gigatron/libc.h>
#include <stdarg.h>

#define FGBG 0x3f38

#define MAXX 26
#define MAXY 17

// Game controller bits (actual controllers in kit have negative output)
// +----------------------------------------+
// |       Up                        B*     |
// |  Left    Right               B     A*  |
// |      Down      Select Start     A      |
// +----------------------------------------+ *=Auto fire
#define BUTTON_RIGHT  0xfe
#define BUTTON_LEFT   0xfd
#define BUTTON_DOWN   0xfb
#define BUTTON_UP     0xf7
#define BUTTON_START  0xef
#define BUTTON_SELECT 0xdf
#define BUTTON_B      0xbf
#define BUTTON_A      0x7f

// length of the queue for automatic uncovering of game fields.
// It is an alternative to a recursive function.
// I am afraid of stack problems with recursive functions.
#define MAXQ 50

#define SFREE 0
#define S1 1
#define S2 2
#define S3 3
#define S4 4
#define S5 5
#define S6 6
#define S7 7
#define S8 8
#define SBOMB 9
#define SBOMBTRIGGERED 10
#define SCURSOR 11
#define SHIDDEN 12
#define SMARKER 13

#define BHIDDEN 0x10
#define BMARKER 0x20

const char sfree[]={44,44,44,44,44,46,44,44,44,44,44,46,44,44,44,44,44,46,44,44,44,44,44,46,44,44,44,44,44,46,46,46,46,46,46,46,250};           // 0
const char s1[]={44,44,48,48,44,46,44,44,44,48,44,46,44,44,44,48,44,46,44,44,44,48,44,46,44,44,44,48,44,46,46,46,46,46,46,46,250};              // 1
const char s2[]={44,8,8,8,44,46,44,44,44,44,8,46,44,44,8,8,44,46,44,8,44,44,44,46,44,8,8,8,8,46,46,46,46,46,46,46,250};                         // 2
const char s3[]={44,35,35,35,44,46,44,44,44,44,35,46,44,44,35,35,44,46,44,44,44,44,35,46,44,35,35,35,44,46,46,46,46,46,46,46,250};              // 3
const char s4[]={44,33,44,44,44,46,44,33,44,44,44,46,44,33,44,33,44,46,44,33,33,33,33,46,44,44,44,33,44,46,46,46,46,46,46,46,250};              // 4
const char s5[]={44,6,6,6,6,46,44,6,44,44,44,46,44,44,6,6,44,46,44,44,44,44,6,46,44,6,6,6,44,46,46,46,46,46,46,46,250};                         // 5
const char s6[]={44,44,57,57,44,46,44,57,44,44,44,46,44,57,57,57,44,46,44,57,44,44,57,46,44,44,57,57,44,46,46,46,46,46,46,46,250};              // 6
const char s7[]={44,16,16,16,16,46,44,44,44,44,16,46,44,44,44,16,44,46,44,44,16,44,44,46,44,44,16,44,44,46,46,46,46,46,46,46,250};              // 7
const char s8[]={44,44,37,37,44,46,44,37,44,44,37,46,44,44,37,37,44,46,44,37,44,44,37,46,44,44,37,37,44,46,46,46,46,46,46,46,250};              // 8
const char sbomb[]={16,44,16,44,16,46,44,61,16,16,44,46,16,16,16,16,16,46,44,16,16,16,44,46,16,44,16,44,16,46,46,46,46,46,46,46,250};           // 9
const char sbombtriggered[]={16,19,16,19,16,19,19,62,16,16,19,19,16,16,16,16,16,19,19,16,16,16,19,19,16,19,16,19,16,19,19,19,19,19,19,19,250};  // 10 [0x0a]
const char scursor[]={35,35,0,0,35,35,35,0,0,0,0,35,0,0,0,0,0,0,0,0,0,0,0,0,35,0,0,0,0,35,35,35,0,0,35,35,250};                                 // 11 [0x0b]
const char shidden[]={58,58,58,58,58,50,58,58,58,58,58,50,58,58,58,58,58,50,58,58,58,58,58,50,58,58,58,58,58,50,50,50,50,50,50,50,250};         // 12 [0x0c]
const char smarker[]={58,58,19,19,58,50,58,19,19,19,58,50,58,58,19,19,58,50,58,58,58,1,58,50,58,58,1,1,1,50,50,50,50,50,50,50,250};             // 13 [0x0d]


char leftMargin;
char topMargin;

void mySpritet(char *addr, char *dest){ // draws sprite with transparencursorY for color 0
    int i,z,v;
    z = 0;
    v = 0;
    i = 0;
    while(addr[v]<128){
        if(addr[v]>0) dest[z + i] = addr[v];
        v++; i++;
        if(i > 5){
            i = 0;
            z += 256;
        }
    }

}

void printSprite(int val, int xx, int yy) // val is the id of the sprite, xx,yy is the x,y position in the playfield
{
	char* ptrChar;
	int sprnum;
	sprnum = val & 0x0F;
	if(val >= BHIDDEN) sprnum = SHIDDEN;
	if(val >= BMARKER) sprnum = SMARKER;
	ptrChar = (char*)scursor;		
	switch(sprnum){
        case SFREE:
            ptrChar = (char*)sfree;
            break;
        case S1:
            ptrChar = (char*)s1;
            break;
        case S2:
            ptrChar = (char*)s2;
            break;
        case S3:
            ptrChar = (char*)s3;
            break;
        case S4:
            ptrChar = (char*)s4;
            break;
        case S5:
            ptrChar = (char*)s5;
            break;
        case S6:
            ptrChar = (char*)s6;
            break;
        case S7:
            ptrChar = (char*)s7;
            break;
        case S8:
            ptrChar = (char*)s8;
            break;
        case SBOMB:
            ptrChar = (char*)sbomb;
            break;
        case SBOMBTRIGGERED:
            ptrChar = (char*)sbombtriggered;
            break;
        case SCURSOR:
            ptrChar = (char*)scursor;
            break;
        case SHIDDEN:
            ptrChar = (char*)shidden;
            break;
        case SMARKER:
            ptrChar = (char*)smarker;
            break;
	}
	SYS_Sprite6_v3(ptrChar, (char*)(yy*6+topMargin<<8)+6*xx+leftMargin);
}

int main()
{
    
    unsigned int ticks;
    int i, x, y, x1, y1, tx, ty; // help variables
    char fieldsX;             	 // width of the playing field
    char fieldsY;             	 // height of the playing field
    char numberBomb;          	 // number of bombs
	char gameOver;	             // flag, end of game reached
	char newGame;	 			 // Flag, start new game without closing the old one
	char firstClick;             // Flag for start of the clock
	unsigned int seconds;        // elapsed seconds
    char cursorX, cursorY;       // cursor in the playing field	 
	char markerCount;         	 // counter for marked fields
	char revealedFields;      	 // counter for revealed fields
	unsigned int queue[MAXQ];    // queue for automatic uncovering of game fields
	char queuePointer;           // pointer to queue
    char field[MAXY][MAXX];  	 // byte array for playing field, lower nibble sprite id, upper nibble flags
    
	numberBomb = 10;
	fieldsX = 9;
	fieldsY = 9;
	topMargin = 35;

	SYS_SetMode(1);

	for(;;){
		SYS_SetMode(1975);        // faster calculation of the playing field
	
		leftMargin = (160 - 6*fieldsX)/2;
        // output top line		
		_console_reset(FGBG);
		_console_clear((char*)(8<<8), 0x030A, 16);
		_console_printchars(0x030A, (char*)(8<<8)+6*1, "B", 1);
		_console_printchars(0x200A, (char*)(8<<8)+6*2, "eginner", 7);
		_console_printchars(0x030A, (char*)(8<<8)+6*10, "A", 1);
		_console_printchars(0x200A, (char*)(8<<8)+6*11, "dvanced", 7);
		_console_printchars(0x030A, (char*)(8<<8)+6*19, "E", 1);
		_console_printchars(0x200A, (char*)(8<<8)+6*20, "xpert", 5);
		
		console_state.fgbg = 0x030A;

		markerCount = 0;
		gameOver = 0;
		newGame = 0;
		firstClick = 0;
		seconds = 0;
		revealedFields = 0;
	
		for( y=0; y<fieldsY; y++ ){
			for( x=0; x<fieldsX; x++ ){
				// field[y][x] = SFREE;
				field[y][x] = BHIDDEN;
				printSprite(field[y][x], x, y);
			}
		}
	
		i = 0; // bomb counter temp
		// setting the bombs in the field
		while(i < numberBomb){
			x = rand() % (fieldsX-1);
			y = rand() % (fieldsY-1);
			if(field[y][x] != (SBOMB | BHIDDEN)){ // field is not a bomb, bomb set
				i++;                              // add bomb
				field[y][x] = SBOMB | BHIDDEN;    // set marker for bomb
			}
		}
	
		for( y=0; y<fieldsY; y++ ){
			for( x=0; x<fieldsX; x++ ){
				
				// count neighboring bombs
				if(field[y][x] != (SBOMB | BHIDDEN)){
					// observe edges
					if(x < fieldsX-1 ){ // right edge
						if(field[y][x+1] == (SBOMB | BHIDDEN)) field[y][x]++;                      // right
						if(y < fieldsY-1 ) if(field[y+1][x+1] == (SBOMB | BHIDDEN)) field[y][x]++; // bottom right
						if(y > 0 ) if(field[y-1][x+1] == (SBOMB | BHIDDEN)) field[y][x]++;         // top right
					}
					if(x > 0 ){ // left edge
						if(field[y][x-1] == (SBOMB | BHIDDEN)) field[y][x]++;                      // left
						if(y < fieldsY-1 ) if(field[y+1][x-1] == (SBOMB | BHIDDEN)) field[y][x]++; // bottom left
						if(y > 0 ) if(field[y-1][x-1] == (SBOMB | BHIDDEN)) field[y][x]++;         // top left
					}
					if(y < fieldsY-1 ){
						if(field[y+1][x] == (SBOMB | BHIDDEN)) field[y][x]++;                      // bottom
					}
					if(y > 0 ) if(field[y-1][x] == (SBOMB | BHIDDEN)) field[y][x]++;               // top
				}
				
			}
		}
	
		SYS_SetMode(-1);   // return to the original video mode
	
		cursorX = 0;
		cursorY = 0;
		
		mySpritet((char*)scursor, (char*)(cursorY*6+topMargin<<8)+6*cursorX+leftMargin );
	
		while(!gameOver){
			
			switch(buttonState) {
			//switch(buttonRaw) {
				case BUTTON_DOWN: // down
					if(cursorY < fieldsY-1){
						printSprite((field[cursorY][cursorX]), cursorX, cursorY);
						cursorY++;
						mySpritet((char*)scursor, (char*)(cursorY*6+topMargin<<8)+6*cursorX+leftMargin );
					}
				buttonState = 0xff;
				break;
				case BUTTON_UP: // up
					if(cursorY > 0){
						printSprite((field[cursorY][cursorX]), cursorX, cursorY);
						cursorY--;
						mySpritet((char*)scursor, (char*)(cursorY*6+topMargin<<8)+6*cursorX+leftMargin );
					}
				buttonState = 0xff;
				break;
				case BUTTON_LEFT: // left
					if(cursorX > 0){
						printSprite((field[cursorY][cursorX]), cursorX, cursorY);
						cursorX--;
						mySpritet((char*)scursor, (char*)(cursorY*6+topMargin<<8)+6*cursorX+leftMargin );
					}
				buttonState = 0xff;
				break;
				case BUTTON_RIGHT: // right
					// display for debugging
					if(cursorX < fieldsX-1){
						printSprite((field[cursorY][cursorX]), cursorX, cursorY);
						cursorX++;
						mySpritet((char*)scursor, (char*)(cursorY*6+topMargin<<8)+6*cursorX+leftMargin );
					}
				buttonState = 0xff;
				break;

				// case BUTTON_START:  // blocked software reset
				case 'n': // start new game
				case 'N':
					gameOver = 1;
					newGame = 1;
				buttonState = 0xff;
				break;

				case 'b': // new game beginner
				case 'B':
					gameOver = 1;
					newGame = 1;
					numberBomb = 10;
					fieldsX = 9;
					fieldsY = 9;
					topMargin = 35;
				buttonState = 0xff;
				break;
				case 'a': // new game advanced
				case 'A':
					gameOver = 1;
					newGame = 1;
					numberBomb = 40;
					fieldsX = 16;
					fieldsY = 16;
					topMargin = 28;
				buttonState = 0xff;
				break;

				case 'e': // new game expert
				case 'E':
					gameOver = 1;
					newGame = 1;
					numberBomb = 88;
					fieldsX = 26;
					fieldsY = 17;
					topMargin = 25;
				buttonState = 0xff;
				break;

				case 'd': // debug show field, uncover
				case 'D':
					for( y=0; y<fieldsY; y++ ){
						for( x=0; x<fieldsX; x++ ){
							printSprite((field[y][x] & 0x0F), x, y);
						}
					}
					gameOver = 1;
				buttonState = 0xff;
				break;

				case BUTTON_B:
				case 0x20: // set,unset marker with space
					if((field[cursorY][cursorX] & BHIDDEN) == BHIDDEN){      // only on covered fields
						if((field[cursorY][cursorX] & BMARKER) == BMARKER){
							field[cursorY][cursorX] = field[cursorY][cursorX] & 0x1F;
							markerCount--;
						}else{
						    if(markerCount<numberBomb){                                // only so many as bombs
								field[cursorY][cursorX] = field[cursorY][cursorX] | 0x20;
								markerCount++;
							}
						}
						printSprite((field[cursorY][cursorX]), cursorX, cursorY);
						mySpritet((char*)scursor, (char*)(cursorY*6+topMargin<<8)+6*cursorX+leftMargin );
					}
				buttonState = 0xff;
				break;

				case BUTTON_A:
				case 0x0A: // uncover game field with enter key
					if(field[cursorY][cursorX] < 0x10) continue;
					if(firstClick == 0){
						firstClick = 1;
						ticks = _clock();
					}
					if(field[cursorY][cursorX] < 0x20){              // marker protects field
						if((field[cursorY][cursorX] & 0x0F) == SBOMB){
							// game over
							gameOver = 1;
							field[cursorY][cursorX] = SBOMBTRIGGERED;
							for( y=0; y<fieldsY; y++ ){              // uncover all hidden fields
								for( x=0; x<fieldsX; x++ ){
									printSprite((field[y][x] & 0x0F), x, y);
								}
							}
						}else{ // no bomb in the field
						    if(field[ty][tx] > 0x1F) markerCount--;  // remove incorrect marker
							field[cursorY][cursorX] = field[cursorY][cursorX]& 0x0F;
							printSprite(field[cursorY][cursorX], cursorX, cursorY);
							revealedFields++;
							if(field[cursorY][cursorX] == SFREE) {
								// field is empty, adjacent fields can be uncovered
								queuePointer = 0;
								queue[queuePointer] = (cursorY<<8) + cursorX;
								queuePointer++;
								while(queuePointer>0){
									queuePointer--;
									ty = queue[queuePointer]>>8;
									tx = queue[queuePointer] & 0xFF;
									if(field[ty][tx] > 0x0F){
										if(field[ty][tx] > 0x1F) markerCount--;  // remove incorrect marker

										revealedFields++;
										field[ty][tx] = field[ty][tx] & 0x0F;
										printSprite(field[ty][tx], tx, ty);
									}
									// search neighboring fields
									for(y = -1; y < 2; y++){
										for(x = -1; x < 2; x++){
											// loop adjacent fields
											x1 = tx + x; y1 = ty + y;
											if((x1 < fieldsX) && (x1 >= 0) && (y1 < fieldsY) && (y1 >= 0) && (field[y1][x1]>0x0F)){
												// field lies in the array and is not uncovered
												if(field[y1][x1] > 0x1F) markerCount--;  // remove incorrect marker
												field[y1][x1] = field[y1][x1] & 0x0F;    // uncover field
												printSprite(field[y1][x1], x1, y1);      // draw revealed field
												revealedFields++;
												if(field[y1][x1] == SFREE){              // field has no neighbor bombs, add to queue
													queue[queuePointer] = (y1<<8) + x1;
													queuePointer++;
												}
											}
										}
									}
								}
							}
						}
						mySpritet((char*)scursor, (char*)(cursorY*6+topMargin<<8)+6*cursorX+leftMargin);
					}
				buttonState = 0xff;
				break;
			}

			if(firstClick) seconds = (_clock() - ticks)/60;

    		_console_printchars(0x020A, (char*)(8*2<<8)+6*1, "Bombs", 5);
			console_state.fgbg = 0x020A;
			console_state.cy = 1;
			console_state.cx = 7;
        	cprintf("%2d", numberBomb - markerCount);
			console_state.cx = 21;
		    cprintf("%4d", seconds);

			if((revealedFields+numberBomb)==(fieldsX*fieldsY)) gameOver = 1;
		}
		// game end
		if(!newGame){
			if((revealedFields+numberBomb)==(fieldsX*fieldsY)){
				_console_clear((char*)(8<<8), 0x01c, 16);
				_console_printchars(0x031c, (char*)(8+8*0<<8)+6*3, "YOU are the winner!", 19);
				_console_printchars(0x031c, (char*)(8+8*1<<8)+6*1, "Hit any key for new game", 24);
			}else{
				_console_clear((char*)(8<<8), 0x0f03, 16);
				_console_printchars(0x0f03, (char*)(8+8*0<<8)+6*2, ">>> You have lost <<<", 21);
				_console_printchars(0x0f03, (char*)(8+8*1<<8)+6*1, "Hit any key for new game", 24);
			}

			while(serialRaw == 0xFF) {}
			while(serialRaw != 0xFF) {}
		}
	}
    //return 0;
}
