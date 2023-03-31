#include <stdlib.h>
#include <string.h>
#include <gigatron/console.h>
#include <gigatron/sys.h>
#include <gigatron/libc.h>
#include <stdarg.h>

#ifndef MEM32
# define MEM32 0 // set to 1 for 64 bits
#endif

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
#define MAXQ 40
#define REPETITION 6 // speed for automatic cursor movement

#ifdef MEM32
    #define MAXX 26 // max 26
    #define MAXY 17 // max 17
    #define TOP 2
#else
    #define MAXX 26 // max 26
    #define MAXY 17 // max 17
    #define TOP 0
#endif

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
const char shidden[]={58,58,58,58,58,50,58,58,58,58,58,50,58,58,58,58,58,50,58,58,58,58,58,50,58,58,58,58,58,50,50,50,50,50,50,50,250};         // 12 [0x0c]
const char smarker[]={58,58,19,19,58,50,58,19,19,19,58,50,58,58,19,19,58,50,58,58,58,1,58,50,58,58,1,1,1,50,50,50,50,50,50,50,250};             // 13 [0x0d]
const char bigcursor[]={35,35,35,35,35,35,35,35,
                        35, 0, 0, 0, 0, 0, 0,35,
                        35, 0, 0, 0, 0, 0, 0,35,
                        35, 0, 0, 0, 0, 0, 0,35,
                        35, 0, 0, 0, 0, 0, 0,35,
                        35, 0, 0, 0, 0, 0, 0,35,
                        35, 0, 0, 0, 0, 0, 0,35,
                        35,35,35,35,35,35,35,35,250};                                                                                           // 11 [0x0b] cursor closed


typedef enum {
    BEGINNER, ADVANCED, EXPERT
} levels;

struct game_level_s {
    char fieldsX, fieldsY;
    int fields;
    char numberBomb, topMargin;
} game_level;

__near char leftMargin;

unsigned int queue[MAXQ];    // queue for automatic uncovering of game fields
char field[MAXY][MAXX];      // byte array for playing field, lower nibble sprite id, upper nibble flags
char backup[64];
unsigned int colors;
char bottonLevel;

void setLevel(struct game_level_s *data, levels level){
    switch(level)
    {
        case ADVANCED:
            (*data).fieldsX = 16;
            (*data).fieldsY = 16;
            (*data).fields = 256;
            (*data).numberBomb = 40;
            (*data).topMargin = 20;
        break;
        case EXPERT:
            (*data).fieldsX = MAXX;               // 26
            (*data).fieldsY = MAXY;               // 17
            (*data).fields = MAXX * MAXY;         // 442
            (*data).numberBomb = MAXX * MAXY / 5; // 88
#ifdef MEM32
            (*data).topMargin = 18;               // 1*8 (one lines text) + 2 + 8 = 18 (pixel + 8)
#else
            (*data).topMargin = 17;               // 1*8 (one lines text) + 1 + 8 = 17 (pixel + 8)
#endif
        break;
        default: // BEGINNER
            (*data).fieldsX = 9;
            (*data).fieldsY = 9;
            (*data).fields = 81;
            (*data).numberBomb = 10;
            (*data).topMargin = 27;
        break;
    }

}

void printCursor(char *addr, char *dest){ //
    int ii, zz, vv;
    zz = vv = ii = 0;
    while(addr[vv] < 128){
        if(addr[vv] > 0){
            backup[vv] = dest[zz + ii];
            dest[zz + ii] = addr[vv];
        }
        vv++; ii++;
        if(ii > 7){
            ii = 0;
            zz += 256;
        }
    }
}

void restoreCursor(char *addr, char *dest){ //
    int ii, zz, vv;
    zz = vv = ii = 0;
    while(addr[vv] < 128){
        if(addr[vv] > 0){
            dest[zz + ii] = backup[vv];
        }
        vv++; ii++;
        if(ii > 7){
            ii = 0;
            zz += 256;
        }
    }
}

void printSprite(int val, int xx, int yy) // val is the id of the sprite, xx,yy is the x,y position in the playfield
{
    char* ptrChar;
    int sprnum;
    sprnum = val & 0x0f;
    if(val >= BHIDDEN) sprnum = SHIDDEN;
    if(val >= BMARKER) sprnum = SMARKER;
    ptrChar = (char*)sfree;
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
        case SHIDDEN:
            ptrChar = (char*)shidden;
            break;
        case SMARKER:
            ptrChar = (char*)smarker;
            break;
    }
    SYS_Sprite6(ptrChar, (char*)(yy*6+game_level.topMargin<<8)+6*xx+leftMargin);
}

int main()
{
    register char rep;
    register unsigned int ticks;
    register unsigned int seconds;        // elapsed seconds
    register char cursorX, cursorY;       // cursor in the playing field
    register char markerCount;            // counter for marked fields
    register char revealedFields;         // counter for revealed fields
    register char queuePointer;           // pointer to queue
    register char gameOver;               // flag, end of game reached
    register char newGame;                // Flag, start new game without closing the old one
    register char firstClick;             // Flag for start of the clock

    register char i, x1, y1, tx, ty; // help variables
    register int x, y; // help variables
    __near char buffer[8];
    //char c;

    bottonLevel = BEGINNER;
    setLevel(&game_level, bottonLevel);

    SYS_SetMode(2);

    for(;;){
        //SYS_SetMode(1975);        // faster calculation of the playing field
        videoTop_v5 = 224;

        leftMargin = (160 - 6*game_level.fieldsX)/2;

        _console_reset(0x3f38);
        _console_clear((char*)((8+8*14)<<8), 0x030a, 8);
        _console_printchars(0x030a, (char*)((8+8*14)<<8)+6*0, "please wait, initialize...", 26);

        console_state.fgbg = 0x030a;

        markerCount = 0;
        gameOver = 0;
        newGame = 0;
        firstClick = 0;
        seconds = 0;
        revealedFields = 0;

        for( y=0; y<game_level.fieldsY; y++ ){
            for( x=0; x<game_level.fieldsX; x++ ){
                // field[y][x] = SFREE;
                field[y][x] = BHIDDEN;
                printSprite(field[y][x], x, y);
            }
        }

        i = 0; // bomb counter temp
        // setting the bombs in the field
        while(i < game_level.numberBomb){
            x = rand() % (game_level.fieldsX-1);
            y = rand() % (game_level.fieldsY-1);
            if(field[y][x] != (SBOMB | BHIDDEN)){ // field is not a bomb, bomb set
                i++;                              // add bomb
                field[y][x] = SBOMB | BHIDDEN;    // set marker for bomb
            }
        }

        for( y=0; y<game_level.fieldsY; y++ ){
            for( x=0; x<game_level.fieldsX; x++ ){

                // count neighboring bombs
                if(field[y][x] != (SBOMB | BHIDDEN)){
                    // observe edges
                    if(x < game_level.fieldsX-1 ){ // right edge
                        if(field[y][x+1] == (SBOMB | BHIDDEN)) field[y][x]++;                      // right
                        if(y < game_level.fieldsY-1 ) if(field[y+1][x+1] == (SBOMB | BHIDDEN)) field[y][x]++; // bottom right
                        if(y > 0 ) if(field[y-1][x+1] == (SBOMB | BHIDDEN)) field[y][x]++;         // top right
                    }
                    if(x > 0 ){ // left edge
                        if(field[y][x-1] == (SBOMB | BHIDDEN)) field[y][x]++;                      // left
                        if(y < game_level.fieldsY-1 ) if(field[y+1][x-1] == (SBOMB | BHIDDEN)) field[y][x]++; // bottom left
                        if(y > 0 ) if(field[y-1][x-1] == (SBOMB | BHIDDEN)) field[y][x]++;         // top left
                    }
                    if(y < game_level.fieldsY-1 ){
                        if(field[y+1][x] == (SBOMB | BHIDDEN)) field[y][x]++;                      // bottom
                    }
                    if(y > 0 ) if(field[y-1][x] == (SBOMB | BHIDDEN)) field[y][x]++;               // top
                }

            }
        }

        // output info line
        _console_clear((char*)((8+8*14)<<8), 0x030a, 8);
        _console_printchars(0x030a, (char*)((8+8*14)<<8)+6*1, "B", 1);
        _console_printchars(0x200a, (char*)((8+8*14)<<8)+6*2, "eginner", 7);
        _console_printchars(0x030a, (char*)((8+8*14)<<8)+6*10, "A", 1);
        _console_printchars(0x200a, (char*)((8+8*14)<<8)+6*11, "dvanced", 7);
        _console_printchars(0x030a, (char*)((8+8*14)<<8)+6*19, "E", 1);
        _console_printchars(0x200a, (char*)((8+8*14)<<8)+6*20, "xpert", 5);

        _console_clear((char*)((TOP+8+8*0)<<8), 0x030a, 8);
        _console_printchars(0x020a, (char*)((TOP+8+8*0)<<8)+6*1, "Bombs", 5);
        videoTop_v5 = 2 * TOP;

        cursorX = 0;
        cursorY = 0;

        printCursor((char*)bigcursor, (char*)(cursorY*6-1+game_level.topMargin<<8) + 6 * cursorX-1 + leftMargin);

        while(!gameOver){

            //c = serialRaw;

            switch(serialRaw) {
                case BUTTON_START:  // blocked software reset
                    bottonLevel++;
                    if(bottonLevel > EXPERT) bottonLevel = BEGINNER;
                    gameOver = 1;
                    newGame = 1;
                    setLevel(&game_level, bottonLevel);
                    while(serialRaw != 0xff) {}
                break;
                case 'n': // start new game
                case 'N':
                    gameOver = 1;
                    newGame = 1;
                    while(serialRaw != 0xff) {}
                break;

                case 'b': // new game beginner
                case 'B':
                    gameOver = 1;
                    newGame = 1;
                    setLevel(&game_level, BEGINNER);
                    while(serialRaw != 0xff) {}
                break;
                case 'a': // new game advanced
                case 'A':
                    gameOver = 1;
                    newGame = 1;
                    setLevel(&game_level, ADVANCED);
                    while(serialRaw != 0xff) {}
                break;

                case 'e': // new game expert
                case 'E':
                    gameOver = 1;
                    newGame = 1;
                    setLevel(&game_level, EXPERT);
                    while(serialRaw != 0xff) {}
                break;
                case BUTTON_B:
                case 0x20: // set,unset marker with space
                    if((field[cursorY][cursorX] & BHIDDEN) == BHIDDEN){      // only on covered fields
                        if((field[cursorY][cursorX] & BMARKER) == BMARKER){
                            field[cursorY][cursorX] = field[cursorY][cursorX] & 0x1f;
                            markerCount--;
                        }else{
                            if(markerCount < game_level.numberBomb){                                // only so many as bombs
                                field[cursorY][cursorX] = field[cursorY][cursorX] | 0x20;
                                markerCount++;
                            }
                        }
                        restoreCursor((char*)bigcursor, (char*)(cursorY*6-1+game_level.topMargin<<8) + 6 * cursorX-1 + leftMargin);
                        printSprite((field[cursorY][cursorX]), cursorX, cursorY);
                        printCursor((char*)bigcursor, (char*)(cursorY*6-1+game_level.topMargin<<8) + 6 * cursorX-1 + leftMargin);
                    }
                    while(serialRaw != 0xff) {}
                break;

                case BUTTON_A:
                case 0x0a: // uncover game field with enter key
                    if(field[cursorY][cursorX] < 0x10) continue;  // is already uncovered
                    if(firstClick == 0){ // first click, start clock
                        firstClick = 1;
                        ticks = _clock();
                    }
                    if(field[cursorY][cursorX] < 0x20){              // marker protects field
                        if((field[cursorY][cursorX] & 0x0f) == SBOMB){
                            // game over
                            gameOver = 1;
                            field[cursorY][cursorX] = SBOMBTRIGGERED;
                            for( y=0; y<game_level.fieldsY; y++ ){              // uncover all hidden fields
                                for( x=0; x<game_level.fieldsX; x++ ){
                                    printSprite((field[y][x] & 0x0f), x, y);
                                }
                            }
                        }else{ // no bomb in the field
                            if(field[cursorY][cursorX] > 0x1f) markerCount--;  // remove incorrect marker ### before bug
                            field[cursorY][cursorX] = field[cursorY][cursorX]& 0x0f;
                            printSprite(field[cursorY][cursorX], cursorX, cursorY);
                            revealedFields++;
                            restoreCursor((char*)bigcursor, (char*)(cursorY*6-1+game_level.topMargin<<8) + 6 * cursorX-1 + leftMargin);
                            if(field[cursorY][cursorX] == SFREE) {
                                // field is empty, adjacent fields can be uncovered
                                queuePointer = 0;
                                queue[queuePointer] = (cursorY<<8) + cursorX;
                                queuePointer++;
                                while(queuePointer>0){
                                    queuePointer--;
                                    ty = queue[queuePointer]>>8;
                                    tx = queue[queuePointer] & 0xff;
                                    if(field[ty][tx] > 0x0f){
                                        if(field[ty][tx] > 0x1f) markerCount--;  // remove incorrect marker

                                        revealedFields++;
                                        field[ty][tx] = field[ty][tx] & 0x0f;
                                        printSprite(field[ty][tx], tx, ty);
                                    }
                                    // search neighboring fields
                                    for(y = -1; y < 2; y++){
                                        for(x = -1; x < 2; x++){
                                            // loop adjacent fields
                                            x1 = tx + x; y1 = ty + y;
                                            if((x1 < game_level.fieldsX) && (x1 >= 0) && (y1 < game_level.fieldsY) && (y1 >= 0) && (field[y1][x1]>0x0f)){
                                                // field lies in the array and is not uncovered
                                                if(field[y1][x1] > 0x1f) markerCount--;  // remove incorrect marker
                                                field[y1][x1] = field[y1][x1] & 0x0f;    // uncover field
                                                printSprite(field[y1][x1], x1, y1);      // draw revealed field
                                                revealedFields++;
                                                if(field[y1][x1] == SFREE){              // field has no neighbor bombs, add to queue
                                                    queue[queuePointer] = (y1<<8) + x1;
                                                    queuePointer++;
                                                    if(queuePointer > MAXQ) queuePointer = MAXQ; // prevent overflow
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        printCursor((char*)bigcursor, (char*)(cursorY*6-1+game_level.topMargin<<8) + 6 * cursorX-1 + leftMargin);
                    }
                break;
                default:
                    rep = 0;
                    if(serialRaw > 0xef) { // it is a controller key

                        if( ((serialRaw ^ BUTTON_DOWN) && (BUTTON_DOWN ^ 0xff)) == 0 ){ // down
                            if(cursorY < game_level.fieldsY-1){
                                restoreCursor((char*)bigcursor, (char*)(cursorY*6-1+game_level.topMargin<<8) + 6 * cursorX-1 + leftMargin);
                                cursorY++;
                                printCursor((char*)bigcursor, (char*)(cursorY*6-1+game_level.topMargin<<8) + 6 * cursorX-1 + leftMargin);
                                rep = REPETITION;
                            }
                        }
                        if( ((serialRaw ^ BUTTON_UP) && (BUTTON_UP ^ 0xff)) == 0 ){ // up
                            if(cursorY > 0){
                                restoreCursor((char*)bigcursor, (char*)(cursorY*6-1+game_level.topMargin<<8) + 6 * cursorX-1 + leftMargin);
                                cursorY--;
                                printCursor((char*)bigcursor, (char*)(cursorY*6-1+game_level.topMargin<<8) + 6 * cursorX-1 + leftMargin);
                                rep = REPETITION;
                            }
                        }
                        if( ((serialRaw ^ BUTTON_LEFT) && (BUTTON_LEFT ^ 0xff)) == 0 ){ // left
                            if(cursorX > 0){
                                restoreCursor((char*)bigcursor, (char*)(cursorY*6-1+game_level.topMargin<<8) + 6 * cursorX-1 + leftMargin);
                                cursorX--;
                                printCursor((char*)bigcursor, (char*)(cursorY*6-1+game_level.topMargin<<8) + 6 * cursorX-1 + leftMargin);
                                rep = REPETITION;
                            }
                        }
                        if( ((serialRaw ^ BUTTON_RIGHT) && (BUTTON_RIGHT ^ 0xff) ) == 0 ){ // right
                            if(cursorX < game_level.fieldsX-1){
                                restoreCursor((char*)bigcursor, (char*)(cursorY*6-1+game_level.topMargin<<8) + 6 * cursorX-1 + leftMargin);
                                cursorX++;
                                printCursor((char*)bigcursor, (char*)(cursorY*6-1+game_level.topMargin<<8) + 6 * cursorX-1 + leftMargin);
                                rep = REPETITION;
                            }
                        }

                        while((serialRaw != 0xff) && (rep > 0)) {
                            _wait(2);
                            rep--;
                        }
                    }

            }

            // output of status line
            if(seconds<999) seconds = (_clock() - ticks) / 60; else seconds = 999;
            if(!firstClick) seconds = 0;
            i = game_level.numberBomb - markerCount;
            if(i>9){
                _console_printchars(0x020a, (char*)((TOP+8+8*0)<<8)+6*7, itoa(i, buffer, 10), 2);
            }else{
                _console_printchars(0x020a, (char*)((TOP+8+8*0)<<8)+6*7, " ", 1);
                _console_printchars(0x020a, (char*)((TOP+8+8*0)<<8)+6*8, itoa(i, buffer, 10), 1);
            }
            if(seconds>999) seconds = 999;
            if(seconds>99) i=3; else if(seconds>9) i=2; else i=1;
            if(i < 3) _console_printchars(0x020a, (char*)((TOP+8+8*0)<<8)+6*22, "  ", 3-i);
            _console_printchars(0x020a, (char*)((TOP+8+8*0)<<8)+6*(25-i), utoa(seconds, buffer, 10), i);


            if((revealedFields + game_level.numberBomb) == game_level.fields) gameOver = 1;

        }
        // game end
        if(!newGame) {
            if((revealedFields + game_level.numberBomb) == game_level.fields) {
                colors = 0x031c;
                _console_clear((char*)((TOP+8+8*0)<<8), colors, 8);
                _console_printchars(colors, (char*)((TOP+8+8*0)<<8)+6*3, "YOU are the winner!", 24);
            }else{
                colors = 0x0f03;
                _console_clear((char*)((TOP+8+8*0)<<8), colors, 8);
                _console_printchars(colors, (char*)((TOP+8+8*0)<<8)+6*2, ">>> You have lost <<<", 24);
            }
            _console_clear((char*)((8+8*14)<<8), colors, 8);
            _console_printchars(colors, (char*)((8+8*14)<<8)+6*1, "Hit any key for new game", 24);

            while(serialRaw != 0xff) {}
            while(serialRaw == 0xff) {}
        }
    }
}
