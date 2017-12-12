#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}
#define KILO_VERSION "0.1"
/*****************data*************************************/


struct editorConfig
{
	struct termios orig_termios;
	int cx, cy;
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


/****************append buffer ********************/
struct abuf
{
	char *b;
	int len;
};

int abAppend(struct abuf *ab, const char *s, int len)
{
	char *new = realloc(ab->b, ab->len+ len);
	if(new == NULL)
		die("abAppend");

	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;

	return 0;
}

void abFree(struct abuf *ab)
{
	free(ab->b);
}

/**************output*****************************/

void editorDrawRows(struct abuf *ab)
{
	int y;
	for (y = 0; y < E.screenrows; y++)
	{
		if (y == E.screenrows/3)
		{
			char welcome[80];
			int welcomelen;
			welcomelen = snprintf(welcome, sizeof(welcome),
			"Kilo editor -- version %s", KILO_VERSION);
			if( welcomelen > E.screencols ) welcomelen = E.screencols;
			int padding = (E.screencols - welcomelen)/2;
			if (padding)
			{
				abAppend(ab,"~",1);
				padding--;
			}
			while(padding--)
				abAppend(ab, " ",1);
			abAppend(ab,welcome, welcomelen);
		}
		else
		{
			abAppend(ab, "~",1);
		}

		abAppend(ab,"\x1b[K",3);
		if(y < E.screenrows - 1)
			abAppend(ab, "\r\n", 2);

	}
}

void editorRefreshScreen()
{
	struct abuf ab = ABUF_INIT;

	//write(STDOUT_FILENO, "\x1b[2J", 4);
	abAppend(&ab, "\x1b[H",3);

	editorDrawRows(&ab);

	char buf[32];
	snprintf(buf,sizeof(buf),"\x1b[%d;%dH", E.cy+1, E.cx+1);
	abAppend(&ab, buf, strlen(buf));

	//abAppend(&ab, "\x1b[H",3);
	int byte_write;
	if ((byte_write = write(STDOUT_FILENO, ab.b, ab.len)) != ab.len) 
		die("Error while writing");
	abFree(&ab);
}


/********************input************************/

void editorMoveCursor(char key)
{
	switch (key)
	{
		case 'a':
			E.cx--;
			break;
		case 'd':
			E.cx++;
			break;
		case 'w':
			E.cy--;
			break;
		case 's':
			E.cy++;
			break;
	}
}


void editorProcessKeypress()
{
	char c = editorReadKey();
	switch(c)
	{
		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H",3);
			exit(0);
			break;
		case 'w':
		case 's':
		case 'a':
		case 'd':
			editorMoveCursor(c);
			break;
	}
}


/*************init**************/
void initEditor()
{
	E.cx = 0;
	E.cy = 0;
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
