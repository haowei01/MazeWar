/* $Header: display.c,v 1.7 88/08/25 09:57:54 kent Exp $ */

/*
 * display.c - Display management routines for MazeWar
 *
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 *		Digital Equipment Corporation
 * Date:	Wed Oct  1 1986
 */

/* Modified by Michael Greenwald for Stanford University, CS340 */

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

#include "display.h"


static XYpair *plotLine(XYpair *, bool);
static XYpair *hidden(Loc, Loc, Direction, XYpair *);
static void TokenVisible(RatIndexType);
static void XORToken(RatIndexType);
static void getRat(RatIndexType, int *, int *, int *);

/* ----------------------------------------------------------------------- */

/*
 * Initialize the display manager.
 */
void
InitDisplay(int argc, char **argv)
{
	InitWindow(argc, argv);
}

/* ----------------------------------------------------------------------- */

/*
 * all in the name of portability... actually put the display on
 * the screen. Some window systems need to do initialization before
 * the net is started, but don't want to display until after.
 */
void
StartDisplay()
{
	StartWindow(ratBits_width, ratBits_height);
}

/* ----------------------------------------------------------------------- */

/*
 * Manage the top portion of the screen, the perspective view of the
 * maze, with the eyeballs. What is seen is controlled by the position
 * in the maze and what players are visible.
 *
 * The perspective view is calculated on the fly from the viewTable array.
 * viewTable contains a set of 12 line segments for 30 views. The longest
 * corridor is 30 spaces, and for each cell in the maze, there are 12
 * distinct lines (wall edges) that could be visible (never all at once).
 *
 * Each time, march down the hall in the direction faced and figure
 * out which lines to actually draw. Then look in Rats2Display and draw
 * in opponents.
 */

#define	NDIR	NDIRECTION
static bool	prevEdge3, prevEdge7;	/* plotter smarts */
static bool	edge1, edge2, edge3, edge4, edge5, edge6, edge7;
static XYpair	edge3Lines[2], edge7Lines[2];
static XY	l1Delta[NDIR] = { {0, -1}, {0, 1}, {1, 0}, {-1, 0} };
static XY	l2Delta[NDIR] = { {1, -1}, {-1, 1}, {1, 1}, {-1, -1} };
static XY	c2Delta[NDIR] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
static XY	r1Delta[NDIR] = { {0, 1}, {0, -1}, {-1, 0}, {1, 0} };
static XY	r2Delta[NDIR] = { {1, 1}, {-1, -1}, {-1, 1}, {1, -1} };
#undef	NDIR

void
ShowView(Loc x, Loc y, Direction dir)
{
	register XYpair		*tp = viewTable;
	register int		tx = x.value();
	register int		ty = y.value();
	RatIndexType			ratIndex(0);
	RatLook			ratLook;
	bool			oldVisible;

	ClearView();
	prevEdge3 = prevEdge7 = FALSE;
	while (!M->maze_[tx][ty]) {
		tp = hidden(tx, ty, dir, tp);	/* draw a cell */
		switch (dir.value()) {
		case NORTH:	tx++; break;
		case SOUTH:	tx--; break;
		case EAST:	ty++; break;
		case WEST:	ty--; break;
		}
	}
	if (prevEdge3)
		(void) plotLine(edge3Lines, TRUE);
	if (prevEdge7)
		(void) plotLine(edge7Lines, TRUE);

	/* show the tokens */

	for (ratIndex = 0; ratIndex < MAX_RATS; ratIndex = RatIndexType(ratIndex.value() + 1)) {
		if (ratIndex == MY_RAT_INDEX)
			continue;
		ratLook = &Rats2Display[ratIndex.value()];
		oldVisible = ratLook->visible;
		TokenVisible(ratIndex);
		if (ratLook->visible == TRUE)
			XORToken(ratIndex);
		if (ratLook->visible != oldVisible)
			UpdateScoreCard(ratIndex);
	}
}

/* ----------------------------------------------------------------------- */

static XYpair *
plotLine(XYpair	*p, bool two)
{
	while (1) {
		DrawViewLine(p->p1.x, p->p1.y, p->p2.x, p->p2.y);
		p++;
		if (!two)
			return p;
		two = FALSE;
	}
}

/* ----------------------------------------------------------------------- */

static XYpair*
hidden(Loc x, Loc y, Direction dir, XYpair *p)
{
	int	l1x, l1y, l2x, l2y;
	int	r1x, r1y, r2x, r2y;
	int	c2x, c2y;

	/* first calculate the coordinates of the neighboring cubes */

	l1x = x.value() + l1Delta[dir.value()].xcor;	/* find left cube */
	l1y = y.value() + l1Delta[dir.value()].ycor;
	l2x = x.value() + l2Delta[dir.value()].xcor;	/* find left forward cube */
	l2y = y.value() + l2Delta[dir.value()].ycor;
	r1x = x.value() + r1Delta[dir.value()].xcor;	/* find right cube */
	r1y = y.value() + r1Delta[dir.value()].ycor;
	r2x = x.value() + r2Delta[dir.value()].xcor;	/* find right forward cube */
	r2y = y.value() + r2Delta[dir.value()].ycor;
	c2x = x.value() + c2Delta[dir.value()].xcor;	/* find forward cube */
	c2y = y.value() + c2Delta[dir.value()].ycor;

	/* next calculate which of the 7 possible cube edges are visible */

	edge2 = M->maze_[c2x][c2y];	/* c2 */
	edge3 = M->maze_[l1x][l1y];	/* l1 */
	edge4 = !edge3;			/* !l1 */

	edge7 = M->maze_[r1x][r1y];	/* r1 */
	edge6 = !edge7;			/* !r1 */

	edge1 = (edge3 && (edge2 || !M->maze_[l2x][l2y]))
		|| ((!edge2) && edge4);
	edge5 = (edge7 && (edge2 || !M->maze_[r2x][r2y]))
		|| ((!edge2) && edge6);

	/*
	 * Should be matching the following:
	 *	x1 = l1 (c2 + !l2) + !c2 !l1
	 *	x2 = c2
	 *	x3 = l1
	 *	x4 = !l1
	 *	x5 = r1 (c2 + !r2) + !c2 !r1
	 *	x6 = !r1
	 *	x7 = r1
	 */

	if (edge1)
		p = plotLine(p, FALSE);
	else
		p++;
	if (edge2)
		p = plotLine(p, TRUE);
	else
		p += 2;
	if (edge3) {
		if (prevEdge3) {
			edge3Lines[0].p2 = (p++)->p2;
			edge3Lines[1].p2 = p->p2;
		} else {
			edge3Lines[0] = *p++;
			edge3Lines[1] = *p;
			prevEdge3 = TRUE;
		}
		p++;
	} else {
		if (prevEdge3) {
			(void) plotLine(edge3Lines, TRUE);
			prevEdge3 = FALSE;
		}
		p += 2;
	}
	if (edge4)
		p = plotLine(p, TRUE);
	else
		p += 2;
	if (edge5)
		p = plotLine(p, FALSE);
	else
		p++;
	if (edge6)
		p = plotLine(p, TRUE);
	else
		p += 2;
	if (edge7) {
		if (prevEdge7) {
			edge7Lines[0].p1 = (p++)->p1;
			edge7Lines[1].p1 = p->p1;
		} else {
			edge7Lines[0] = *p++;
			edge7Lines[1] = *p;
			prevEdge7 = TRUE;
		}
		p++;
	} else {
		if (prevEdge7) {
			(void) plotLine(edge7Lines, TRUE);
			prevEdge7 = FALSE;
		}
		p += 2;
	}
	return p;
}

/* ----------------------------------------------------------------------- */

/*
 * sets Rats2Display[hisRatIndex] variables
 */
static void
TokenVisible(RatIndexType hisRatIndex)
{
	RatLook		ratLook = &Rats2Display[hisRatIndex.value()];
	Rat rat = M->rat(hisRatIndex);
	Loc		tx(0), ty(0);
	Direction	td(0);
	int		ix, ix12;

	ratLook->visible = FALSE;
	if (!rat.playing)	return;
	if (M->peeking()) {
		tx = M->xPeek();
		ty = M->yPeek();
		td = M->dirPeek();
	} else {
		tx = MY_X_LOC;
		ty = MY_Y_LOC;
		td = MY_DIR;
	}
	ix = 0;
	while (!M->maze_[tx.value()][ty.value()]) {
		switch(td.value()) {
		case NORTH:	tx = Loc(tx.value()+1); break;
		case SOUTH:	tx = Loc(tx.value()-1); break;
		case EAST:	ty = Loc(ty.value()+1); break;
		case WEST:	ty = Loc(ty.value()-1); break;
		}
		ix++;
		if ((tx == rat.x) && (ty == rat.y)) {
			ratLook->visible = TRUE;
			ix12 = ix * 12;
			ratLook->x = (viewTable[ix12+3].p2.x +
				     viewTable[ix12+10].p1.x)/2;
			ratLook->y = (viewTable[ix12+3].p1.y +
				     viewTable[ix12+3].p2.y)/2;
			ratLook->tokenId = relativeTokens[td.value()][rat.dir.value()];
			ratLook->distance = ix;
			break;
		}
	}
}

/* ----------------------------------------------------------------------- */

/*
 * draw him into the maze, with the right size for his distance away
 * and facing the right way.
 */
static void
XORToken(RatIndexType hisRatIndex)
{
	int	size;
	int	srcX, srcY;

	getRat(hisRatIndex, &srcX, &srcY, &size);
	DisplayRatBitmap(Rats2Display[hisRatIndex.value()].x.value() - size/2,
			Rats2Display[hisRatIndex.value()].y.value()  - size/2,
			size, size, srcX, srcY);
}

/* ----------------------------------------------------------------------- */

/*
 * Dig the appropriate bitmap out of the master bitmap, which has all
 * possible views neatly packed into place.
 */
static void
getRat(RatIndexType hisRatIndex, int *x, int *y, int *size)
{
	int	viewT = LEFT;
	int	view = Rats2Display[hisRatIndex.value()].tokenId.value();

	switch (Rats2Display[hisRatIndex.value()].distance) {
		case 1:
			*size = 64;
			*x = 0;
			*y = 0;
			for (viewT = 0; viewT != view; *x += 64)
				viewT++;
			return;

		case 2:
			*size = 32;
			for (*x = 4*64; *x != 64*5; *x += 32)
				for (*y = 0; *y != 64; *y += 32)
					if (viewT == view)
						return;
					else
						viewT++;

		case 3:
			*size = 24;
			for (*x = 5*64; *x != 64*5+48; *x += 24)
				for (*y = 0; *y != 48; *y += 24)
					if (viewT == view)
						return;
					else
						viewT++;

		case 4:
		case 5:
			*size = 16;
			*x = 64*5+48;
			for (*y = 0; *y != 64; *y += 16)
				if (viewT == view)
					return;
				else
					viewT++;

		case 6:
		case 7:
		case 8:
			*size = 9;
			*y = 48;
			for (*x = 64*5; *x != 64*5 + 4*9; *x += 9)
				if (viewT == view)
					return;
				else
					viewT++;

		case 9:
		case 10:
		case 11:
		case 12:
			*size = 6;
			*y = 48+9;
			for (*x = 64*5; *x != 64*5 + 4*6; *x += 6)
				if (viewT == view)
					return;
				else
					viewT++;

		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
			*size = 4;
			*y = 48+9+3;
			for (*x = 64*5 + 4*6; *x != 64*5 +4*6 + 4*4; *x += 4)
				if (viewT == view)
					return;
				else
					viewT++;

		default:
			*size = 3;
			*y = 48+9;
			for (*x = 64*5 + 4*6; *x != 64*5 + 4*6 + 4*3; *x += 3)
				if (viewT == view)
					return;
				else
					viewT++;
	}
}

/* ----------------------------------------------------------------------- */

/*
 * The maze display. This is controlled by the clearArray and Rats2Display
 */
void
SetMyRatIndexType(RatIndexType ratIndex)
{
	Rat myRat;
	myRat = M->rat(ratIndex);
	myRat.playing = TRUE;
	M->ratIs(myRat, ratIndex);
}

/* ----------------------------------------------------------------------- */

/*
 * Someone joined us; add him to the display.
 */
void
SetRatPosition(RatIndexType ratIndex, Loc x_loc, Loc y_loc, Direction dir)
{
	Rat & newRat = M->updateRat(ratIndex);
	newRat.playing = TRUE;
	newRat.x = x_loc;
	newRat.y = y_loc;
	newRat.dir = dir;
//	M->ratIs(newRat, ratIndex);

	clearPosition(ratIndex, x_loc, y_loc);
}

/* ----------------------------------------------------------------------- */

/*
 * Someone left.
 */
void
ClearRatPosition(RatIndexType ratIndex)

{
	Rat leftRat = M->rat(ratIndex);

	clearPosition(ratIndex, leftRat.x, leftRat.y);
	leftRat.playing = FALSE;
	M->ratIs(leftRat, ratIndex);


}

/* ----------------------------------------------------------------------- */

/*
 * Tell the display controller I moved.
 */

void
ShowPosition(Loc x_loc, Loc y_loc, Direction tdir)
{
	Rat myRat = M->rat(MY_RAT_INDEX);
	clearPosition(MY_RAT_INDEX, myRat.x, myRat.y);
	showMe(x_loc, y_loc, tdir);
}

/* ----------------------------------------------------------------------- */

/*
 * Show where everybody is.  Only happens when window is exposed from scratch
 */

void
ShowAllPositions()
{
	RatIndexType	ratIndex(0);

	for (ratIndex = 0; ratIndex < MAX_RATS; ratIndex = RatIndexType(ratIndex.value() + 1)) {
		if (ratIndex == MY_RAT_INDEX)
			continue;
		else if (M->rat(ratIndex).playing)
			clearPosition(ratIndex, M->rat(ratIndex).x, M->rat(ratIndex).y);

	}
}

/* ----------------------------------------------------------------------- */

/*
 * Do the actual work of showing me with arrow in top-down view
 */
void
showMe(Loc x_loc, Loc y_loc, Direction dir)
{
	register BitCell	*bp;
	Rat		rp;

	bp = normalArrows;

	HackMazeBitmap(x_loc, y_loc, &bp[dir.value()]);


	rp = M->rat(MY_RAT_INDEX);

	rp.playing = TRUE;
	rp.x = x_loc;
	rp.y = y_loc;
	rp.dir = dir;
	M->ratIs(rp, MY_RAT_INDEX);
}

/* ----------------------------------------------------------------------- */

/*
 * When someone moves off a position, clear it out. If there's someone
 * "under" him, display the second guy.
 */
void
clearPosition(RatIndexType ratIndex, Loc xClear, Loc yClear)
{
	Rat	rp;
	register int		i;

	clearSquare(xClear, yClear);
	for (i = 0; i < MAX_RATS; i++) {
		rp = M->rat(i);
		if ((i == ratIndex.value()) || !(rp.playing))
			continue;
		if ((rp.x == xClear) && (rp.y == yClear))
			if (i == MY_RAT_INDEX)
			  showMe(rp.x, rp.y, rp.dir);
	}
}

/* ----------------------------------------------------------------------- */

static BitCell EMPTY = { {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} };

/* clear out a square of the maze */
void
clearSquare(Loc xClear, Loc yClear)
{
	HackMazeBitmap(xClear, yClear, &EMPTY);
}

/* ----------------------------------------------------------------------- */

/*
 * Handle the score card area. Most of the work is done in window system
 * dependent routines.
 */

void
NewScoreCard()
{
	register RatIndexType	ratIndex(0);

	for( ratIndex = 0; ratIndex < MAX_RATS; ratIndex = RatIndexType(ratIndex.value() +1))
		UpdateScoreCard(ratIndex);
}

/* ----------------------------------------------------------------------- */
void
UpdateScoreCard(RatIndexType ratIndex)
{
	ClearScoreLine(ratIndex);
	if (M->rat(ratIndex).playing)
		WriteScoreString(ratIndex);
	if (Rats2Display[ratIndex.value()].visible == TRUE){
		InvertScoreLine(ratIndex);
  }
}

/* ----------------------------------------------------------------------- */

/*
 * Some window systems (notably X10) choose to represent bitmaps in
 * little-endian order. That is, the least significant bit in the word is
 * the leftmost bit on the screen. This means that bitmaps look
 * "backwards" on the screen from the way they look in source code. So, we
 * flip them all here, once, at initialization time, so they can be edited
 * "by hand".
 *
 * It is up to the window system initialization code to call this, if
 * deemed necessary.
 *
 * Hack hack.
 */
void
FlipBitmaps()
{
	int i;

	bitFlip(normalArrows, 4);
	bitFlip(missile, 1);
	for (i = 0; i < ratBits_width * ratBits_height / 16; i += 64)
		bitFlip((BitCell *) &ratBits[i], 4);
}

/* ----------------------------------------------------------------------- */

/*
 * quick bit flipper -- divide and conquer due to Jeff Mogul.
 * unrolled to do size 16 bit words, but generally extensible.
 */
void
bitFlip(register BitCell *bits, int size)
{
	int	num, top, bot, i, j;

	for (i = 0; i < size; i++) {
		for (j = 0; j < 16; j++) {
			num = bits->bits[j] & 0xffff;
			top = num & 0xff00;
			bot = num & 0x00ff;
			top >>= 8;
			bot <<= 8;
			num = top|bot;

			top = num & 0xf0f0;
			bot = num & 0x0f0f;
			top >>= 4;
			bot <<= 4;
			num = top|bot;

			top = num & 0xcccc;
			bot = num & 0x3333;
			top >>= 2;
			bot <<= 2;
			num = top|bot;

			top = num & 0xaaaa;
			bot = num & 0x5555;
			top >>= 1;
			bot <<= 1;
			bits->bits[j] = top|bot;
		}
		bits++;
	}
}

/* ----------------------------------------------------------------------- */

/*
 * Then there are the systems that are perfectly happy with MSBFirst data, but
 * byte swap the shorts that are used.  We fix that here.
 *
 * The htons() routine is a defined to be a no-op on machines that don't have
 * this problem.
 *
 */
void
SwapBitmaps()
{
	int i;

	byteSwap(normalArrows, 4);
	byteSwap(missile, 1);
	for (i = 0; i < ratBits_width * ratBits_height / 16; i += 64)
		byteSwap((BitCell *) &ratBits[i], 4);
}

/* ----------------------------------------------------------------------- */

/*
 * Byte swap size 16x16 bitmaps.
 */
void
byteSwap(register BitCell *bits, int size)
{
	register int i, j;

	for (i = 0; i < size; i++, bits++)
		for (j = 0; j < 16; j++)
			bits->bits[j] = htons(bits->bits[j] & 0xffff);
}

/* ----------------------------------------------------------------------- */

/*
 * Displays a missile.  Takes the new location to display the missile in x_loc and y_loc
 * Takes the previous location of the missile to delete, if the clear boolean is set
 */
void
showMissile(Loc x_loc, Loc y_loc, Direction dir, Loc prev_x, Loc prev_y, bool clear)
{
	{
		register BitCell	*bp;

		bp = missile;
        if ( clear )
        	clearSquare(prev_x, prev_y);
		HackMazeBitmap(x_loc, y_loc, &bp[0]);
	}
}
