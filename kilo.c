#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>

#define CTRL_KEY(k) ((k) & 0x1f)



struct termios orig_termios;

/*
 * Helper method for printing the error and
 * exit the program */
void die(const char *msg)
{
	perror(msg);
	exit(1);
}



/*cleanup routine to store the default behavaior
 * of user's terminal */
void disableRawMode()
{
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1 ) die("tcsetattr");
}


void enableRawMode()
{
	struct termios raw;
        if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");	
	raw = orig_termios;

	atexit(disableRawMode);
	raw.c_iflag &= ~(BRKINT|INPCK|ISTRIP|IXON|ICRNL);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO|ICANON|IEXTEN|ISIG);

	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int main(void)
{
	enableRawMode();
	while(1)
	{
		char c = '\0';
		if(read(STDIN_FILENO, &c, 1) == -1)
				die("read error");
		if(iscntrl(c))
		{
			printf("%d\r\n",c);
		}
		else
		{
			printf("%d ('%c')\r\n",c,c);
		}
		if (c == CTRL_KEY('q')) break;
	}
	return 0;
}
