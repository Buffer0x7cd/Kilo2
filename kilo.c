#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>

#define CTRL_KEY(k) ((k) & 0x1f)

/*****************data*************************************/


struct editorConfig
{
	struct termios orig_termios;
	int screenrows;
	int screencols;
};

struct editorConfig E;
/******************terminal**************/
/*
 * Helper method for printing the error and
 * exit the program */


void die(const char *msg)
{

	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H",3);
	perror(msg);
	exit(1);
}


char editorReadKey()
{
	int nread;
	char c;
	while((nread = read(STDIN_FILENO, &c, 1)) != 1)
		if (nread == -1) 
			die("read");
	return c;
}
/*cleanup routine to store the default behavaior
 * of user's terminal */
void disableRawMode()
{

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1 ) die("tcsetattr");
}


void enableRawMode()
{
	struct termios raw;
        if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");	
	raw = E.orig_termios;

	atexit(disableRawMode);
	raw.c_iflag &= ~(BRKINT|INPCK|ISTRIP|IXON|ICRNL);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO|ICANON|IEXTEN|ISIG);

	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int getWindowSize(int *rows, int *cols)
{
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
	{
		return -1;
	}
	else
	{
		*rows = ws.ws_row;
		*cols = ws.ws_col;
		return 0;
	}
}

/**************output*****************************/

void editorDrawRows()
{
	int y;
	for (y = 0; y < E.screenrows; y++)
		write(STDOUT_FILENO, "~\r\n", 3);
}

void editorRefreshScreen()
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H",3);

	editorDrawRows();
	write(STDOUT_FILENO, "\x1b[H",3);
}


/********************input************************/

void editorProcessKeypress()
{
	char c = editorReadKey();
	switch(c)
	{
		case CTRL_KEY('q'):
			exit(0);
			break;
	}
}


/*************init**************/
void initEditor()
{
	if(getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main(void)
{
	enableRawMode();
	initEditor();
	while(1)
	{
		editorRefreshScreen();
		editorProcessKeypress();
	}
	return 0;
}
