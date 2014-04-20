/* $Header: mazewar.h,v 1.7 88/08/25 09:59:51 kent Exp $ */

/*
 * mazewar.h - Definitions for MazeWar
 *
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 * 		Digital Equipment Corporation
 * Date:	Wed Sep 24 1986
 */

/* Modified by Michael Greenwald for CS244B, Mar 1992,
   Greenwald@cs.stanford.edu */

/* Modified by Nicholas Dovidio for CS244B, Mar 2009,
 * ndovidio@stanford.edu
 * This version now uses the CS249a/b style of C++ coding.
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

#ifndef MAZEWAR_H
#define MAZEWAR_H


#include "fwk/NamedInterface.h"

#include "Nominal.h"
#include "Exception.h"
#include <string>

/* fundamental constants */

#ifndef	TRUE
#define	TRUE		1
#define	FALSE		0
#endif	/* TRUE */

/* You can modify this if you want to */
#define	MAX_RATS	8

/* network stuff */
/* Feel free to modify.  This is the simplest version we came up with */

/* A unique MAZEPORT will be assigned to your team by the TA */
#define	MAZEPORT	5008
/* The multicast group for Mazewar is 224.1.1.1 */
#define MAZEGROUP       0xe0010101
#define	MAZESERVICE	"mazewar244B"

/* The next two >must< be a power of two, because we subtract 1 from them
   to get a bitmask for random()
 */
#define	MAZEXMAX	32
#define	MAZEYMAX	16
#define	VECTORSIZE	55
#define	NAMESIZE  16
#define	NDIRECTION	4
#define	NORTH		0
#define	SOUTH		1
#define	EAST		2
#define	WEST		3
#define	NVIEW		4
#define	LEFT		0
#define	RIGHT		1
#define	REAR		2
#define	FRONT		3

#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))

#define MISSILE_COOL_SEC 2
#define MISSILE_UPDATE_MSEC 500
#define MIN_LOC_UPDATE_WAIT 100
#define MIN_DIR_SCORE_UPDATE_WAIT 200
#define PERIODIC_UPDATE_WAIT 500
#define SETTLE_DOWN_TIME 1000
#define DISCONNECT_TIME 2000
#define QUIT_TIME 10000
/* types */

typedef	struct sockaddr_in			Sockaddr;
typedef bool	               		MazeRow[MAZEYMAX];
typedef	MazeRow						MazeType [MAZEXMAX];
typedef	MazeRow						*MazeTypePtr;
//typedef	short						Direction;
typedef	struct {short	x, y; }		XYpoint;
typedef	struct {XYpoint	p1, p2;}	XYpair;
typedef	struct {short	xcor, ycor;}XY;
typedef	struct {unsigned short	bits[16];}	BitCell;
typedef	char						RatName[NAMESIZE];


 	class Direction : public Ordinal<Direction, short> {
	public:
		Direction(short num) : Ordinal<Direction, short>(num) {
			if(num<NORTH || num>NDIRECTION){
				throw RangeException("Error: Unexpected value.\n");
			}
		}
	};

 	class Loc : public Ordinal<Loc, short> {
	public:
		Loc(short num) : Ordinal<Loc, short>(num) {
			if(num<0){
				throw RangeException("Error: Unexpected negative value.\n");
			}
		}
	};

 	class Score : public Ordinal<Score, int> {
	public:
		Score(int num) : Ordinal<Score, int>(num) {}
	};


 	class RatIndexType : public Ordinal<RatIndexType, int> {
	public:
		RatIndexType(int num) : Ordinal<RatIndexType, int>(num) {
			if(num<0){
				throw RangeException("Error: Unexpected negative value.\n");
			}
		}
	};

 	class RatId : public Ordinal<RatId, unsigned short> {
	public:
		RatId(unsigned short num) : Ordinal<RatId, unsigned short>(num) {
		}
	};

 	class TokenId : public Ordinal<TokenId, long> {
	public:
		TokenId(long num) : Ordinal<TokenId, long>(num) {}
	};


class RatAppearance{

	public:
		RatAppearance() :  x(1), y(1), tokenId(0) {};
		bool	visible;
		Loc	x, y;
		short	distance;
		TokenId	tokenId;
};

class Rat{

public:
	Rat() :  playing(0), x(1), y(1), dir(NORTH), getName(false),
          score(0), pinned(false), statusSeqNum(0){};
	bool playing;
	Loc	x, y;
	Direction dir;
  RatName name;
  bool getName;
  Score score;
  bool pinned;
  unsigned int statusSeqNum;
  struct timeval lastUpdate;
};

typedef	RatAppearance			RatApp_type [MAX_RATS];
typedef	RatAppearance *			RatLook;

/* defined in display.c */
extern RatApp_type 			Rats2Display;

struct TagRat {
  unsigned int tagRatId;
  Loc xloc;
  Loc yloc;
  bool firstPktSend;
  bool secondPktSend;
  TagRat(int Id, unsigned short x, unsigned short y) : tagRatId(Id), 
    xloc(x), yloc(y), firstPktSend(false), secondPktSend(false) {}
};
/* missile class */
class Missile {
public: 
  Missile(unsigned int missileID, Loc x, Loc y, Direction dir, 
          struct timeval *curTime) : missileID_(missileID), xloc_(x), yloc_(y), 
          dir_(dir), hitWall_(false) {
    update_.tv_sec = curTime->tv_sec;
    update_.tv_usec = curTime->tv_usec;
    tagRats_.clear();
  }
  inline unsigned int missileID() const { return missileID_; }
  inline bool needUpdateLoc(struct timeval curTime) {
    return (curTime.tv_sec - update_.tv_sec) * 1000 
        + (curTime.tv_usec - update_.tv_usec)/1000 >= MISSILE_UPDATE_MSEC;
  }
  void updateMissileLoc(struct timeval);
  inline Direction dir() const {return dir_;}
  inline Loc xloc() const { return xloc_; }
  inline Loc yloc() const { return yloc_; }
  inline bool hitWall() const { return hitWall_; }
private:
  unsigned int missileID_;
  Loc xloc_, yloc_;
  Direction dir_;
  struct timeval update_;
  bool hitWall_;
public:
  list<struct TagRat> tagRats_;
  struct timeval lastReqTime_;
};

class PlayerShare {
public:
  PlayerShare(): statusSeqNum_(0), xloc_(0), yloc_(0), dir_(0) {}
  PlayerShare(uint32_t sockAddr, int socketID) : sockAddr_(sockAddr), 
    socketID_(socketID), statusSeqNum_(0), xloc_(0), yloc_(0), dir_(0) {}
  void updateName(char *name) {
    if(strlen(name) > 15) {
      strncpy(name_, name, 15);
      name_[15] = 0;
    } else
      strcpy(name_, name);
  }
  inline void updateStatus(unsigned int seqNum, Loc x, Loc y, Direction dir, 
                          int score, struct timeval curTime) 
  {
    lastUpdate_ = curTime;
    if(seqNum > statusSeqNum_ || seqNum > statusSeqNum_ + 5) {
      statusSeqNum_ = seqNum;
      xloc_ = x;
      yloc_ = y;
      dir_ = dir;
      score = score_;
    }
  }
  uint32_t sockAddr_;
  int socketID_;
  unsigned int statusSeqNum_;
  Loc xloc_;
  Loc yloc_;
  Direction dir_;
  int score_;
  bool invisible_;
  RatName name_;
  struct timeval lastUpdate_;
};

/* variables "exported" by the mazewar "module" */
class MazewarInstance :  public Fwk::NamedInterface  {
 public:
    typedef Fwk::Ptr<MazewarInstance const> PtrConst;
    typedef Fwk::Ptr<MazewarInstance> Ptr;

	static MazewarInstance::Ptr mazewarInstanceNew(string s){
      MazewarInstance * m = new MazewarInstance(s);
      return m;
    }

    inline Direction dir() const { return dir_; }
    void dirIs(Direction dir) { this->dir_ = dir; }
    inline Direction dirPeek() const { return dirPeek_; }
    void dirPeekIs(Direction dirPeek) { this->dirPeek_ = dirPeek; }

    inline long mazePort() const { return mazePort_; }
    void mazePortIs(long  mazePort) { this->mazePort_ = mazePort; }
    inline Sockaddr* myAddr() const { return myAddr_; }
    void myAddrIs(Sockaddr *myAddr) { this->myAddr_ = myAddr; }
    inline RatId myRatId() const { return myRatId_; }
    void myRatIdIs(RatId myRatId) { this->myRatId_ = myRatId; }

    inline bool peeking() const { return peeking_; }
    void peekingIs(bool peeking) { this->peeking_ = peeking; }
    inline int theSocket() const { return theSocket_; }
    void theSocketIs(int theSocket) { this->theSocket_ = theSocket; }
    inline Score score() const { return score_; }
    void scoreIs(Score score) { this->score_ = score; }
    inline Loc xloc() const { return xloc_; }
    void xlocIs(Loc xloc) { this->xloc_ = xloc; }
    inline Loc yloc() const { return yloc_; }
    void ylocIs(Loc yloc) { this->yloc_ = yloc; }
    inline Loc xPeek() const { return xPeek_; }
    void xPeekIs(Loc xPeek) { this->xPeek_ = xPeek; }
    inline Loc yPeek() const { return yPeek_; }
    void yPeekIs(Loc yPeek) { this->yPeek_ = yPeek; }
    inline int active() const { return active_; }
    void activeIs(int active) { this->active_ = active; }
    inline Rat rat(RatIndexType num) const { return mazeRats_[num.value()]; }
    inline Rat &updateRat(RatIndexType num) { return (Rat &)mazeRats_[num.value()]; }
    void ratIs(Rat rat, RatIndexType num) { this->mazeRats_[num.value()] = rat; }
    inline void getCurTime() { gettimeofday(&curTime_, NULL); }
    inline struct timeval curTime() { return curTime_; }
    inline bool canLaunch() { return (curTime_.tv_sec - nextLaunch_.tv_sec) * 1000 + (curTime_.tv_usec - nextLaunch_.tv_usec)/1000 >= 0; }
    inline void updateLaunchTime() {
      nextLaunch_.tv_sec = curTime_.tv_sec + MISSILE_COOL_SEC;
      nextLaunch_.tv_usec = curTime_.tv_usec;
    }
    bool launch();     
    MazeType maze_;
    RatName myName_;
    map<unsigned int, Missile> missiles_;
    unsigned int playerID;
    map<uint32_t, int> allPlayers_;
    inline void updateMoveTime() { moveTime_ = curTime_; }
    inline bool checkSettleDown() {
      return (curTime_.tv_sec-moveTime_.tv_sec)*1000
        +(curTime_.tv_usec-lastStatusTime_.tv_usec)/1000 > SETTLE_DOWN_TIME;
    }
    inline void updateStatusTime() { lastStatusTime_ = curTime_;  }
    inline bool checkSendStatusPKT(int minWaitTime) {
        return (curTime_.tv_sec-lastStatusTime_.tv_sec)*1000
        +(curTime_.tv_usec-lastStatusTime_.tv_usec)/1000 > minWaitTime;
    }
    list<pair<unsigned short, unsigned short> > preLoc;
    inline unsigned int updateStatusSeqNum() {
      if(xPre_.value() == xloc_.value() && yPre_.value() == yloc_.value() 
     && dirPre_.value() == dir_.value() && score_.value() == scorePre_.value())
        return nextSeqNum_;
      else {
        xPre_ = xloc_;
        yPre_ = yloc_;
        dirPre_ = dir_;
        scorePre_ = score_;
        return ++nextSeqNum_; 
      }
    }
protected:
	MazewarInstance(string s) : Fwk::NamedInterface(s), dir_(0), dirPeek_(0), myRatId_(0), score_(0),
		xloc_(1), yloc_(3), xPeek_(0), yPeek_(0), nextMissileID_(0), 
    nextSeqNum_(1), xPre_(1), yPre_(3), dirPre_(0), scorePre_(0) {
		myAddr_ = (Sockaddr*)malloc(sizeof(Sockaddr));
		if(!myAddr_) {
			printf("Error allocating sockaddr variable");
		}
    gettimeofday(&nextLaunch_, NULL);
	}
	Direction	dir_;
    Direction dirPeek_;

    long mazePort_;
    Sockaddr *myAddr_;
    Rat mazeRats_[MAX_RATS];
    RatId myRatId_;

    bool peeking_;
    int theSocket_;
    Score score_;
    Loc xloc_;
    Loc yloc_;
    Loc xPeek_;
    Loc yPeek_;
    int active_;
    unsigned int nextMissileID_;
    struct timeval curTime_;
    struct timeval nextLaunch_;
    struct timeval moveTime_;
    struct timeval lastStatusTime_;
    unsigned int nextSeqNum_;
    Loc xPre_;
    Loc yPre_;
    Direction dirPre_;
    Score scorePre_;
};
extern MazewarInstance::Ptr M;

/* defined all packets format */

#define PKT_RTJ 1
#define PKT_ATN 2
#define PKT_REQNAME 3
#define PKT_LOC_DIR_SCORE 5
#define PKT_RTT 6
#define PKT_ACK_TAG 7
#define PKT_RTB 8
#define PKT_RTE 9
PACK(
struct RTJPacket {
  unsigned int playerID;
  char nameLen;
  RatName name;
});

PACK(
struct PacketWithName {
  unsigned int playerID;
  char nameLen;
  RatName name;
});

struct PacketReqName {
  unsigned int playerID;
};
PACK(
struct PacketLocDirScore {
  unsigned int playerID;
  unsigned int statusSeqNum;
  char xloc;
  char yloc;
  char dir;
  int score;
});

PACK(
struct RTTPacket {
  unsigned int hitPlayerID;
  unsigned int shootPlayerID;
  char xloc;
  char yloc;
  unsigned int missileID;
});

PACK(
struct AckTagPacket {
  unsigned int shootPlayerID;
  unsigned int hitPlayerID;
  char tagged;
  unsigned int missileID;
});

PACK(
struct RTBPacket {
  unsigned int invaderID;
  char xloc;
  char yloc;
});

struct RTEPacket {
  unsigned int playerID;
};

#define MY_RAT_INDEX		0
#define MY_RAT_ID   M->myRatId().value()
#define MY_DIR			M->dir().value()
#define MY_X_LOC		M->xloc().value()
#define MY_Y_LOC		M->yloc().value()

/* events */

#define	EVENT_A		1		/* user pressed "A" */
#define	EVENT_S		2		/* user pressed "S" */
#define	EVENT_F		3		/* user pressed "F" */
#define	EVENT_D		4		/* user pressed "D" */
#define	EVENT_BAR	5		/* user pressed space bar */
#define	EVENT_LEFT_D	6		/* user pressed left mouse button */
#define	EVENT_RIGHT_D	7		/* user pressed right button */
#define	EVENT_MIDDLE_D	8		/* user pressed middle button */
#define	EVENT_LEFT_U	9		/* user released l.M.b */
#define	EVENT_RIGHT_U	10		/* user released r.M.b */

#define	EVENT_NETWORK	16		/* incoming network packet */
#define	EVENT_INT	17		/* user pressed interrupt key */
#define	EVENT_TIMEOUT	18		/* nothing happened! */

extern unsigned short	ratBits[];
/* replace this with appropriate definition of your own */
typedef	struct {
	unsigned char type;
	char	body[32];
}					MW244BPacket;

typedef	struct {
	short		eventType;
	MW244BPacket	*eventDetail;	/* for incoming data */
	Sockaddr	eventSource;
}					MWEvent;

void		*malloc();
Sockaddr	*resolveHost();

/* display.c */
void InitDisplay(int, char **);
void StartDisplay(void);
void ShowView(Loc, Loc, Direction);
void SetMyRatIndexType(RatIndexType);
void SetRatPosition(RatIndexType, Loc, Loc, Direction);
void ClearRatPosition(RatIndexType);
void ShowPosition(Loc, Loc, Direction);
void ShowAllPositions(void);
void showMe(Loc, Loc, Direction);
void clearPosition(RatIndexType, Loc, Loc);
void clearSquare(Loc xClear, Loc yClear);
void NewScoreCard(void);
void UpdateScoreCard(RatIndexType);
void FlipBitmaps(void);
void bitFlip(BitCell *, int size);
void SwapBitmaps(void);
void byteSwap(BitCell *, int size);
void
showMissile(Loc x_loc, Loc y_loc, Direction dir, Loc prev_x, Loc prev_y, bool clear);

/* init.c */
void MazeInit(int, char **);
void ratStates(bool rat_move, bool rat_dir_change);
void getMaze(void);
void setRandom(void);
void getName(char *, char **);
void getString(char *, char **);
void getHostName(char *, char **, Sockaddr *);
Sockaddr *resolveHost(char *);
bool emptyAhead();
bool emptyRight();
bool emptyLeft();
bool emptyBehind();

/* toplevel.c */
void play(void);
void aboutFace(void);
void leftTurn(void);
void rightTurn(void);
void forward(void);
void backward(void);
void peekLeft(void);
void peekRight(void);
void peekStop(void);
bool shoot(void);
void quit(int);
void NewPosition(MazewarInstance::Ptr M, int x=-1, int y=-1, int dir=-1);
void MWError(const char *);
Score GetRatScore(RatIndexType);
char  *GetRatName(RatIndexType);
void ConvertIncoming(MW244BPacket *);
void ConvertOutgoing(MW244BPacket *);
void ratStates(bool, bool, bool);
bool manageMissiles(void);
void sendRTTPacket(Missile & mi, uint32_t dstID);
void processRTTPacket(struct RTTPacket *);
void sendAckTagPacket(bool tagged, unsigned int shootPlayerID_n,
                      unsigned int missileID_n);
void processAckTagPacket(struct AckTagPacket *);
bool checkOtherLoc(Loc xloc, Loc yloc, vector<unsigned int> &sameloc);
void DoViewUpdate(void);
void sendPacketToPlayer(RatId);
void processPacket(MWEvent *);
void netInit(void);
void requestToJoin(void);
void sendRTJPacket(void);
void formPacketWithName(void *);
void addPlayerName(RatIndexType idx, size_t length, char *name);
void addPlayerName(unsigned int playerID, size_t length, char *name);
void processRTJPacket(void *);
void processATNPacket(struct PacketWithName *);
void sendStatus(void);
void sendRTBPacket(unsigned int, char, char);
void processRTBPacket(struct RTBPacket * );
void playerBackoff(void);
void processLocDirScore(struct PacketLocDirScore *);
void sendReqNamePacket(unsigned int);
void processReqNamePacket(struct PacketReqName *);
void processRTEPacket(struct RTEPacket *);
void sendMultiCast(MW244BPacket *);


/* winsys.c */
void InitWindow(int, char **);
void StartWindow(int, int);
void ClearView(void);
void DrawViewLine(int, int, int, int);
void NextEvent(MWEvent *, int);
bool KBEventPending(void);
void HourGlassCursor(void);
void RatCursor(void);
void DeadRatCursor(void);
void HackMazeBitmap(Loc, Loc, BitCell *);
void DisplayRatBitmap(int, int, int, int, int, int);
void WriteScoreString(RatIndexType);
void ClearScoreLine(RatIndexType);
void InvertScoreLine(RatIndexType);
void NotifyPlayer(void);
void DrawString(const char*, uint32_t, uint32_t, uint32_t);
void StopWindow(void);


#endif
