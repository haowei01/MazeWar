/*
 *   FILE: toplevel.c
 * AUTHOR: name (email)
 *   DATE: March 31 23:59:59 PST 2013
 *  DESCR:
 */

/* #define DEBUG */

#include "main.h"
#include <string>
#include "mazewar.h"

static bool		updateView;	/* true if update needed */
MazewarInstance::Ptr M;

/* Use this socket address to send packets to the multi-cast group. */
static Sockaddr         groupAddr;
#define MAX_OTHER_RATS  (MAX_RATS - 1)


int main(int argc, char *argv[])
{
    Loc x(1);
    Loc y(5);
    Direction dir(0);
    char *ratName;

    signal(SIGHUP, quit);
    signal(SIGINT, quit);
    signal(SIGTERM, quit);

    if(argc < 2) {
      getName((char *)("Welcome to CS244B MazeWar!\n\nYour Name"), &ratName);
      ratName[strlen(ratName) - 1] = 0;
    } else {
      printf("Welcome to CS244B MazeWar!\n\nYour Name: %s\n", argv[1]);
      ratName = (char *)malloc(sizeof(char)*NAMESIZE);
      strncpy(ratName, argv[1], NAMESIZE);
    }
    if(strlen(ratName) >= NAMESIZE )
      ratName[NAMESIZE-1] = 0;

    M = MazewarInstance::mazewarInstanceNew(string(ratName));
    MazewarInstance* a = M.ptr();
    strncpy(M->myName_, ratName, NAMESIZE);
    free(ratName);

    MazeInit(argc, argv);

    requestToJoin();
//    NewPosition(M);
    M->getCurTime();
    M->updateMoveTime();
    sendStatus();
    cout << "before play begin:"<< MY_X_LOC <<endl;
    updateView = TRUE;
    DoViewUpdate();

    /* So you can see what a Rat is supposed to look like, we create
    one rat in the single player mode Mazewar.
    It doesn't move, you can't shoot it, you can just walk around it */

    play();

    return 0;
}


/* ----------------------------------------------------------------------- */

void
play(void)
{
	MWEvent		event;
	MW244BPacket	incoming;

	event.eventDetail = &incoming;

	while (TRUE) {
		NextEvent(&event, M->theSocket());
    M->getCurTime();
    bool rat_move = false;
    bool rat_dir_change = false;
    bool rat_score_change = false; 
		if (!M->peeking())
			switch(event.eventType) {
			case EVENT_A:
				aboutFace();
        rat_dir_change = true;
				break;

			case EVENT_S:
				leftTurn();
        rat_dir_change = true;
				break;

			case EVENT_D:
				forward();
        rat_move = true;
				break;

			case EVENT_F:
				rightTurn();
        rat_dir_change = true;
				break;

			case EVENT_BAR:
				backward();
        rat_move = true;
				break;

			case EVENT_LEFT_D:
				peekLeft();
				break;

			case EVENT_MIDDLE_D:
				rat_score_change = shoot();
				break;

			case EVENT_RIGHT_D:
				peekRight();
				break;

			case EVENT_NETWORK:
				processPacket(&event);
				break;

			case EVENT_INT:
				quit(0);
				break;

			}
		else
			switch (event.eventType) {
			case EVENT_RIGHT_U:
			case EVENT_LEFT_U:
				peekStop();
				break;

			case EVENT_NETWORK:
				processPacket(&event);
				break;
			}

		ratStates(rat_move, rat_dir_change, rat_score_change);	/* clean house */

		bool needShowMe = manageMissiles();
    if(needShowMe)
      updateView = TRUE;

		DoViewUpdate();

		/* Any info to send over network? */

	}
}

/* ----------------------------------------------------------------------- */

static	Direction	_aboutFace[NDIRECTION] ={SOUTH, NORTH, WEST, EAST};
static	Direction	_leftTurn[NDIRECTION] =	{WEST, EAST, NORTH, SOUTH};
static	Direction	_rightTurn[NDIRECTION] ={EAST, WEST, SOUTH, NORTH};

void
aboutFace(void)
{
	M->dirIs(_aboutFace[MY_DIR]);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void
leftTurn(void)
{
	M->dirIs(_leftTurn[MY_DIR]);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void
rightTurn(void)
{
	M->dirIs(_rightTurn[MY_DIR]);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

/* remember ... "North" is to the right ... positive X motion */

void
forward(void)
{
	register int	tx = MY_X_LOC;
	register int	ty = MY_Y_LOC;

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[tx+1][ty])	tx++; break;
	case SOUTH:	if (!M->maze_[tx-1][ty])	tx--; break;
	case EAST:	if (!M->maze_[tx][ty+1])	ty++; break;
	case WEST:	if (!M->maze_[tx][ty-1])	ty--; break;
	default:
		MWError("bad direction in Forward");
	}
  vector<uint32_t> sameloc;
  if(checkOtherLoc(Loc(tx), Loc(ty), sameloc))
    return;
	if ((MY_X_LOC != tx) || (MY_Y_LOC != ty)) {
    M->preLoc.push_front(pair<uint16_t, uint16_t>(MY_X_LOC, MY_Y_LOC));
    if(M->preLoc.size() > MAX_RATS)
      M->preLoc.pop_back();
		M->xlocIs(Loc(tx));
		M->ylocIs(Loc(ty));
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void backward()
{
	register int	tx = MY_X_LOC;
	register int	ty = MY_Y_LOC;

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[tx-1][ty])	tx--; break;
	case SOUTH:	if (!M->maze_[tx+1][ty])	tx++; break;
	case EAST:	if (!M->maze_[tx][ty-1])	ty--; break;
	case WEST:	if (!M->maze_[tx][ty+1])	ty++; break;
	default:
		MWError("bad direction in Backward");
	}
	vector<uint32_t> sameloc;
  if(checkOtherLoc(Loc(tx), Loc(ty), sameloc))
    return;
  if ((MY_X_LOC != tx) || (MY_Y_LOC != ty)) {
		M->preLoc.push_front(pair<uint16_t, uint16_t>(MY_X_LOC, MY_Y_LOC));
    if(M->preLoc.size() > MAX_RATS)
      M->preLoc.pop_back();
    M->xlocIs(Loc(tx));
		M->ylocIs(Loc(ty));
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekLeft()
{
	M->xPeekIs(MY_X_LOC);
	M->yPeekIs(MY_Y_LOC);
	M->dirPeekIs(MY_DIR);

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[MY_X_LOC+1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC + 1);
				M->dirPeekIs(WEST);
			}
			break;

	case SOUTH:	if (!M->maze_[MY_X_LOC-1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC - 1);
				M->dirPeekIs(EAST);
			}
			break;

	case EAST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC+1]) {
				M->yPeekIs(MY_Y_LOC + 1);
				M->dirPeekIs(NORTH);
			}
			break;

	case WEST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC-1]) {
				M->yPeekIs(MY_Y_LOC - 1);
				M->dirPeekIs(SOUTH);
			}
			break;

	default:
			MWError("bad direction in PeekLeft");
	}

	/* if any change, display the new view without moving! */

	if ((M->xPeek() != MY_X_LOC) || (M->yPeek() != MY_Y_LOC)) {
		M->peekingIs(TRUE);
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekRight()
{
	M->xPeekIs(MY_X_LOC);
	M->yPeekIs(MY_Y_LOC);
	M->dirPeekIs(MY_DIR);

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[MY_X_LOC+1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC + 1);
				M->dirPeekIs(EAST);
			}
			break;

	case SOUTH:	if (!M->maze_[MY_X_LOC-1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC - 1);
				M->dirPeekIs(WEST);
			}
			break;

	case EAST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC+1]) {
				M->yPeekIs(MY_Y_LOC + 1);
				M->dirPeekIs(SOUTH);
			}
			break;

	case WEST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC-1]) {
				M->yPeekIs(MY_Y_LOC - 1);
				M->dirPeekIs(NORTH);
			}
			break;

	default:
			MWError("bad direction in PeekRight");
	}

	/* if any change, display the new view without moving! */

	if ((M->xPeek() != MY_X_LOC) || (M->yPeek() != MY_Y_LOC)) {
		M->peekingIs(TRUE);
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekStop()
{
	M->peekingIs(FALSE);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

bool MazewarInstance::launch() {
  if(canLaunch()) {
    missiles_.insert(pair<unsigned int, Missile>(nextMissileID_, 
                    Missile(nextMissileID_, xloc_, yloc_, dir_, &curTime_)));
    ++nextMissileID_;
    updateLaunchTime();
    return true;
  }
  return false;
}

//shoot, return true if score changes
bool shoot()
{
//  sendPacketToPlayer(M->myRatId());
  if(M->launch()) {
	  M->scoreIs( M->score().value()-1 );
	  UpdateScoreCard(M->myRatId().value());
    return true;
  } else {
    cout << "shoot no success"<<endl;
    return false;
  }
}

/* ----------------------------------------------------------------------- */

/*
 * Exit from game, clean up window
 */

void quit(int sig)
{
  MW244BPacket pack;
  pack.type = PKT_RTE;
  memset(pack.body, 0, sizeof(pack.body));
  struct RTEPacket *p = (struct RTEPacket *)&pack.body;
  p->playerID = htonl(M->playerID);
  if (sendto(M->theSocket(), &pack, sizeof(pack), 0,
      (const struct sockaddr*)&groupAddr , sizeof(Sockaddr)) < 0)
    { MWError("Sample error"); }

	StopWindow();
	exit(0);
}

void processRTEPacket(struct RTEPacket * RTE)
{
  unsigned int playerID = ntohl(RTE->playerID);
  if(M->allPlayers_.find(playerID) != M->allPlayers_.end()){
    RatIndexType idx(M->allPlayers_[playerID]);
    M->updateRat(idx).playing = false;
    M->updateRat(idx).pinned  = false;
    M->updateRat(idx).getName = false;
    M->updateRat(idx).statusSeqNum = 0;
    UpdateScoreCard(idx);
    M->allPlayers_.erase(playerID);
    updateView = TRUE;
  }
}

void newPlayerHandlePacket(MW244BPacket *pack){
  struct PacketWithName * pName;
  pName = (struct PacketWithName *) &(pack->body);
  switch (pack->type) {
    case PKT_ATN: 
      if(ntohl(pName->playerID) != M->playerID) {
//        cout << "New Player Handle ATN\n";
        processATNPacket((struct PacketWithName *)&(pack->body));
      }
      break;
    case PKT_LOC_DIR_SCORE:
      if(ntohl(pName->playerID) != M->playerID) {
//        cout << "New Player Hanlde status packet\n";
        processLocDirScore((struct PacketLocDirScore *)&(pack->body));
      }
      break;
    case PKT_REQNAME:
      if(ntohl(pName->playerID) == M->playerID) {
        processReqNamePacket((struct PacketReqName *)&(pack->body));
      }
      break;
    default: break;
  }
}

void requestToJoin()
{
  srand(time(NULL));
  unsigned int rand_val = rand();
  M->playerID = rand_val;
  M->allPlayers_[M->playerID] = 0; //self
  M->updateRat(RatIndexType(0)).pinned = true;
  sendRTJPacket();
  struct timeval joinTime;
  gettimeofday(&joinTime, NULL);

  MWEvent		event;
	MW244BPacket	incoming;

	event.eventDetail = &incoming;
//wait other players' reply
  while (TRUE) {
		NextEvent(&event, M->theSocket());
    M->getCurTime();
    if(event.eventType == EVENT_NETWORK) {
      newPlayerHandlePacket(event.eventDetail);
    }
    if(M->curTime().tv_sec-joinTime.tv_sec 
      +(M->curTime().tv_usec-joinTime.tv_usec)/1000000 >= 1.0) {
      cout << "Finish waiting"<<endl;
      break;
    }
  }
  vector<unsigned int> other;
  if(checkOtherLoc(M->xloc(), M->yloc(), other))
    NewPosition(M);
}

void formPacketWithName(void *packBody)
{
  struct PacketWithName * p = (struct PacketWithName *)packBody;
  p->playerID = htonl(M->playerID); //current player
  size_t length = strlen(M->myName_);
  if(length > 15 ){
    length = 15;
    M->myName_[length] = 0;
  }
  p->nameLen = char(length);
  strncpy(p->name, M->myName_, NAMESIZE);
}

/* Request To Join packet */
void sendRTJPacket()
{
  MW244BPacket pack;
  pack.type = 1;
  memset(pack.body, 0, sizeof(pack.body));
  formPacketWithName((void *) &(pack.body));
  
  if (sendto(M->theSocket(), &pack, sizeof(pack), 0,
      (const struct sockaddr*)&groupAddr , sizeof(Sockaddr)) < 0)
    { MWError("Sample error"); }
}

void addPlayerName(RatIndexType idx, size_t length, char *name)
{
  M->updateRat(idx).getName = true;
  if(length >= NAMESIZE)
    length = NAMESIZE - 1;
  strncpy(M->updateRat(idx).name, name, length);
//  printf("after copy %s\n", M->updateRat(idx).name);
  M->updateRat(idx).name[NAMESIZE-1] = 0;
}

void addPlayerName(unsigned int playerID, size_t length, char *name)
{
  if(M->allPlayers_.find(playerID) == M->allPlayers_.end()) {
    int i;
    for(i=1; i<MAX_RATS; ++i) {
      RatIndexType idx(i);
      if(!M->rat(idx).pinned) {
        M->allPlayers_[playerID] = i;
        M->updateRat(idx).playing = true;
        M->updateRat(idx).pinned = true;
        M->updateRat(idx).statusSeqNum = 0;
        addPlayerName(idx, length, (char *)(name));
        UpdateScoreCard(idx);
        break;
      }
    }
    if(i == MAX_RATS){
      cout << "Handle Me: player is full\n";
    }
  } else {
    RatIndexType idx(M->allPlayers_[playerID]);
    if(!M->rat(idx).getName){
      addPlayerName(idx, length, (char *)(name));
      UpdateScoreCard(idx);
    }
  }
}

void processRTJPacket(struct RTJPacket * RTJ)
{
  unsigned int playerID = ntohl(RTJ->playerID);
  cout << "playerID "<<playerID << endl;
  cout << "namelen " << int(RTJ->nameLen) << endl;
  cout << "name " << (RTJ->name) <<endl;
  addPlayerName(playerID, size_t(RTJ->nameLen), RTJ->name);
  //form reply packet with Name, ATN packet
  MW244BPacket pack;
  pack.type = 2;
  memset(pack.body, 0, sizeof(pack.body));
  formPacketWithName((void* )&(pack.body));
  if (sendto(M->theSocket(), &pack, sizeof(pack), 0,
    (const struct sockaddr*)&groupAddr , sizeof(Sockaddr)) < 0)
    { MWError("Sample error"); }
  //report current status
  sendStatus(); 
  //hack, see the Rat
}

void processATNPacket(struct PacketWithName * packName)
{
  unsigned int playerID = ntohl(packName->playerID);
  size_t length = size_t(packName->nameLen);
  addPlayerName(playerID, length, packName->name);
  // no need to further reply
}

/* ----------------------------------------------------------------------- */

void NewPosition(MazewarInstance::Ptr m, int x, int y, int dir_in)
{
	Loc newX(0);
	Loc newY(0);
  if(x>=0 && x < MAZEXMAX )
    newX = Loc(x);
  if(y>=0 && y < MAZEYMAX )
    newY = Loc(y);
	Direction dir(0); /* start on occupied square */

  vector<uint32_t> sameloc;
	while (M->maze_[newX.value()][newY.value()] ||
      checkOtherLoc(newX, newY, sameloc)) {
	  /* MAZE[XY]MAX is a power of 2 */
	  newX = Loc(random() & (MAZEXMAX - 1));
	  newY = Loc(random() & (MAZEYMAX - 1));
    sameloc.clear();
	  /* In real game, also check that square is
	     unoccupied by another rat */
	}

	/* prevent a blank wall at first glimpse */
  if(dir_in < 0 || dir_in > 3) {
	  if (!m->maze_[(newX.value())+1][(newY.value())]) dir = Direction(NORTH);
	  if (!m->maze_[(newX.value())-1][(newY.value())]) dir = Direction(SOUTH);
	  if (!m->maze_[(newX.value())][(newY.value())+1]) dir = Direction(EAST);
	  if (!m->maze_[(newX.value())][(newY.value())-1]) dir = Direction(WEST);
  } else 
    dir = Direction(dir_in);
	m->xlocIs(newX);
	m->ylocIs(newY);
	m->dirIs(dir);
}

/* ----------------------------------------------------------------------- */

void MWError(const char *s)

{
	StopWindow();
	fprintf(stderr, "CS244BMazeWar: %s\n", s);
	perror("CS244BMazeWar");
	exit(-1);
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
Score GetRatScore(RatIndexType ratId)
{
  if (ratId.value() == 	M->myRatId().value())
    { return(M->score()); }
  else { 
    return M->rat(ratId).score;
  }
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
char *GetRatName(RatIndexType ratId)
{
  if (ratId.value() ==	M->myRatId().value())
    { return(M->myName_); }
  else { //return (char *)("Dummy");
    // why it needs to be first convert to string then to c_str()?
//    printf("in get name, id is %d\n", ratId.value());
    return (char *)(string(M->rat(ratId).name).c_str());
  }
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own if necessary */
void ConvertIncoming(MW244BPacket *p)
{
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own if necessary */
void ConvertOutgoing(MW244BPacket *p)
{
}

/* ----------------------------------------------------------------------- */

void sendStatus(void)
{
  MW244BPacket pack;
  pack.type = PKT_LOC_DIR_SCORE;
  memset(pack.body, 0, sizeof(pack.body));
  struct PacketLocDirScore * p = (struct PacketLocDirScore *) &(pack.body);
  p->playerID = htonl(M->playerID);
  p->statusSeqNum = htonl(M->updateStatusSeqNum());
  p->xloc = char(MY_X_LOC);
  p->yloc = char(MY_Y_LOC);
  p->dir  = char(MY_DIR);
  p->score = htonl(M->score().value());
  if (sendto(M->theSocket(), &pack, sizeof(pack), 0,
    (const struct sockaddr*)&groupAddr , sizeof(Sockaddr)) < 0)
  { MWError("Sample error"); }
  M->updateStatusTime();
}

/* This is just for the sample version, rewrite your own */
void ratStates(bool rat_move, bool rat_dir_change, bool rat_score_change)
{
  /* In our sample version, we don't know about the state of any rats over
     the net, so this is a no-op */
  if(rat_move) {
    M->updateMoveTime();
    if(M->checkSendStatusPKT(MIN_LOC_UPDATE_WAIT))
      sendStatus();
  } else if((rat_dir_change || rat_score_change)
          && M->checkSendStatusPKT(MIN_DIR_SCORE_UPDATE_WAIT)) {
      sendStatus(); 
  } else if(M->checkSendStatusPKT(PERIODIC_UPDATE_WAIT))
    sendStatus();
  map<uint32_t, int>::iterator mapit;
  for(mapit = M->allPlayers_.begin(); mapit != M->allPlayers_.end(); ) {
    RatIndexType idx(mapit->second);
    if(idx.value() == 0){//self
      ++mapit;
      continue;
    }
    if(M->rat(idx).pinned) {
      struct timeval & t = M->updateRat(idx).lastUpdate;
      if(M->rat(idx).playing) {
        M->updateRat(idx).playing = (M->curTime().tv_sec - t.tv_sec) * 1000 
          + (M->curTime().tv_usec - t.tv_usec)/1000 < DISCONNECT_TIME;
        if(!M->updateRat(idx).playing) {
          cout << idx.value() <<" someone disconnected\n";
          updateView = TRUE;
        }
      } else 
        if((M->curTime().tv_sec - t.tv_sec) * 1000
                + (M->curTime().tv_usec - t.tv_usec)/1000 > QUIT_TIME){
          M->updateRat(idx).playing = false;
          M->updateRat(idx).pinned  = false;
          M->updateRat(idx).getName = false;
          M->updateRat(idx).statusSeqNum = 0;
          map<uint32_t, int>::iterator erase_it = mapit++;
          M->allPlayers_.erase(erase_it);
          UpdateScoreCard(idx);
          updateView = TRUE;
          continue;
        }
      ++mapit;
    } else {
      M->updateRat(idx).getName = false;
      map<uint32_t, int>::iterator erase_it = mapit++;
      M->allPlayers_.erase(erase_it);
      UpdateScoreCard(idx);
      updateView = TRUE;
    }
  }
}

void processLocDirScore(struct PacketLocDirScore * p){
  unsigned int playerID = ntohl(p->playerID);
  unsigned short xloc_v = p->xloc;
  unsigned short yloc_v = p->yloc;
  unsigned short dir = p->dir;
  unsigned int statusSeqNum = ntohl(p->statusSeqNum);
  int score = ntohl(p->score);
//  printf("%s, receive status update\n", GetRatName(RatIndexType(0)));
  if(M->allPlayers_.find(playerID) != M->allPlayers_.end()) {
    RatIndexType idx(M->allPlayers_[playerID]);
    M->updateRat(idx).playing = true;
    M->updateRat(idx).lastUpdate = M->curTime();
    if(!M->rat(idx).getName)
      sendReqNamePacket(playerID);
    if(statusSeqNum > M->rat(idx).statusSeqNum){
      M->updateRat(idx).statusSeqNum = statusSeqNum;
    } else {
//      cout << "duplicate status pkt"<<endl;
      return;
    }
    if(xloc_v == MY_X_LOC && yloc_v == MY_Y_LOC) {
      if(M->checkSettleDown())
        sendRTBPacket(p->playerID, p->xloc, p->yloc);
      else
        playerBackoff();
    }else if(xloc_v != M->rat(idx).x.value() || yloc_v != M->rat(idx).y.value() 
      || dir != M->rat(idx).y.value()) {
      SetRatPosition(idx, Loc(xloc_v), Loc(yloc_v), Direction(dir));
      updateView = TRUE;
    }
    if(score != M->rat(idx).score.value()){
      M->updateRat(idx).score = Score(score);
      UpdateScoreCard(idx);
    }
  } else {
    int i;
    for(i=1; i<MAX_RATS; ++i) {
      RatIndexType idx(i);
      if(!M->rat(idx).pinned) {
        M->allPlayers_[playerID] = i;
        M->updateRat(idx).playing = true;
        M->updateRat(idx).pinned  = true;
        M->updateRat(idx).getName = false;
        sendReqNamePacket(playerID);
        M->updateRat(idx).statusSeqNum = statusSeqNum;
        SetRatPosition(idx, Loc(xloc_v), Loc(yloc_v), Direction(dir));
        M->updateRat(idx).score = Score(score);
        updateView = TRUE;
        UpdateScoreCard(idx);
        M->updateRat(idx).lastUpdate = M->curTime();
        break;
      }
    }
    if(i == MAX_RATS){
      cout << "Handle Me: player is full\n";
    }
  }
}

void sendReqNamePacket(unsigned int playerID)
{
  MW244BPacket pack;
  pack.type = PKT_REQNAME;
  struct PacketReqName * REQ = (struct PacketReqName *)&pack.body;
  REQ->playerID = htonl(playerID);
  if (sendto(M->theSocket(), &pack, sizeof(pack), 0,
    (const struct sockaddr*)&groupAddr , sizeof(Sockaddr)) < 0)
  { MWError("Sample error"); }
}

void processReqNamePacket(struct PacketReqName * p)
{
  MW244BPacket pack;
  pack.type = PKT_ATN;
  memset(pack.body, 0, sizeof(pack.body));
  formPacketWithName((void* )&(pack.body));
  if (sendto(M->theSocket(), &pack, sizeof(pack), 0,
    (const struct sockaddr*)&groupAddr , sizeof(Sockaddr)) < 0)
    { MWError("Sample error"); }
}

void sendRTBPacket(unsigned int playerID_n, char xloc, char yloc)
{
  cout << "send RTB packet\n";
  MW244BPacket pack;
  pack.type = PKT_RTB;
  struct RTBPacket * RTB = (struct RTBPacket *)&pack.body;
  RTB->invaderID = playerID_n;
  RTB->xloc = xloc;
  RTB->yloc = yloc;
  if (sendto(M->theSocket(), &pack, sizeof(pack), 0,
    (const struct sockaddr*)&groupAddr , sizeof(Sockaddr)) < 0)
  { MWError("Sample error"); }
}

void playerBackoff()
{
  list<pair<uint16_t, uint16_t> >::iterator lit;
  for(lit = M->preLoc.begin(); lit != M->preLoc.end(); lit++){
    M->xlocIs(Loc(lit->first));
    M->ylocIs(Loc(lit->second));
    M->preLoc.erase(lit);
    updateView = TRUE;
    break;
  }
}

void processRTBPacket(struct RTBPacket *p)
{
  cout << "handle RTB packet\n";
  unsigned short xloc_v = p->xloc;
  unsigned short yloc_v = p->yloc;
  if(xloc_v != MY_X_LOC || yloc_v != MY_Y_LOC)
    return;
  playerBackoff();
} 

/* ----------------------------------------------------------------------- */

void Missile::updateMissileLoc(struct timeval curTime)
{
  register int mx = xloc_.value();
  register int my = yloc_.value();
  switch(dir_.value()) {
	  case EAST: if(!M->maze_[mx][my+1]) my ++ ;
               else hitWall_ = true;		break;
	  case WEST: if(!M->maze_[mx][my-1]) my -- ;		
               else hitWall_ = true;  break;
	  case NORTH: if(!M->maze_[mx+1][my]) mx ++ ;  
               else hitWall_ = true;  break;
	  case SOUTH: if(!M->maze_[mx-1][my]) mx -- ;	
               else hitWall_ = true;  break;
	  default: MWError("bad direction in shoot");
  }
  update_ = curTime;
  if(!hitWall_) {
    xloc_ = Loc(mx);
    yloc_ = Loc(my);
    //on the new location, try to see if it tag some player
  } 
}

// Check for all other player who is in the game, if there location is this
// Return true if there is someone in this location, and push the player ID
// back in the vector
bool checkOtherLoc(Loc xloc, Loc yloc, vector<uint32_t> &sameloc){
  map<uint32_t, int>::iterator mapit;
  for(mapit= M->allPlayers_.begin(); mapit!= M->allPlayers_.end(); ++mapit){
    if(mapit->first == M->playerID)
      continue;
    RatIndexType idx(mapit->second);
    if(!M->rat(idx).playing)
      continue;
    if(M->rat(idx).x.value() == xloc.value() 
      && M->rat(idx).y.value() == yloc.value()){
      sameloc.push_back(mapit->first);
    }
  }
  if(sameloc.size())
    return true;
  else
    return false;
}

void sendRTTPacket(Missile & mi, uint32_t dstID)
{
  printf("send RTT packet\n");
  MW244BPacket pack;
  pack.type = PKT_RTT;
//  memset(pack.body, 0, sizeof(pack.body)); 
  struct RTTPacket * RTT = (struct RTTPacket *) &pack.body;
  RTT->hitPlayerID = htonl(dstID);
  RTT->shootPlayerID = htonl(M->playerID);
  RTT->xloc = char(mi.xloc().value());
  RTT->yloc = char(mi.yloc().value());
  RTT->missileID = htonl(mi.missileID());
  if (sendto(M->theSocket(), &pack, sizeof(pack), 0,
    (const struct sockaddr*)&groupAddr , sizeof(Sockaddr)) < 0)
  { MWError("Sample error"); }

}

void sendAckTagPacket(bool tagged, unsigned int shootPlayerID_n, 
                      unsigned int missileID_n)
{
  MW244BPacket pack;
  pack.type = PKT_ACK_TAG;
  memset(pack.body, 0, sizeof(pack.body));
  struct AckTagPacket * AckTag = (struct AckTagPacket *) &pack.body;
  AckTag->shootPlayerID = shootPlayerID_n;
  AckTag->hitPlayerID = htonl(M->playerID);
  AckTag->tagged = char(tagged);
  AckTag->missileID = missileID_n;
  if (sendto(M->theSocket(), &pack, sizeof(pack), 0,
    (const struct sockaddr*)&groupAddr , sizeof(Sockaddr)) < 0)
  { MWError("Sample error"); }
}

void processRTTPacket(struct RTTPacket * p)
{
  unsigned short px = (unsigned short)(p->xloc);
  unsigned short py = (unsigned short)(p->yloc);
  if(px == MY_X_LOC && py ==MY_Y_LOC){
    sendAckTagPacket(true, p->shootPlayerID, p->missileID);
    cout << "I am hit" <<endl;
    M->scoreIs( M->score().value()- 5 );
	  UpdateScoreCard(M->myRatId().value());
    //need to respawn
    NewPosition(M);
    sendStatus(); 
    updateView = true; 
  } else 
    sendAckTagPacket(false, p->shootPlayerID, p->missileID);
}

void processAckTagPacket(struct AckTagPacket * AckTag)
{
  bool tagged = (bool)(AckTag->tagged);
  unsigned int hitPlayerId = ntohl(AckTag->hitPlayerID);
//  cout << "Hit Player ID "<< hitPlayerId <<endl;
  unsigned int missileID = ntohl(AckTag->missileID);
//  cout << "Receive AckTagPacket" <<endl;
  if(M->missiles_.find(missileID) == M->missiles_.end()){
//    printf("This missile is out\n");
    return;
  }
  Missile & mi = M->missiles_.find(missileID)->second;
  list<struct TagRat>::iterator lit;
  for(lit = mi.tagRats_.begin(); lit != mi.tagRats_.end(); lit++)
    if(lit ->tagRatId == hitPlayerId )
      break;
  if(lit == mi.tagRats_.end()) {
    printf( "Not in the tag queue\n");
    return;
  }
  if(tagged){
    M->scoreIs( M->score().value()+11 );
    UpdateScoreCard(M->myRatId().value());
    sendStatus();
    clearSquare(mi.xloc(), mi.yloc());
    mi.tagRats_.clear();
    M->missiles_.erase(missileID);
  } else 
    mi.tagRats_.erase(lit);
}

void setMissileInitialReqTime(Missile & mi){
  mi.lastReqTime_ = M->curTime();
  if(mi.lastReqTime_.tv_usec/1000 > 200)
    mi.lastReqTime_.tv_usec -= 200000;
  else{
    mi.lastReqTime_.tv_sec -= 1;
    mi.lastReqTime_.tv_usec += 800000;
  }
}

inline bool AckTagTimeOut(Missile & mi){
  if((M->curTime().tv_sec - mi.lastReqTime_.tv_sec) * 1000 
  + (M->curTime().tv_usec - mi.lastReqTime_.tv_usec)/1000 > 200)
    return true;
  else
    return false;
}

/* This is just for the sample version, rewrite your own */
bool manageMissiles()
{
  /* Leave this up to you. */
  map<unsigned int, Missile>::iterator mapit;
  map<unsigned int, Missile>::iterator mapit_erase;
  bool needShowMe = false;
//  bool sendMissileReq = false;
  for(mapit = M->missiles_.begin(); mapit != M->missiles_.end(); ) {
    Missile & mi = mapit->second;
    if (!mi.hitWall() && mi.needUpdateLoc(M->curTime())) {
      Loc preX(mi.xloc());
      Loc preY(mi.yloc());
      mi.updateMissileLoc(M->curTime());  
      if(!mi.hitWall()){
        showMissile(mi.xloc(), mi.yloc(), mi.dir(), preX, preY, true);
        if(preX.value() == MY_X_LOC && preY.value() == MY_Y_LOC)
          needShowMe = true; 
      } else {
        clearSquare(mi.xloc(), mi.yloc());
        if(mi.xloc().value() == MY_X_LOC && mi.yloc().value() == MY_Y_LOC)
          needShowMe = true; 
      }
    } 
    vector<uint32_t> sameloc;
    sameloc.clear();
    //if the first hitRatTag pushed in the queue, can immediate send RTT
    if(!mi.tagRats_.size())
      setMissileInitialReqTime(mi);       
    if(checkOtherLoc(mi.xloc(), mi.yloc(), sameloc)) { 
      for(size_t i = 0; i < sameloc.size(); ++i){
        struct TagRat tryTag(sameloc[i], mi.xloc().value(), mi.yloc().value());
        mi.tagRats_.push_back(tryTag);
      }
    }
 
    if(mi.tagRats_.size() && AckTagTimeOut(mi)){
      if(!mi.tagRats_.front().firstPktSend 
        || !mi.tagRats_.front().secondPktSend) {
        
        sendRTTPacket(mi, mi.tagRats_.front().tagRatId);
        mi.tagRats_.front().firstPktSend = true;
//        sendMissileReq = true;
        mi.lastReqTime_ = M->curTime();
        if(!mi.tagRats_.front().firstPktSend)
          mi.tagRats_.front().firstPktSend = true;
        else
          mi.tagRats_.front().secondPktSend = true;
      } else 
          mi.tagRats_.pop_front();
    }
    if(mi.hitWall() && !mi.tagRats_.size()) {
      mapit_erase = mapit++;
      M->missiles_.erase(mapit_erase);
    } else 
      mapit++;
  }
//  if(sendMissileReq)
//    printf( "finish manage missile\n");
  return needShowMe;
}

/* ----------------------------------------------------------------------- */

void DoViewUpdate()
{
	if (updateView) {	/* paint the screen */
		ShowPosition(MY_X_LOC, MY_Y_LOC, MY_DIR);
		if (M->peeking())
			ShowView(M->xPeek(), M->yPeek(), M->dirPeek());
		else
			ShowView(MY_X_LOC, MY_Y_LOC, MY_DIR);
		updateView = FALSE;
	}
}

/* ----------------------------------------------------------------------- */

/*
 * Sample code to send a packet to a specific destination
 */

/*
 * Notice the call to ConvertOutgoing.  You might want to call ConvertOutgoing
 * before any call to sendto.
 */

void sendPacketToPlayer(RatId ratId)
{
  MW244BPacket pack;
  pack.type = 0;
  memset(pack.body, 0, sizeof(pack.body));
  strcpy((char *)&pack.body, "Hello World\n");
  if (sendto(M->theSocket(), &pack, sizeof(pack), 0,
      (const struct sockaddr*)&groupAddr , sizeof(Sockaddr)) < 0)
    { MWError("Sample error"); }

/*
	MW244BPacket pack;
	DataStructureX *packX;

	pack.type = PACKET_TYPE_X;
	packX = (DataStructureX *) &pack.body;
	packX->foo = d1;
	packX->bar = d2;

        ....

	ConvertOutgoing(pack);

	if (sendto((int)mySocket, &pack, sizeof(pack), 0,
		   (Sockaddr) destSocket, sizeof(Sockaddr)) < 0)
	  { MWError("Sample error") };
*/
}

/* ----------------------------------------------------------------------- */

/* Sample of processPacket. */

void processPacket (MWEvent *eventPacket)
{
//  cout << "Packet received"<<endl;
  MW244BPacket * pack = eventPacket->eventDetail;
  char * message;
  struct RTJPacket *RTJ;
  struct PacketWithName *packName;
  struct PacketReqName *ReqName;
  struct PacketLocDirScore *LocDirScore;
  struct RTTPacket *RTT;
  struct AckTagPacket *AckTag;
  struct RTBPacket *RTB;
//  uint32_t invaderID;
  struct RTEPacket *RTE;
  switch(pack->type) {
    case 0:  cout << "received test packet\n";
          message = (char *)&(pack->body);
          cout << message <<endl; break; 
    case PKT_RTJ: RTJ = (struct RTJPacket *)&(pack->body);
            if(M->playerID != ntohl(RTJ->playerID))
              processRTJPacket(RTJ);  
            break;
    case PKT_ATN: packName = (struct PacketWithName *)&(pack->body);
            if(M->playerID != ntohl(packName->playerID))
              processATNPacket(packName); 
            break;
    case PKT_REQNAME: ReqName = (struct PacketReqName *)&(pack->body);
            if(M->playerID == ntohl(ReqName->playerID))
              processReqNamePacket(ReqName);
            break; 
    case PKT_LOC_DIR_SCORE:
            LocDirScore = (struct PacketLocDirScore *)&(pack->body);
            if(M->playerID != ntohl(LocDirScore->playerID))
              processLocDirScore(LocDirScore);
            break;
    case PKT_RTT: RTT = (struct RTTPacket *)&(pack->body);
                  if(ntohl(RTT->hitPlayerID) == M->playerID)
                    processRTTPacket(RTT);
                  break;
    case PKT_ACK_TAG: AckTag = (struct AckTagPacket *)&(pack->body);
                  if(ntohl(AckTag->shootPlayerID) == M->playerID)
                    processAckTagPacket(AckTag);
                  break;
    case PKT_RTB: RTB = (struct RTBPacket *)&(pack->body);
                  if(ntohl(RTB->invaderID) == M->playerID)
                    processRTBPacket(RTB);
                  break;
    case PKT_RTE: RTE = (struct RTEPacket *)&(pack->body);
                  processRTEPacket(RTE);
                  break;
    default: cout <<"cannot hanle"<<endl;
  } 
/*
	MW244BPacket		*pack = eventPacket->eventDetail;
	DataStructureX		*packX;

	switch(pack->type) {
	case PACKET_TYPE_X:
	  packX = (DataStructureX *) &(pack->body);
	  break;
        case ...
	}
*/

}

/* ----------------------------------------------------------------------- */

/* This will presumably be modified by you.
   It is here to provide an example of how to open a UDP port.
   You might choose to use a different strategy
 */
void
netInit()
{
	Sockaddr		nullAddr;
	Sockaddr		*thisHost;
	char			buf[128];
	int				reuse;
	u_char          ttl;
	struct ip_mreq  mreq;

	/* MAZEPORT will be assigned by the TA to each team */
	M->mazePortIs(htons(MAZEPORT));

	gethostname(buf, sizeof(buf));
  cout << buf <<endl;
	if ((thisHost = resolveHost(buf)) == (Sockaddr *) NULL)
	  MWError("who am I?");
	bcopy((caddr_t) thisHost, (caddr_t) (M->myAddr()), sizeof(Sockaddr));

	char ip4[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(M->myAddr()->sin_addr), ip4, INET_ADDRSTRLEN);
	std::cout << ip4 << std::endl;

	M->theSocketIs(socket(AF_INET, SOCK_DGRAM, 0));
	if (M->theSocket() < 0)
	  MWError("can't get socket");

	/* SO_REUSEADDR allows more than one binding to the same
	   socket - you cannot have more than one player on one
	   machine without this */
	reuse = 1;
	if (setsockopt(M->theSocket(), SOL_SOCKET, SO_REUSEADDR, &reuse,
		   sizeof(reuse)) < 0) {
		MWError("setsockopt failed (SO_REUSEADDR)");
	}

	nullAddr.sin_family = AF_INET;
	nullAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	nullAddr.sin_port = M->mazePort();
	if (bind(M->theSocket(), (struct sockaddr *)&nullAddr,
		 sizeof(nullAddr)) < 0)
	  MWError("netInit binding");

	/* Multicast TTL:
	   0 restricted to the same host
	   1 restricted to the same subnet
	   32 restricted to the same site
	   64 restricted to the same region
	   128 restricted to the same continent
	   255 unrestricted

	   DO NOT use a value > 32. If possible, use a value of 1 when
	   testing.
	*/

	ttl = 1;
	if (setsockopt(M->theSocket(), IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
		   sizeof(ttl)) < 0) {
		MWError("setsockopt failed (IP_MULTICAST_TTL)");
	}

	/* join the multicast group */
	mreq.imr_multiaddr.s_addr = htonl(MAZEGROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(M->theSocket(), IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)
		   &mreq, sizeof(mreq)) < 0) {
		MWError("setsockopt failed (IP_ADD_MEMBERSHIP)");
	}

	/*
	 * Now we can try to find a game to join; if none, start one.
	 */
	 
	printf("\n");

	/* set up some stuff strictly for this local sample */
	M->myRatIdIs(0);
	M->scoreIs(0);
	SetMyRatIndexType(0);

	/* Get the multi-cast address ready to use in SendData()
           calls. */
	memcpy(&groupAddr, &nullAddr, sizeof(Sockaddr));
	groupAddr.sin_addr.s_addr = htonl(MAZEGROUP);

	inet_ntop(AF_INET, &(groupAddr.sin_addr), ip4, INET_ADDRSTRLEN);
	std::cout << ip4 << std::endl;
}


/* ----------------------------------------------------------------------- */
