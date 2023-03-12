#include <stdio.h>
#include <gigatron/console.h>
#include <gigatron/sys.h>
#include <gigatron/libc.h>
#include <time.h>


#define FGBG 0x3f20


// extern void _console_clear(char *addr, int clr, int nl);
// extern int _console_printchars(int fgbg, char *addr, const char *s, int len);
// extern void _console_reset(int fgbg);

main(void) {
	int x, y;
	char *addr;
	int c,r;
	
	//console_clear_screen();
	x = 30;
	y = 22;

	console_state.fgbg = 0x030a;
	_console_reset(0x3f20);
	_console_clear((char*)(8<<8), 0x030a, 16);
	_console_printchars(0x030a, (char*)(8<<8)+6*1, "B", 1);
	_console_printchars(0x200a, (char*)(8<<8)+6*2, "eginner", 7);
	_console_printchars(0x030a, (char*)(8<<8)+6*10, "A", 1);
	_console_printchars(0x200a, (char*)(8<<8)+6*11, "dvanced", 7);
	_console_printchars(0x030a, (char*)(8<<8)+6*19, "E", 1);
	_console_printchars(0x200a, (char*)(8<<8)+6*20, "xpert", 5);
	
	console_state.fgbg = 0x030a;

	
	_console_clear((char*)((8+8*3)<<8), 0x0f03, 16);
	_console_printchars(0x0f03, (char*)(8+8*3<<8)+6*2, ">>> You have lost <<<", 21);
	_console_printchars(0x0f03, (char*)(8+8*4<<8)+6*1, "Hit any key for new game", 24);

	_console_clear((char*)((8+8*6)<<8), 0x01c, 16);
	_console_printchars(0x031c, (char*)(8+8*6<<8)+6*3, "YOU are the winner!", 19);
	_console_printchars(0x031c, (char*)(8+8*7<<8)+6*1, "Hit any key for new game", 24);

	for(;;){
		_console_printchars(0x020a, (char*)(8*2<<8)+6*1, "Bombs", 5);
		console_state.fgbg = 0x020a;
		console_state.cy = 1;
		console_state.cx = 7;
		cprintf("%2d", x);
		console_state.cx = 21;
		cprintf("%4d", y);
		
		while(serialRaw == 0xff) {}
		while(serialRaw != 0xff) {}
		x++;
		y+=3;
	}

	return 0;
}


