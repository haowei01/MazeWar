/*
 * init.c - Initialization code for mazewar
 *
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 *	 	Digital Equipment Corporation
 * Date:	Thu Oct  2 1986
 */

/*
   Modified by Michael Greenwald, CS244B Stanford University.
   michaelg@pescadero.stanford.edu
 */

/***********************************************************
Copyright 1986 by Digital Equipment Corporation, Maynard, Massachusetts,

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital not be
used in advertising or publicity pertaining to disstribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include "main.h"

#include "mazewar.h"

#include "unistd.h"

static char rstate[32];

static const MazeType mazeBits = {
	{ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},	/* 0 */
	{ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{ 1,0,1,1,1,1,1,0,1,1,1,1,0,1,1,1},
	{ 1,0,0,0,0,0,1,0,1,0,0,0,0,0,0,1},

	{ 1,0,1,0,1,0,0,0,1,0,1,1,1,1,0,1},	/* 1 */
	{ 1,0,1,1,1,0,1,0,1,0,1,0,0,0,0,1},
	{ 1,0,0,0,0,0,1,0,0,0,1,0,1,1,0,1},
	{ 1,1,1,1,1,0,1,1,1,0,0,0,0,0,0,1},

	{ 1,0,0,0,0,0,0,0,1,0,1,1,1,1,0,1},	/* 2 */
	{ 1,0,1,1,1,1,1,0,1,0,1,0,0,0,0,1},
	{ 1,0,0,0,0,0,1,0,0,0,1,0,1,1,1,1},
	{ 1,0,1,1,1,0,1,0,1,0,1,0,0,0,0,1},

	{ 1,0,0,0,0,0,1,0,1,0,1,1,1,1,0,1},	/* 3 */
	{ 1,0,1,1,1,1,1,0,1,0,1,1,0,1,0,1},
	{ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{ 1,0,1,1,1,1,1,1,1,0,1,0,1,1,1,1},

	{ 1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1},	/* 4 */
	{ 1,1,1,0,1,0,1,0,1,0,1,0,1,1,0,1},
	{ 1,0,0,0,1,0,1,0,1,0,1,0,0,1,0,1},
	{ 1,0,1,1,1,0,1,0,1,0,1,1,0,0,0,1},

	{ 1,0,0,0,0,0,0,0,1,0,1,0,0,1,0,1},	/* 5 */
	{ 1,0,1,1,1,1,1,1,1,0,1,0,1,1,0,1},
	{ 1,0,0,0,1,0,0,0,1,0,1,0,0,1,0,1},
	{ 1,0,1,0,1,0,1,0,1,0,1,1,0,0,0,1},

	{ 1,0,1,0,1,0,1,0,1,0,0,0,0,1,0,1},	/* 6 */
	{ 1,0,1,0,1,0,1,0,1,0,1,0,1,1,0,1},
	{ 1,0,1,0,1,0,1,0,1,0,1,0,0,1,0,1},
	{ 1,0,1,0,0,0,1,0,1,0,1,1,0,1,0,1},

	{ 1,0,1,0,1,0,1,0,0,0,1,0,0,0,0,1},	/* 7 */
	{ 1,0,1,0,1,0,1,0,1,0,1,1,1,1,0,1},
	{ 1,0,0,0,1,0,0,0,1,0,0,0,0,1,0,1},
	{ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};


/* ----------------------------------------------------------------------- */

void
MazeInit(int argc, char	**argv)
{
	getMaze();

	setRandom();

	InitDisplay(argc, argv);

  int xloc = -1, yloc = -1, dir = -1;
  if(argc >= 3)
    xloc = atoi(argv[2]);
  if(argc >= 4)
    yloc = atoi(argv[3]);
  if(argc >= 5)
    switch(argv[4][0]) {
      case 'n' :
      case 'N' : dir = NORTH; break;
      case 's' : 
      case 'S' : dir = SOUTH; break;
      case 'w' :
      case 'W' : dir = WEST; break;
      case 'e' :
      case 'E' : dir = EAST; break;
      default  : dir = -1; break;
    }

	NewPosition(M, xloc, yloc, dir);

	printf("%d X LOC \n",M->xloc().value());
	/*
	 * We don't do ShowPosition() or ShowView() here, but let the update
	 * routine in the window handler do it for the first time when
	 * the window is exposed.
	 */

	netInit();
	StartDisplay();
	RatCursor();
}

/* ----------------------------------------------------------------------- */

/* get the maze into memory */
void
getMaze(void)
{
	int i, j;

	for (i = 0; i < MAZEXMAX; i++){
		for (j = 0; j < MAZEYMAX; j++){
			if (mazeBits[i][MAZEYMAX-1 - j] == 1){
				M->maze_[i][j] = TRUE;
			}else{
				M->maze_[i][j] = FALSE;
			}
		}
	}
}

/* ----------------------------------------------------------------------- */

/* initialize random() */
void
setRandom(void)
{

  struct timeval	now;

  gettimeofday (&now, NULL);
  initstate((now.tv_usec/16000), (char *) rstate, 32);
  setstate(rstate);
}

/* ----------------------------------------------------------------------- */

/* get player and maze name */
void
getName(char *prompt, char **ratName)
{
	char		buf[128], *cp;
	char		*comma;

	buf[0] = '\0';
	printf("%s: ", prompt);

	fgets(buf, sizeof(buf)-1, stdin);
	if (buf[0] == '\0') {
		if ((cp = getenv("USERNAME")) != NULL)
			strcpy(buf, cp);
		else {
			strcpy(buf, (getpwuid(getuid()))->pw_gecos);
			comma = (char *)index(buf, ',');
			if (comma != NULL)
				*comma = '\0';
		}
	}
	*ratName = (char*)malloc((unsigned) (strlen(buf) + 1));
	if (*ratName == NULL)
		MWError("no mem for ratName");
	strcpy(*ratName, buf);
}

/* ----------------------------------------------------------------------- */
void
getString(char *prompt, char **string)
{
	char		buf[128];

	buf[0] = '\0';
	printf("%s: ", prompt);
	fgets(buf, sizeof(buf)-1, stdin);
	*string = (char*)malloc((unsigned) (strlen(buf) + 1));
	if (*string == NULL)
	  MWError("no mem for getString");
	strcpy(*string, buf);
}

/* ----------------------------------------------------------------------- */

/* get hostname and host socket */
void
getHostName(char *prompt, char **hostName, Sockaddr *hostAddr)
{
	char		buf[128];
	Sockaddr	*AddrTemp;

	buf[0] = '\0';
	for (AddrTemp = (Sockaddr *)NULL; AddrTemp == (Sockaddr *)NULL; )
	  {
		printf("%s %s: " , prompt, "(CR for any host)");
		fgets(buf, sizeof(buf)-1, stdin);
		if (strlen(buf) == 0)
			break;
		*hostName = (char*)malloc((unsigned) (strlen(buf) + 1));
		if (*hostName == NULL)
			MWError("no mem for hostName");
		strcpy(*hostName, buf);

		/* check for valid maze name */
		AddrTemp = resolveHost(*hostName);
		if (AddrTemp== (Sockaddr *) NULL) {
			printf("Don't know host %s\n", *hostName);
			free(*hostName);
			*hostName = NULL;
		}
	}
	if ((*hostName != NULL) &&
	    (strlen(*hostName) != 0))
		bcopy((char *) AddrTemp, (char *) hostAddr, sizeof(Sockaddr));
}

/* ----------------------------------------------------------------------- */

/*
 * Resolve the specified host name into an internet address.  The "name" may
 * be either a character string name, or an address in the form a.b.c.d where
 * the pieces are octal, decimal, or hex numbers.  Returns a pointer to a
 * sockaddr_in struct (note this structure is statically allocated and must
 * be copied), or NULL if the name is unknown.
 */

Sockaddr *
resolveHost(register char *name)
{
	register struct hostent *fhost;
	struct in_addr fadd;
	static Sockaddr sa;

  struct in_addr **addr_list;

	if ((fhost = gethostbyname(name)) != NULL) {
		sa.sin_family = fhost->h_addrtype;
		sa.sin_port = 0;
		bcopy(fhost->h_addr, &sa.sin_addr, fhost->h_length);
    addr_list = (struct in_addr **)fhost->h_addr_list;
    for(int i=0; addr_list[i] != NULL; ++i) {
      cout << "cout get host by name" <<inet_ntoa(*addr_list[i]) <<endl;
    }
	} else {
		fadd.s_addr = inet_addr(name);
		if (fadd.s_addr != (unsigned long)(-1)) {
			sa.sin_family = AF_INET;	/* grot */
			sa.sin_port = 0;
			sa.sin_addr.s_addr = fadd.s_addr;
		} else
			return(NULL);
	}
	return(&sa);
}

/* ----------------------------------------------------------------------- */

bool emptyAhead ()
{
  register int tx = MY_X_LOC;
  register int ty = MY_Y_LOC;

  switch(MY_DIR) {
  case NORTH:	return(!M->maze_[tx+1][ty]);
  case SOUTH:	return(!M->maze_[tx-1][ty]);
  case EAST:	return(!M->maze_[tx][ty+1]);
  case WEST:	return(!M->maze_[tx][ty-1]);
  }

  assert(0);
  return (0);
}

/* ----------------------------------------------------------------------- */

bool emptyRight ()
{
  register int 	tx = MY_X_LOC;
  register int 	ty = MY_Y_LOC;

  switch(MY_DIR) {
  case NORTH:	return(!M->maze_[tx][ty+1]);
  case SOUTH:	return(!M->maze_[tx][ty-1]);
  case WEST:	return(!M->maze_[tx+1][ty]);
  case EAST:	return(!M->maze_[tx-1][ty]);
  }

  assert(0);
  return (0);
}

/* ----------------------------------------------------------------------- */

bool emptyLeft ()
{
  register int 	tx = MY_X_LOC;
  register int 	ty = MY_Y_LOC;

  switch(MY_DIR) {
  case NORTH:	return(!M->maze_[tx][ty-1]);
  case SOUTH:	return(!M->maze_[tx][ty+1]);
  case WEST:	return(!M->maze_[tx-1][ty]);
  case EAST:	return(!M->maze_[tx+1][ty]);
  }

  assert(0);
  return (0);
}

/* ----------------------------------------------------------------------- */

bool emptyBehind ()
{
  register int 	tx = MY_X_LOC;
  register int 	ty = MY_Y_LOC;

  switch(MY_DIR) {
  case NORTH:	return(!M->maze_[tx-1][ty]);
  case SOUTH:	return(!M->maze_[tx+1][ty]);
  case EAST:	return(!M->maze_[tx][ty-1]);
  case WEST:	return(!M->maze_[tx][ty+1]);
  }

  assert(0);
  return (0);
}

/* ----------------------------------------------------------------------- */
