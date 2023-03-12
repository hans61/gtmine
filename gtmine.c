#include <stdlib.h>
#include <string.h>
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

static char sfree[]={44,44,44,44,44,46,44,44,44,44,44,46,44,44,44,44,44,46,44,44,44,44,44,46,44,44,44,44,44,46,46,46,46,46,46,46,250};           // 0
static char s1[]={44,44,48,48,44,46,44,44,44,48,44,46,44,44,44,48,44,46,44,44,44,48,44,46,44,44,44,48,44,46,46,46,46,46,46,46,250};              // 1
static char s2[]={44,8,8,8,44,46,44,44,44,44,8,46,44,44,8,8,44,46,44,8,44,44,44,46,44,8,8,8,8,46,46,46,46,46,46,46,250};                         // 2
static char s3[]={44,35,35,35,44,46,44,44,44,44,35,46,44,44,35,35,44,46,44,44,44,44,35,46,44,35,35,35,44,46,46,46,46,46,46,46,250};              // 3
static char s4[]={44,33,44,44,44,46,44,33,44,44,44,46,44,33,44,33,44,46,44,33,33,33,33,46,44,44,44,33,44,46,46,46,46,46,46,46,250};              // 4
static char s5[]={44,6,6,6,6,46,44,6,44,44,44,46,44,44,6,6,44,46,44,44,44,44,6,46,44,6,6,6,44,46,46,46,46,46,46,46,250};                         // 5
static char s6[]={44,44,57,57,44,46,44,57,44,44,44,46,44,57,57,57,44,46,44,57,44,44,57,46,44,44,57,57,44,46,46,46,46,46,46,46,250};              // 6
static char s7[]={44,16,16,16,16,46,44,44,44,44,16,46,44,44,44,16,44,46,44,44,16,44,44,46,44,44,16,44,44,46,46,46,46,46,46,46,250};              // 7
static char s8[]={44,44,37,37,44,46,44,37,44,44,37,46,44,44,37,37,44,46,44,37,44,44,37,46,44,44,37,37,44,46,46,46,46,46,46,46,250};              // 8
static char sbomb[]={16,44,16,44,16,46,44,61,16,16,44,46,16,16,16,16,16,46,44,16,16,16,44,46,16,44,16,44,16,46,46,46,46,46,46,46,250};           // 9
static char sbombtriggered[]={16,19,16,19,16,19,19,62,16,16,19,19,16,16,16,16,16,19,19,16,16,16,19,19,16,19,16,19,16,19,19,19,19,19,19,19,250};  // 10 [0x0a]
static char scursor[]={35,35,0,0,35,35,35,0,0,0,0,35,0,0,0,0,0,0,0,0,0,0,0,0,35,0,0,0,0,35,35,35,0,0,35,35,250};                                 // 11 [0x0b]
static char shidden[]={58,58,58,58,58,50,58,58,58,58,58,50,58,58,58,58,58,50,58,58,58,58,58,50,58,58,58,58,58,50,50,50,50,50,50,50,250};         // 12 [0x0c]
static char smarker[]={58,58,19,19,58,50,58,19,19,19,58,50,58,58,19,19,58,50,58,58,58,1,58,50,58,58,1,1,1,50,50,50,50,50,50,50,250};             // 13 [0x0d]



// print code borrowed from gigatron-lcc/stuff/tst/TSTmemcpyext.c
typedef struct {
	char *addr;
	char x;
	char y;
} screenpos_t;

int leftMargin;
int fgbg;
fgbg = FGBG;
char topMargin;

void clear_lines(int l1, int l2)
{
	int i;
	for (i=l1; i<l2; i++) {
		char *row = (char*)(videoTable[i+i]<<8);
		memset(row, fgbg & 0xff, 160);
	}
}

void clear_screen(screenpos_t *pos)
{
	int i;
	for (i=0; i<120; i++) {
		videoTable[i+i] = 8 + i;
		videoTable[i+i+1] = 0;
	}
	clear_lines(0,120);
	pos->x = pos->y = 0;
	pos->addr = (char*)(videoTable[0]<<8);
}

void scroll(void)
{
	char pages[8];
	int i;
	for (i=0; i<8; i++)
		pages[i] = videoTable[i+i];
	for (i=0; i<112; i++)
		videoTable[i+i] = videoTable[i+i+16];
	for (i=112; i<120; i++)
		videoTable[i+i] = pages[i-112];
}

void newline(screenpos_t *pos)
{
	pos->x = 0;
	pos->y += 1;
	if (pos->y >  14) {
		scroll();
		clear_lines(112,120);
		pos->y = 14;
	}
	pos->addr = (char*)(videoTable[16*pos->y]<<8);
}

void print_char(screenpos_t *pos, int ch)
{
	unsigned int fntp;
	char *addr;
	int i;
	if (ch < 32) {
		if (ch == '\n') 
			newline(pos);
		return;
	} else if (ch < 82) {
		fntp = font32up + 5 * (ch - 32);
	} else if (ch < 132) {
		fntp = font82up + 5 * (ch - 82);
	} else {
		return;
	}
	addr = pos->addr;
	for (i=0; i<5; i++) {
		SYS_VDrawBits(fgbg, SYS_Lup(fntp), addr);
		addr += 1;
		fntp += 1;
	}
	pos->x += 1;
	pos->addr = addr + 1;
	if (pos->x > 24)
		newline(pos);
}

screenpos_t pos;

void print_unsigned(unsigned int n, int radix)
{
	static char digit[] = "0123456789abcdef";
	char buffer[8];
	char *s = buffer;
	do {
		*s++ = digit[n % radix];
		n = n / radix;
	} while (n);
	while (s > buffer)
		print_char(&pos, *--s);
}

void print_int(int n, int radix)
{
	if (n < 0) {
		print_char(&pos, '-');
		n = -n;
	}
	print_unsigned(n, radix);
}

int myprintf(const char *fmt, ...)
{
	char c;
	va_list ap;
	va_start(ap, fmt);
	while (c = *fmt++) {
		if (c != '%') {
			print_char(&pos, c);
			continue;
		}
		if (c = *fmt++) {
			if (c == 'd')
				print_int(va_arg(ap, int), 10);
			else if (c == 'u')
				print_unsigned(va_arg(ap, unsigned), 10);
			else if (c == 'x')
				print_unsigned(va_arg(ap, unsigned), 16);
			else
				print_char(&pos, c);
		}
	}
	va_end(ap);
	return 0;
}
// end of print code
void mySprite(char *addr, char *dest){ // draws sprite like sys function
    int i,z,v;
    z = 0;
    v = 0;
    i = 0;
    while(addr[v]<128){
        dest[z + i] = addr[v];
        v++; i++;
        if(i > 5){
            i = 0;
            z += 256;
        }
    }

}
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
	SYS_Sprite6(ptrChar, (char*)(yy*6+topMargin<<8)+6*xx+leftMargin);
	//mySprite(ptrChar, (char*)(yy*6+topMargin<<8)+6*xx+leftMargin);
}

int main()
{
    
    unsigned int ticks;
    int i, x, y, x1, y1, tx, ty; // help variables
    char fieldsx;             	 // width of the playing field
    char fieldsy;             	 // height of the playing field
    char numberbomb;          	 // number of bombs
	char gameOver;	             // flag, end of game reached
	char newGame;	 			 // Flag, start new game without closing the old one
	char firstClick;             // Flag for start of the clock
	unsigned int seconds;        // elapsed seconds
    char cursorX, cursorY;       // cursor in the playing field	 
	char markerCount;         	 // counter for marked fields
	char revealedFields;      	 // counter for revealed fields
	unsigned int queue[MAXQ];    // queue for automatic uncovering of game fields
	char qptr;                	 // pointer to queue
    char field[MAXY][MAXX];  	 // byte array for playing field, lower nibble sprite id, upper nibble flags
    
	numberbomb = 10;
	fieldsx = 9;
	fieldsy = 9;
	topMargin = 35;

	SYS_SetMode(1);

	while(1){
	
    	SYS_SetMode(1975);

		leftMargin = (160 - 6*fieldsx)/2;
		
		fgbg = FGBG;
		clear_screen(&pos);
		
		fgbg = 0x030A;
		clear_lines(0,16);
		pos.x = 1; pos.y = 0;
		pos.addr = (char*)(videoTable[16*pos.y]<<8)+6*pos.x;
		myprintf("B");
		fgbg = 0x200A;
		myprintf("eginner ");
		fgbg = 0x030A;
		myprintf("A");
		fgbg = 0x200A;
		myprintf("dvanced ");
		fgbg = 0x030A;
		myprintf("E");
		fgbg = 0x200A;
		myprintf("xpert");
		
		markerCount = 0;
		gameOver = 0;
		newGame = 0;
		firstClick = 0;
		seconds = 0;
		// qmax = 0;
		revealedFields = 0;
	
		for( y=0; y<fieldsy; y++ ){
			for( x=0; x<fieldsx; x++ ){
				// field[y][x] = SFREE;
				field[y][x] = BHIDDEN;
				printSprite(field[y][x], x, y);
			}
		}
	
		i = 0; // bomb counter temp
		// setting the bombs in the field
		while(i < numberbomb){
			x = rand() % (fieldsx-1);
			y = rand() % (fieldsy-1);
			if(field[y][x] != (SBOMB | BHIDDEN)){ // field is not a bomb, bomb set
				i++;                              // add bomb
				field[y][x] = SBOMB | BHIDDEN;    // set marker for bomb
			}
		}
	
		for( y=0; y<fieldsy; y++ ){
			for( x=0; x<fieldsx; x++ ){
				
				// count neighboring bombs
				if(field[y][x] != (SBOMB | BHIDDEN)){
					// observe edges
					if(x < fieldsx-1 ){ // right edge
						if(field[y][x+1] == (SBOMB | BHIDDEN)) field[y][x]++;                      // right
						if(y < fieldsy-1 ) if(field[y+1][x+1] == (SBOMB | BHIDDEN)) field[y][x]++; // bottom right
						if(y > 0 ) if(field[y-1][x+1] == (SBOMB | BHIDDEN)) field[y][x]++;         // top right
					}
					if(x > 0 ){ // left edge
						if(field[y][x-1] == (SBOMB | BHIDDEN)) field[y][x]++;                      // left
						if(y < fieldsy-1 ) if(field[y+1][x-1] == (SBOMB | BHIDDEN)) field[y][x]++; // bottom left
						if(y > 0 ) if(field[y-1][x-1] == (SBOMB | BHIDDEN)) field[y][x]++;         // top left
					}
					if(y < fieldsy-1 ){
						if(field[y+1][x] == (SBOMB | BHIDDEN)) field[y][x]++;                      // bottom
					}
					if(y > 0 ) if(field[y-1][x] == (SBOMB | BHIDDEN)) field[y][x]++;               // top
				}
				
			}
		}
	
		SYS_SetMode(-1);
	
		cursorX = 0;
		cursorY = 0;
		
		mySpritet((char*)scursor, (char*)(cursorY*6+topMargin<<8)+6*cursorX+leftMargin );
	
		while(!gameOver){
			
			switch(buttonState) {
			//switch(buttonRaw) {
				case BUTTON_DOWN: // down
					if(cursorY < fieldsy-1){
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
					if(cursorX < fieldsx-1){
						printSprite((field[cursorY][cursorX]), cursorX, cursorY);
						cursorX++;
						mySpritet((char*)scursor, (char*)(cursorY*6+topMargin<<8)+6*cursorX+leftMargin );
					}
					buttonState = 0xff;
				break;
				case BUTTON_B:
				case 0x20: // set,unset marker with space
					if((field[cursorY][cursorX] & BHIDDEN) == BHIDDEN){      // only on covered fields
						if((field[cursorY][cursorX] & BMARKER) == BMARKER){
							field[cursorY][cursorX] = field[cursorY][cursorX] & 0x1F;
							markerCount--;
						}else{
							field[cursorY][cursorX] = field[cursorY][cursorX] | 0x20;
							markerCount++;
						}
						printSprite((field[cursorY][cursorX]), cursorX, cursorY);
						mySpritet((char*)scursor, (char*)(cursorY*6+topMargin<<8)+6*cursorX+leftMargin );
					}
					buttonState = 0xff;
				break;
				// case BUTTON_START:
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
					numberbomb = 10;
					fieldsx = 9;
					fieldsy = 9;
					topMargin = 35;
					buttonState = 0xff;
				break;
				case 'a': // new game advanced
				case 'A':
					gameOver = 1;
					newGame = 1;
					numberbomb = 40;
					fieldsx = 16;
					fieldsy = 16;
					topMargin = 28;
					buttonState = 0xff;
				break;
				case 'e': // new game expert
				case 'E':
					gameOver = 1;
					newGame = 1;
					numberbomb = 88;
					fieldsx = 26;
					fieldsy = 17;
					topMargin = 25;
					buttonState = 0xff;
				break;
				case 'd': // debug show field, uncover
				case 'D':
					for( y=0; y<fieldsy; y++ ){
						for( x=0; x<fieldsx; x++ ){
							printSprite((field[y][x] & 0x0F), x, y);
						}
					}
					gameOver = 1;
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
							for( y=0; y<fieldsy; y++ ){              // uncover all hidden fields
								for( x=0; x<fieldsx; x++ ){
									printSprite((field[y][x] & 0x0F), x, y);
								}
							}
						}else{ // no bomb in the field
							field[cursorY][cursorX] = field[cursorY][cursorX]& 0x0F;
							printSprite(field[cursorY][cursorX], cursorX, cursorY);
							revealedFields++;
							if(field[cursorY][cursorX] == SFREE) {
								// field is empty, adjacent fields can be uncovered
								qptr = 0;
								queue[qptr] = (cursorY<<8) + cursorX;
								qptr++;
								while(qptr>0){                       // loop automatic uncovering
									qptr--;
									ty = queue[qptr]>>8;
									tx = queue[qptr] & 0xFF;
									if(field[ty][tx] > 0x0F){
										if(field[ty][tx] > 0x1F) markerCount--; // remove incorrect marker
										revealedFields++;
										field[ty][tx] = field[ty][tx] & 0x0F;
										printSprite(field[ty][tx], tx, ty);
									}
									// search neighboring fields
									for(y = -1; y < 2; y++){
										for(x = -1; x < 2; x++){
											// loop adjacent fields
											x1 = tx + x; y1 = ty + y;
											if((x1 < fieldsx) && (x1 >= 0) && (y1 < fieldsy) && (y1 >= 0) && (field[y1][x1]>0x0F)){
												// field lies in the array and is not uncovered
												if(field[y1][x1] > 0x1F) markerCount--;  // remove incorrect marker
												field[y1][x1] = field[y1][x1] & 0x0F;    // uncover field
												printSprite(field[y1][x1], x1, y1);      // draw revealed field
												revealedFields++;
												if(field[y1][x1] == SFREE){              // field has no neighbor bombs, add to queue
													queue[qptr] = (y1<<8) + x1;
													qptr++;
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
			pos.x = 1;
			pos.y = 1;
			pos.addr = (char*)(videoTable[16*pos.y]<<8)+6*pos.x;
			myprintf("Bombs %d ", (numberbomb - markerCount));
			
			if(firstClick) seconds = (_clock() - ticks)/60;

			pos.x = 22;
			pos.y = 1;
			pos.addr = (char*)(videoTable[16*pos.y]<<8)+6*pos.x;
			if(seconds<10) myprintf("0");			
			if(seconds<100) myprintf("0");			
			myprintf("%d", seconds);			
			
			if((revealedFields+numberbomb)==(fieldsx*fieldsy)) gameOver = 1;
			
		}
		// game end
		if(!newGame){
			fgbg = 0x0F0A;
			clear_lines(0,16);
			pos.x = 0;
			pos.y = 0;
			pos.addr = (char*)(videoTable[16*pos.y]<<8)+6*pos.x;
			if((revealedFields+numberbomb)==(fieldsx*fieldsy)){
				myprintf("YOU are the winner!");
			}else{
				myprintf("You have lost");
			}
			pos.x = 0;
			pos.y = 1;
			pos.addr = (char*)(videoTable[16*pos.y]<<8)+6*pos.x;
			myprintf("Hit any key for new game");
			while(serialRaw == 0xFF) {}
			while(serialRaw != 0xFF) {}
		}
	}
	
    return 0;

}
