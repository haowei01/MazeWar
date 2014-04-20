/* $Header: winX11.c,v 1.32 89/08/25 13:53:49 kent Exp $ */

/*
 * window_X11.c - Window-system specific routines for X11
 *
 * Author:	Mike Yang
 * 		Western Research Laboratory
 *	 	Digital Equipment Corporation
 * Date:	Mon Jul 25 1988
 */

/* modified by Michael Greenwald for CS244B, March 1992. */

/***********************************************************
Copyright 1988 by Digital Equipment Corporation, Maynard, Massachusetts,

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

#define	USE_BITMAPS	/* turn on if you know CopyPlanes works */

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/StringDefs.h>

#include "main.h"

#include "mazewar.h"

#include <X11/Intrinsic.h>
#include <X11/Xaw/Cardinals.h>

#define	VIEW_X_DIM	400
#define	VIEW_Y_DIM	400
#define	VIEW_X_ORIGIN	100
#define	VIEW_Y_ORIGIN	50
#define	MAZE_X_DIM	(MAZEXMAX*16)
#define	MAZE_Y_DIM	(MAZEYMAX*16)
#define	MAZE_X_ORIGIN	48
#define	MAZE_Y_ORIGIN	451
#define	SCORE_X_DIM	192
#define	SCORE_Y_DIM	((scoreFontInfo->max_bounds.ascent + scoreFontInfo->max_bounds.descent) * MAX_RATS)
#define	SCORE_X_ORIGIN	208
#define	SCORE_Y_ORIGIN	708
#define	MIN_X_DIM	608		/* see comments for InitWindow() */
#define	MIN_Y_DIM	SCORE_Y_ORIGIN	/* see InitWindow() */

#define DEFAULT_FONT "8x13"
#define DEFAULT_POSITION "%dx%d+0+0"	/* upper left hand corner */

#define ICON_FLASH_PERIOD 4
#define TIME_INTERVAL "200"

short	mazeBits[MAZEXMAX*MAZEYMAX*16];	/* shorts better be 16 bits! */
Window	mwWindow;			/* parent window */
#ifdef	USE_BITMAPS
Pixmap	mazeBitmap;			/* the maze */
Pixmap	ratsBitmap;			/* the rats */
#else
XImage	*ratsImage;			/* the rats */
XImage	*mazeImage;			/* the maze */
#endif	/* USE_BITMAPS */
Pixmap	iconMask;			/* mask for icon outline */
XFontStruct	*scoreFontInfo;
bool	mapped =	FALSE;		/* should really display? */
bool	flashIcon =	FALSE;		/* should icon be flashing? */
bool	iconInverted =	FALSE;		/* icon should be/is inverted */
int	displayFD;			/* fd of display for events */

Display	*dpy;				/* display used */
int	screen;				/* screen on the display */
unsigned int	cur_width, cur_height;	/* current width, height of window */
GC	copyGC, xorGC;			/* graphics contexts for window */
Pixmap	icon_pixmap, iconmask_pixmap;	/* icon, mask bitmaps */
Pixmap	icon_reverse_pixmap;
int	icon_flash = 0;
XImage	*arrowImage;

#include "bitmaps/icon.ic"
#include "bitmaps/rat.cur"
#include "bitmaps/ratMask.cur"
#include "bitmaps/dRat.cur"
#include "bitmaps/dRatMask.cur"
#include "bitmaps/cup.cur"

static Cursor	ratCursor, deadRatCursor, hourGlassCursor;

static char	*progName;

XtAppContext app_con;

struct _resources {
  Pixel		fg_pixel;		/* color of lines and such */
  Pixel		bg_pixel;		/* color of background */
  Pixel		mouseground;		/* mouse cursor color */
  Font		scoreFont;		/* font for printing scores */
  Pixel		borderColor;
  Cardinal	borderWidth;

  int 		time_interval; /* in milliseconds */
  bool	robotic;
 }
app_resources;

String default_resources[] = {
    (char *)(string("time_interval ") + TIME_INTERVAL).c_str(),

    NULL,
};


static XrmOptionDescRec opTable[] = {
{(char *)("-mouse"), (char *)("*mouseColor"), XrmoptionSepArg, (caddr_t) NULL},
{(char *)("-time_interval"), (char *)("*time_interval"), XrmoptionSepArg,	(caddr_t) NULL},
};

#define	XtNmouse	"mouse"

int	zero = 0;
int	one = 1;
const void* blah;

#define offset(field)	XtOffset (struct _resources *, field)
static XtResource resources[] = {
        {(char *)("time_interval"), (char *)("Time_Interval"), XtRInt, sizeof (int),
	   offset(time_interval), XtRString, strdup(TIME_INTERVAL) },
	{XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	 offset(fg_pixel), XtRString, strdup("black")},
	{XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
	 offset(bg_pixel), XtRString, strdup("white")},
	{(char *)(XtNmouse), XtCCursor, XtRPixel, sizeof(Pixel),
	   offset(mouseground), XtRString, strdup("black")},
	{XtNfont, XtCFont, XtRFont, sizeof(Font),
	 offset(scoreFont), XtRString, strdup(DEFAULT_FONT)},
	{XtNborderWidth, XtCBorderWidth, XtRInt, sizeof(int),
	 offset(borderWidth), XtRInt, (caddr_t) (void*)&one},
	{XtNborder, XtCBorderColor, XtRPixel, sizeof(Pixel),
	 offset(borderColor), XtRString, strdup("black")},
};
#undef offset

char *GetRatName();


static short RandomEvent(void);
static void initCursors(void);
static void initMaze(void);
static void initRats(int width, int height);
static void drawMaze(void);
static void repaintWindow(void);
static void repaintIcon(void);
static Pixmap xCreateBitmapFromBitCell(Display *, Drawable, char *,
				       unsigned int, unsigned int);


/* ----------------------------------------------------------------------- */

/*
 * ALL COORDINATES ASSUME ORIGIN (0,0) AT UPPER LEFT!
 */

/*
 * Initialize the window. The overall window must be at least 608x by
 * 808y (the size of the Alto screen). There are three distinct
 * subregions:
 *	A 400x by 400y area for the view, beginning at (100,50)
 *	A 512x by 256y area for the maze, beginning at (48, 451)
 *	A 192x by 96y area for the score lines, at (208, 706)
 *
 *	Actually, the y dimension of both the whole window and the score
 *	subwindow must be enough to accommodate MAX_RATS lines of the
 *	height of the scoreFont.
 */

/* parse arguments and set up window state */

void
InitWindow(int argc, char **argv)
{
	XGCValues gc_values;
	XWMHints wmhints;
	Widget	w;
	XClassHint	classHint;
	GC	iconGC;

	progName = (char *)rindex(argv[0], '/');
	if (progName)
		progName++;
	else
		progName = argv[0];

	/*
	 * We cheat here by using the Toolkit to do the initialization work.
	 * We just ignore the top-level widget that gets created.
	 */

	w = XtAppInitialize(&app_con, progName, opTable, XtNumber(opTable),
			&argc, argv, default_resources, NULL, ZERO);

	if ((argc > 1) && (strcmp("-robot", argv[1]) == 0))
	  { argc--;
	    app_resources.robotic = TRUE;
	  }
	else {app_resources.robotic = FALSE;}

	printf("set robot. ");
	dpy = XtDisplay(w);
	screen = DefaultScreen(dpy);

	printf("set Xscreen. ");

	XtGetApplicationResources(w, (caddr_t) &app_resources, resources,
				XtNumber(resources), NULL, (Cardinal) 0);

	printf("set XResource. ");

	if (!app_resources.scoreFont)
		MWError("cannot open font");
	scoreFontInfo = XQueryFont(dpy, app_resources.scoreFont);

	printf("set XQueue. ");

        cur_width = MIN_X_DIM;
        cur_height = MIN_Y_DIM + (MAX_RATS+1) *
	             (scoreFontInfo->max_bounds.ascent +
		      scoreFontInfo->max_bounds.descent);

	mwWindow = XCreateSimpleWindow(dpy,
					RootWindow(dpy, screen),
					0, 0,
					cur_width, cur_height,
					app_resources.borderWidth, 0,
				       app_resources.bg_pixel);
	XStoreName(dpy, mwWindow, "MazeWar");
	XSetIconName(dpy, mwWindow, "MazeWar");
	classHint.res_name = (char *)("cs244Bmazewar");
	classHint.res_class = (char *)("cs244Bmazewar");
	XSetClassHint(dpy, mwWindow, &classHint);

	gc_values.function = GXcopy;
	gc_values.foreground = app_resources.fg_pixel;
	gc_values.background = app_resources.bg_pixel;
	gc_values.font = app_resources.scoreFont;
	gc_values.line_width = 0;
	copyGC = XCreateGC(dpy, mwWindow,
		       GCFunction | GCForeground | GCBackground
		       | GCLineWidth | GCFont,
		       &gc_values);

	gc_values.function = GXxor;
	gc_values.plane_mask = AllPlanes;
	gc_values.foreground = app_resources.fg_pixel ^ app_resources.bg_pixel;
	gc_values.background = 0;
	xorGC = XCreateGC(dpy, mwWindow,
		       GCFunction | GCForeground | GCBackground | GCPlaneMask,
		       &gc_values);

	icon_pixmap = XCreatePixmapFromBitmapData(
			dpy, mwWindow,
			(char *)icon_bits,
			icon_width, icon_height,
			app_resources.fg_pixel, app_resources.bg_pixel,
			XDefaultDepth(dpy, screen));

	/* is this even used? */
	gc_values.function = GXclear;
	gc_values.plane_mask = AllPlanes;
	iconGC = XCreateGC(dpy, mwWindow,
			GCFunction | GCPlaneMask,
			&gc_values);
	iconmask_pixmap = XCreatePixmap(dpy, mwWindow,
					icon_width, icon_height,
					XDefaultDepth(dpy, screen));
	XFillRectangle(dpy, iconmask_pixmap, iconGC, 0, 0,
			icon_width, icon_height);

	icon_reverse_pixmap = XCreatePixmapFromBitmapData(dpy, mwWindow,
						  (char *)icon_bits,
						  icon_width, icon_height,
						  app_resources.bg_pixel,
						  app_resources.fg_pixel,
						  XDefaultDepth(dpy, screen));

	wmhints.input = TRUE;
        wmhints.flags = IconPixmapHint | IconMaskHint | InputHint;
        wmhints.icon_pixmap = icon_pixmap;
        wmhints.icon_mask = iconmask_pixmap;
        XSetWMHints(dpy, mwWindow, &wmhints);

	initCursors();
	arrowImage = XCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
				1, XYBitmap, 0, NULL,
				16, 16, 8, 2);
	arrowImage->byte_order = MSBFirst;
	arrowImage->bitmap_bit_order = MSBFirst;
}

/* ----------------------------------------------------------------------- */

/*
 * actually start the display up, after all the user interaction has
 * been done.
 */
void
StartWindow(int ratWidth, int ratHeight)
{

	XSelectInput(dpy, mwWindow, KeyPressMask | ButtonPressMask |
		                    ButtonReleaseMask |  ExposureMask |
				    StructureNotifyMask | FocusChangeMask);
	HourGlassCursor();

	initMaze();
	SwapBitmaps();

	displayFD = XConnectionNumber(dpy);

	XMapWindow(dpy, mwWindow);		/* Map window to screen */
	initRats(ratWidth, ratHeight);
}

/* ----------------------------------------------------------------------- */

/*
 * Clear out the view subwindow. If possible, delay the visible effect
 * until all the lines have been drawn (since this is only called by
 * ShowView(), which is about to draw a bunch of lines).
 */
void
ClearView(void)
{
	XClearArea(dpy, mwWindow, VIEW_X_ORIGIN, VIEW_Y_ORIGIN,
		       VIEW_X_DIM, VIEW_Y_DIM, False);
}

/* ----------------------------------------------------------------------- */

/*
 * Draw a line in the view subwindow. Don't show it right away if possible.
 */
void
DrawViewLine(int x1, int y1, int x2, int y2)
{
	XDrawLine(dpy, mwWindow, copyGC, x1+VIEW_X_ORIGIN, y1+VIEW_Y_ORIGIN,
		  x2+VIEW_X_ORIGIN, y2+VIEW_Y_ORIGIN);
}

/* ----------------------------------------------------------------------- */

static short TheNextEvent = 0;

static short RandomEvent(void)
{
  bool 	visible = FALSE;
  register 	RatIndexType ratIndex(0);
  int      	choice;

  choice = random()%4;

  if ((choice == 0) || M->peeking()) {
    for (ratIndex = RatIndexType(0); ratIndex.value()<MAX_RATS; ratIndex = RatIndexType(ratIndex.value() +1 ))
      { if ((ratIndex != MY_RAT_INDEX) &&
	    (Rats2Display[ratIndex.value()].visible == TRUE))
	  { visible = TRUE; break; }
      }
  }

  if (TheNextEvent > 0) {
    TheNextEvent = -TheNextEvent;
    return(EVENT_D);
  }
  else if (TheNextEvent < 0) {
    M->dirIs( Direction(-TheNextEvent));
    TheNextEvent = 0;
    return(EVENT_MIDDLE_D);
  }
  else if ((M->peeking()) && (visible == TRUE))
    { /* Cheat */
      TheNextEvent = M->dirPeek().value();
      return(EVENT_LEFT_U); }
  else if ((M->peeking()) && (choice != 2))
    { return(EVENT_LEFT_U); }
  else if (visible == TRUE)
    { return(EVENT_MIDDLE_D); }
  else if (emptyAhead() && choice != 2)
    { return(EVENT_D); }
  else if (emptyRight())
    { return(EVENT_F); }
  else if (emptyLeft())
    { return(EVENT_S); }
  else if ((choice != 2) && (emptyBehind()))
    { return(EVENT_A); }
  else { return(4 + (random() % 4)); } /* Never about face unless blocked */
}

/* ----------------------------------------------------------------------- */

/*
 * Event handling. Presents a uniform event interface to the game, and
 * handles all other events in here. The game only sees the events
 * that affect it directly, as defined in mazewar.h. They are:
 *
 *	keypresses that affect the game
 *	mouse button clicks
 *	incoming network packets
 *	timeouts (there's a "heartbeat" to drive the shot clock)
 *
 * All other events should be swallowed by this routine. Also, since
 * keyboard focus is dependent on the mouse being in the game window,
 * try to let the user know that he's wandered outside the window
 * by inverting the window (should make it obvious) and is now vulnerable
 * and defenseless.
 */

void
NextEvent(MWEvent *event, int socket)
{
  fd_set	fdmask;
  XEvent	xev;
  static int nexttimeoutinitialized=0;
  static struct timeval nexttimeout;
  struct timeval timeout,oldtime,newtime;
  int	ret;
  char	c;

  if (!nexttimeoutinitialized)
    {
      nexttimeoutinitialized=1;
      nexttimeout.tv_sec=app_resources.time_interval/1000;
      nexttimeout.tv_usec=1000*(app_resources.time_interval%1000);
    }
  while (1)
    {
      icon_flash = (++icon_flash) % ICON_FLASH_PERIOD;

      if (!XPending(dpy))	/* this does an XFlush, too */
	if (flashIcon && !icon_flash && !mapped)
	  {
	    /* invert the icon  */
	    iconInverted = !iconInverted;
	    repaintIcon();
	  }

      /*
       * Look for events.  Try to arrange that X events have priority over
       * network traffic.  See if there's an X event pending.  If so, check
       * for a net event, too; if not, select on both the network and the X
       * connection.  If that doesn't time out, but there's no X event
       * pending, try again, just selecting on the X connection.  If that
       * times out, let the network event get processed.
       *
       * Can't just select on the two fds, because there may be X events
       * pending in the queue that have already been read.
       *
       * This may look baroque, but we've seen some instances where X server
       * latency seems to let the network events take priority over sever
       * events, leading to sluggish keyboard response and lots of local
       * death.
       *
       */

      if (!XPending(dpy))
	{
	  FD_ZERO(&fdmask);
	  FD_SET(displayFD, &fdmask);
	  FD_SET(socket, &fdmask);
	  for (ret=0;ret<=0;)
	    {
	      if ((nexttimeout.tv_sec<0)||
		  ((nexttimeout.tv_sec==0)&&(nexttimeout.tv_usec<=0)))
		{
		  nexttimeout.tv_sec=app_resources.time_interval/1000;
		  nexttimeout.tv_usec=
		    1000*(app_resources.time_interval%1000);
		  if (app_resources.robotic==TRUE)
		    {
		      event->eventType=RandomEvent();
		      return;
		    }
		}
	      timeout.tv_sec=nexttimeout.tv_sec;
	      timeout.tv_usec=nexttimeout.tv_usec;
	      gettimeofday(&oldtime,0);
	      ret=select(32,&fdmask,NULL,NULL,&timeout);
	      gettimeofday(&newtime,0);
	      nexttimeout.tv_sec-=newtime.tv_sec-oldtime.tv_sec;
	      nexttimeout.tv_usec-=newtime.tv_usec-oldtime.tv_usec;
	      if (nexttimeout.tv_usec<0)
		{
		  nexttimeout.tv_sec--;
		  nexttimeout.tv_usec+=1000000;
		}
	      if (ret==-1)
		{
		  if (errno!=EINTR)
		    MWError("select error on events");
		}
	      else if (ret==0)
		{
		  nexttimeout.tv_sec=app_resources.time_interval/1000;
		  nexttimeout.tv_usec=
		    1000*(app_resources.time_interval%1000);
		  if (app_resources.robotic==TRUE)
		    event->eventType=RandomEvent();
		  else {
		    event->eventType=EVENT_TIMEOUT;
//			std::cout << "TIMEOUT EVENT"<< std::endl;
//			std::cout << ctime(&newtime.tv_sec) << std::endl;
			}
		  return;
		}
	    }
	}
      else
	{
	  FD_ZERO(&fdmask);
	  FD_SET(socket, &fdmask);
	  timeout.tv_sec = 0;
	  timeout.tv_usec = 0;
	  while ((ret = select(32, &fdmask, NULL, NULL, &timeout)) == -1)
	    if (errno != EINTR)
	      MWError("select error on events");
	}
      if (XPending(dpy))
	{
	  XNextEvent(dpy, &xev);
	  switch (xev.type)
	    {
	    case KeyPress:
	      event->eventType = 0;
	      XLookupString((XKeyEvent *) &xev, &c, 1,
			    NULL, NULL);

	      switch(c)
		{
		case 'a':
		case '4':	/* keypad */
		  event->eventType = EVENT_A;
		  return;

		case 's':
		case '5':
		  event->eventType = EVENT_S;
		  return;

		case 'd':
		case '6':
		  event->eventType = EVENT_D;
		  return;

		case 'f':
		case ',':
		  event->eventType = EVENT_F;
		  return;

		case ' ':
		case '\033':	/* ESC lead in of arrow */
		  event->eventType = EVENT_BAR;
		  return;

		case 'q':
		case '\177':	/* DEL */
		case '\003':	/* ^C */
		  event->eventType = EVENT_INT;
		  return;
		}
	      break;

#define	RightButton	Button3
#define	MiddleButton	Button2
#define	LeftButton	Button1
	    case ButtonPress:
	      event->eventType = 0;
	      switch(((XButtonPressedEvent *)
		      &xev)->button & 0xff)
		{
		case RightButton:
		  event->eventType = EVENT_RIGHT_D;
		  return;

		case MiddleButton:
		  event->eventType = EVENT_MIDDLE_D;
		  return;

		case LeftButton:
		  event->eventType = EVENT_LEFT_D;
		  return;
		}
	      break;

	    case ButtonRelease:
	      event->eventType = 0;
	      switch(((XButtonReleasedEvent *)
		      &xev)->button&0xff)
		{
		case RightButton:
		  event->eventType = EVENT_RIGHT_U;
		  return;

		case LeftButton:
		  event->eventType = EVENT_LEFT_U;
		  return;
		}
	      break;

	    case Expose:
	      repaintWindow();
	      break;

	    case FocusIn:
	    case MapNotify:
	      mapped = TRUE;
	      iconInverted = FALSE;
	      flashIcon = FALSE;
	      repaintIcon();
	      break;

	    case FocusOut:
	    case UnmapNotify:
	      mapped = FALSE;
	      break;
	    }
	}

      if (FD_ISSET(socket, &fdmask))
	{
	  socklen_t fromLen = sizeof(event->eventSource);
	  int cc;

	  event->eventType = EVENT_NETWORK;
	  cc = recvfrom(socket, (char*)event->eventDetail,
			sizeof(MW244BPacket), 0,
		        (struct sockaddr *)&event->eventSource,
			&fromLen);
	  if (cc <= 0)
	    {
	      if (cc < 0 && errno != EINTR)
		perror("event recvfrom");
	      continue;
	    }
	  if (fromLen != sizeof(struct sockaddr_in))
	    continue;
	  ConvertIncoming(event->eventDetail);
	  return;
	}
    }
}

/* ----------------------------------------------------------------------- */

/*
 * Peek to see if there's a keyboard event waiting, in case the
 * program wants to short-circuit some code. If your system won't let
 * you implement this feature easily, just always return FALSE.
 */

bool
KBEventPending(void)
{
        return (XPending(dpy) != 0);
}

/* ----------------------------------------------------------------------- */

/* Please stand by ... */

void
HourGlassCursor(void)
{
	XDefineCursor(dpy, mwWindow, hourGlassCursor);
	XFlush(dpy);
}

/* ----------------------------------------------------------------------- */

/* Let the games begin! */
void
RatCursor(void)
{
	XDefineCursor(dpy, mwWindow, ratCursor);
	XFlush(dpy);
}

/* ----------------------------------------------------------------------- */

/* He's dead, Jim ... or might be */
void
DeadRatCursor(void)
{
	XDefineCursor(dpy, mwWindow, deadRatCursor);
	XFlush(dpy);
}

/* ----------------------------------------------------------------------- */

/*
 * Update the displayed bitmap. Would really like to store the arrow bitmaps
 * remotely, but the non-normal ones (especially otherArrows) may change.
 */

void
HackMazeBitmap(Loc x, Loc y, BitCell *newBits)
{
	arrowImage->data = (char *)newBits;
	XPutImage(dpy, mwWindow, copyGC, arrowImage, 0, 0,
			  x.value()*16 + MAZE_X_ORIGIN, y.value()*16 + MAZE_Y_ORIGIN,
			  16, 16);
}

/* ----------------------------------------------------------------------- */

/*
 * Display a rat.  Width, height, srcX and srcY describe the subbitmap in the
 * rats bitmap that is the desired view of the opponent.
 */

void
DisplayRatBitmap(int screenX, int screenY, int width, int height, int srcX, int srcY)
{
#ifdef	USE_BITMAPS
	XCopyPlane(dpy, ratsBitmap, mwWindow, xorGC, srcX, srcY,
		  width, height,
		  screenX+VIEW_X_ORIGIN, screenY+VIEW_Y_ORIGIN, 1);
#else
	XPutImage(dpy, mwWindow, xorGC, ratsImage, srcX, srcY,
		  screenX+VIEW_X_ORIGIN, screenY+VIEW_Y_ORIGIN,
		  width, height);
#endif	/* USE_BITMAPS */
}

/* ----------------------------------------------------------------------- */

/*
 * Display the score line for the indicated player. Name is left
 * justified, score is right justified within the score window.
 */

void
WriteScoreString(RatIndexType rat)
{
	char buf[64];
	int	leftEdge;

	sprintf(buf, "%d", (unsigned int) GetRatScore(rat).value());

	XClearArea(dpy, mwWindow, SCORE_X_ORIGIN,
		   SCORE_Y_ORIGIN +
		   rat.value() * (scoreFontInfo->max_bounds.ascent +
			  scoreFontInfo->max_bounds.descent),
		   SCORE_X_DIM,
		   (scoreFontInfo->max_bounds.ascent +
		    scoreFontInfo->max_bounds.descent),
		   FALSE);
	XDrawImageString(dpy, mwWindow, copyGC, SCORE_X_ORIGIN,
			 SCORE_Y_ORIGIN +
			 rat.value() * (scoreFontInfo->max_bounds.ascent +
				scoreFontInfo->max_bounds.descent) +
			 scoreFontInfo->max_bounds.ascent,
			 GetRatName(rat), strlen(GetRatName(rat)));
	leftEdge = SCORE_X_DIM - XTextWidth(scoreFontInfo, buf, strlen(buf));
	XDrawImageString(dpy, mwWindow, copyGC, leftEdge+SCORE_X_ORIGIN,
			 SCORE_Y_ORIGIN +
			 rat.value() * (scoreFontInfo->max_bounds.ascent +
				scoreFontInfo->max_bounds.descent) +
			 scoreFontInfo->max_bounds.ascent,
			 buf, strlen(buf));
}

/* ----------------------------------------------------------------------- */

/*
 * Clear out the score line for a player that's left the game.
 */
void
ClearScoreLine(RatIndexType rat)
{
	XClearArea(dpy, mwWindow, SCORE_X_ORIGIN,
		       SCORE_Y_ORIGIN +
		       rat.value() * (scoreFontInfo->max_bounds.ascent +
			      scoreFontInfo->max_bounds.descent),
		       SCORE_X_DIM,
		       scoreFontInfo->max_bounds.ascent +
		         scoreFontInfo->max_bounds.descent, False);
}

/* ----------------------------------------------------------------------- */

/*
 * Pretty obvious, eh? Means the guy's in sight.
 */
void
InvertScoreLine(RatIndexType rat)
{
	XFillRectangle(dpy, mwWindow, xorGC,
		  SCORE_X_ORIGIN,
		  SCORE_Y_ORIGIN +
		  rat.value() * (scoreFontInfo->max_bounds.ascent +
			 scoreFontInfo->max_bounds.descent),
		  SCORE_X_DIM,
		  scoreFontInfo->max_bounds.ascent +
		  scoreFontInfo->max_bounds.descent);
}

/* ----------------------------------------------------------------------- */

/*
 * Let the user know that a new player joined. This really should only
 * take effect if the window is iconic, so you can keep a game around
 * all day, close it up when there's no activity, and know when
 * someone else is also out to kill some time.
 */
void
NotifyPlayer(void)
{
	flashIcon = TRUE;
}

/*
 * END PUBLIC ROUTINES
 */

/* ----------------------------------------------------------------------- */

/* set up needed bitmaps in the server */
static void
initCursors(void)
{
        Pixmap	p, m;
	XColor	mc, bc;

	mc.pixel = app_resources.mouseground;
	bc.pixel = app_resources.bg_pixel;
	mc.flags = DoRed | DoGreen | DoBlue;
	bc.flags = DoRed | DoGreen | DoBlue;
	XQueryColor(dpy, DefaultColormap(dpy, screen), &mc);
	XQueryColor(dpy, DefaultColormap(dpy, screen), &bc);

	m = XCreateBitmapFromData(dpy, mwWindow,
		(char *)ratMask_bits,
		ratMask_width, ratMask_height);
	p = XCreateBitmapFromData(dpy, mwWindow,
		(char *)rat_bits,
		rat_width, rat_height);
	ratCursor = XCreatePixmapCursor(dpy, p, m, &mc, &bc,
		rat_x_hot, rat_y_hot);

	m = XCreateBitmapFromData(dpy, mwWindow,
		(char *)dRatMask_bits,
		dRatMask_width, dRatMask_height);
	p = XCreateBitmapFromData(dpy, mwWindow,
		(char *)dRat_bits,
		dRat_width, dRat_height);
	deadRatCursor = XCreatePixmapCursor(dpy, p, m, &mc, &bc, 0, 0);

	p = XCreateBitmapFromData(dpy, mwWindow,
		(char *)cup_bits,
		cup_width, cup_height);
	hourGlassCursor = XCreatePixmapCursor(dpy, p, p, &mc, &bc, 0, 0);
}

/* ----------------------------------------------------------------------- */

/*
 * construct an XImage of the maze.
 */
static void
initMaze()
{
	register int	i, j, k, line, index;

	for (i = 0; i < MAZEYMAX; i++) {
		line = i * MAZEXMAX * MAZEYMAX;
		for (j = 0; j < MAZEXMAX; j++) {
			index = line + j;
			for (k = 0; k < 16; k++) {
				if (M->maze_[j][i])
					mazeBits[index] = 0177777;
				else
					mazeBits[index] = 0;
				index += 32;
			}

		}
	}

#ifdef	USE_BITMAPS
	mazeBitmap = xCreateBitmapFromBitCell(dpy, mwWindow, (char *) mazeBits,
					MAZE_X_DIM, MAZE_Y_DIM);
	if (mazeBitmap == 0)
#else
	mazeImage = XCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
				1, XYBitmap, 0, mazeBits,
				MAZE_X_DIM, MAZE_Y_DIM, 8, MAZE_X_DIM>>3);
	mazeImage->byte_order = MSBFirst;
	mazeImage->bitmap_bit_order = MSBFirst;

	if (mazeImage == 0)
#endif	/* USE_BITMAPS */
		MWError("Can't create maze Pixmap");
}

/* ----------------------------------------------------------------------- */

/*
 * actually put the maze Pixmap on the screen.
 */
void
drawMaze()
{
#ifdef	USE_BITMAPS
	XCopyPlane(dpy, mazeBitmap, mwWindow, copyGC, 0, 0,
			MAZE_X_DIM, MAZE_Y_DIM,
			MAZE_X_ORIGIN, MAZE_Y_ORIGIN, 1);
#else
	XPutImage(dpy, mwWindow, copyGC, mazeImage, 0, 0,
			MAZE_X_ORIGIN, MAZE_Y_ORIGIN,
			MAZE_X_DIM, MAZE_Y_DIM);
#endif	/* USE_BITMAPS */
}

/* ----------------------------------------------------------------------- */

/*
 * Create the bitmap of the rats for later use.
 */
static void
initRats(int width, int height)
{
#ifdef	USE_BITMAPS
	ratsBitmap = xCreateBitmapFromBitCell(dpy, mwWindow, (char *) ratBits,
					width, height);
	if (ratsBitmap == 0)
#else
	ratsImage = XCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
				1, XYBitmap, 0, (char *) ratBits,
				width, height, 8, width>>3);
	ratsImage->byte_order = MSBFirst;
	ratsImage->bitmap_bit_order = MSBFirst;

	if (ratsImage == 0)
#endif	/* USE_BITMAPS */
		MWError("Can't create rats");
}

/* ----------------------------------------------------------------------- */

/*
 * Repaint the window for exposure and resize events. All drawing is
 * done here, even though there are times during the initialization
 * code when it would seem obvious to put some of the display up; this
 * way portions of the display aren't shown twice then.
 */

static void
repaintWindow(void)
{
	drawMaze();
	ShowPosition(MY_X_LOC, MY_Y_LOC, MY_DIR);
	ShowView(MY_X_LOC, MY_Y_LOC, MY_DIR); //draws maze
	ShowAllPositions();

	NewScoreCard();
	XFlush(dpy);
}

/* ----------------------------------------------------------------------- */

/*
 * Repaint the icon for exposure events, or while flashing to indicate
 * that there's a new player.
 */

static void
repaintIcon()
{
	XWMHints wmhints;

	if (!iconInverted)
		wmhints.icon_pixmap = icon_pixmap;
	else
		wmhints.icon_pixmap = icon_reverse_pixmap;
	wmhints.input = TRUE;
	wmhints.flags = IconPixmapHint | IconMaskHint | InputHint;
	wmhints.icon_mask = iconmask_pixmap;
	XSetWMHints(dpy, mwWindow, &wmhints);
}

/* ----------------------------------------------------------------------- */

/*
 * A hack of XCreateBitmapFromData that assumes MSBFirst BitCells insterad of
 * LSBFirst bytes.
 */

static Pixmap
xCreateBitmapFromBitCell(Display *display, Drawable d, char *data,
		unsigned int width, unsigned int height)
{
	XImage ximage;
	GC gc;
	Pixmap pix;

	pix = XCreatePixmap(display, d, width, height, 1);
	if (!pix)
	  return(0);
	gc = XCreateGC(display, pix, (unsigned long)0, (XGCValues *)0);
	memset(&ximage, 0, sizeof(ximage));
	ximage.height = height;
	ximage.width = width;
	ximage.depth = 1;
	ximage.xoffset = 0;
	ximage.format = ZPixmap;
	ximage.data = data;
	ximage.byte_order = MSBFirst;
	ximage.bitmap_unit = 8;
	ximage.bitmap_bit_order = MSBFirst;
	ximage.bitmap_pad = 8;
	ximage.bytes_per_line = (width+7)/8;
	XInitImage(&ximage);

	XPutImage(display, pix, gc, &ximage, 0, 0, 0, 0, width, height);
	XFreeGC(display, gc);
	return(pix);
}

/* ----------------------------------------------------------------------- */

/*
 * Draw string on specified location of the screen
 */
void DrawString(const char* msg, uint32_t length, uint32_t x, uint32_t y)
{
	XDrawImageString(dpy, mwWindow, copyGC, x, y, msg, length);
}


/* ----------------------------------------------------------------------- */

/*
 * Shut down the window system, resetting any terminal modes or what
 * have you that may have been altered. No-op for X11.
 */

void
StopWindow(void)
{
}

/* ----------------------------------------------------------------------- */
