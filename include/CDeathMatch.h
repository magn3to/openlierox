/*
	OpenLieroX

	death match gamemode

	created 2009-02-09
	code under LGPL
*/

#ifndef __CDEATHMATCH_H__
#define __CDEATHMATCH_H__

#include "CGameMode.h"
#include "Consts.h"

class CDeathMatch : public CGameMode {
private:
	bool CompareWormsScore(CWorm *w1, CWorm *w2);
public:
	CDeathMatch(GameServer* server, CWorm* worms);
	virtual ~CDeathMatch();
	
	virtual void PrepareGame();
	virtual void PrepareWorm(CWorm* worm);
	virtual bool Spawn(CWorm* worm, CVec pos);
	virtual void Kill(CWorm* victim, CWorm* killer);
	virtual bool Shoot(CWorm* worm);
	virtual void Drop(CWorm* worm);
	virtual void Simulate();
	virtual bool CheckGame();
	virtual int  GameType();
	virtual int  GameTeams();
	virtual int  Winner();

protected:
	bool bFirstBlood;
	int  iKillsInRow[MAX_WORMS];
	int  iDeathsInRow[MAX_WORMS];
	int  iWinner;
};

#endif
