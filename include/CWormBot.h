/////////////////////////////////////////
//
//             OpenLieroX
//
// code under LGPL, based on JasonBs work,
// enhanced by Dark Charlie and Albert Zeyer
//
//
/////////////////////////////////////////



#ifndef __CWORMBOT_H__
#define __CWORMBOT_H__

#include "CWorm.h"

class CWormBotInputHandler : public CWormInputHandler {
public:
	CWormBotInputHandler(CWorm* w);
	virtual ~CWormBotInputHandler();
	
	virtual std::string name() { return "Bot input handler"; }
	
	virtual void initWeaponSelection();
	virtual void doWeaponSelectionFrame(SDL_Surface * bmpDest, CViewport *v);
	
	// simulation
	virtual void startGame() {}
	virtual void getInput(); 
    virtual void clearInput() {}

	virtual void onRespawn();
	
protected:
	
    /*
	 Artificial Intelligence
	 */
    int         nAIState;
    int         nAITargetType;
    CWorm       *psAITarget;
    CBonus      *psBonusTarget;
    CVec        cPosTarget;
    int         nPathStart[2];
    int         nPathEnd[2];
    CVec        cStuckPos;
    float       fStuckTime;
    bool        bStuck;
    float       fStuckPause;
    float       fLastThink;
    CVec        cNinjaShotPos;
	float		fLastFace;
	float		fBadAimTime;
    int			iAiGameType; // AI game type, we have predefined behaviour for mostly played settings
	int			iAiGame;
	int			iAiTeams;
	int			iAiTag;
	int			iAiVIP;
	int			iAiCTF;
	int			iAiTeamCTF;
	int			iAiDiffLevel;
	int			iRandomSpread;
	CVec		vLastShootTargetPos;
	
	float		fLastShoot;
	float		fLastJump;
	float		fLastWeaponChange;
	float		fLastCreated;
	float		fLastCompleting;
	float		fLastRandomChange;
	float		fLastGoBack;
	
	float		fCanShootTime;
	
	float		fRopeAttachedTime;
	float		fRopeHookFallingTime;
	
    // Path Finding
    int         nGridCols, nGridRows;
	float       fLastPathUpdate;
	bool		bPathFinished;
	float		fSearchStartTime;
	
	// its type is searchpath_base*; defined in CWorm_AI.cpp
	void*		pathSearcher;
	
	NEW_ai_node_t	*NEW_psPath;
	NEW_ai_node_t	*NEW_psCurrentNode;
	NEW_ai_node_t	*NEW_psLastNode;
	
	
public:
	
	
    //
    // AI
    //
    bool        AI_Initialize();
    void        AI_Shutdown(void);
	
	// TODO: what is the sense of all these parameters? (expect gametype)
	void		AI_Respawn();
	// TODO: what is the sense of all these parameters?
    void        AI_Think(int gametype, int teamgame, int taggame);
    bool        AI_FindHealth();
    bool        AI_SetAim(CVec cPos);
    CVec        AI_GetTargetPos(void);
	
    void        AI_InitMoveToTarget();
    void        AI_SimpleMove(bool bHaveTarget=true);
	//    void        AI_PreciseMove();
	
    void		AI_splitUpNodes(NEW_ai_node_t* start, NEW_ai_node_t* end);
    void		AI_storeNodes(NEW_ai_node_t* start, NEW_ai_node_t* end);
	
    int         AI_FindClearingWeapon();
    bool        AI_Shoot();
    int         AI_GetBestWeapon(int iGameMode, float fDistance, bool bDirect, float fTraceDist);
    void        AI_ReloadWeapons();
    int         cycleWeapons();
	void		AI_SetGameType(int type)  { iAiGameType = type; }
	int			AI_GetGameType()  { return iAiGameType; }
	
    CWorm       *findTarget(int gametype, int teamgame, int taggame);
	bool		weaponCanHit(int gravity,float speed, CVec cTrgPos);
	int			traceWeaponLine(CVec target, float *fDist, int *nType);
	
	
	CVec		AI_GetBestRopeSpot(CVec trg);
	CVec		AI_FindClosestFreeCell(CVec vPoint);
	bool		AI_CheckFreeCells(int Num);
	bool		AI_IsInAir(CVec pos, int area_a=3);
	CVec		AI_FindClosestFreeSpotDir(CVec vPoint, CVec vDirection, int Direction);
	CVec		AI_FindBestFreeSpot(CVec vPoint, CVec vStart, CVec vDirection, CVec vTarget, CVec* vEndPoint);
	int			AI_CreatePath(bool force_break = false);
	void		AI_MoveToTarget();
	CVec		AI_GetNearestRopeSpot(CVec trg);
	void		AI_Carve();
	bool		AI_Jump();
	CVec		AI_FindShootingSpot();
	int			AI_GetRockBetween(CVec pos,CVec trg, CMap *pcMap);
#ifdef _AI_DEBUG
	void		AI_DrawPath();
#endif
	
	
	
    //int         getBestWeapon(int iGameMode, float fDistance, CVec cTarget);
	
	
	
};

#endif  //  __CWORMBOT_H__