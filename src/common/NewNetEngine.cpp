
#include <string>
#include <limits.h>
#include <time.h>

#include "NewNetEngine.h"
#include "MathLib.h"
#include "Sounds.h"
#include "FindFile.h"
#include "CBytestream.h"
#include "CClient.h"
#include "CMap.h"
#include "CWorm.h"
#include "ProfileSystem.h"
#include "CWormHuman.h"
#include "Entity.h"
#include "CServer.h"

namespace NewNet
{

// -------- The stuff that interacts with OLX: save/restore game state and calculate physics ---------

CWorm * SavedWormState = NULL;
NetSyncedRandom netRandom, netRandom_Saved;

void SaveState()
{
	netRandom_Saved = netRandom;
	cClient->getMap()->NewNet_SaveToMemory();

	cClient->NewNet_SaveProjectiles();
	NewNet_SaveEntities();

	for( int i = 0; i < MAX_WORMS; i++ )
		cClient->getRemoteWorms()[i].NewNet_SaveWormState( &SavedWormState[i] );
};

void RestoreState()
{
	netRandom = netRandom_Saved;
	cClient->getMap()->NewNet_RestoreFromMemory();

	cClient->NewNet_LoadProjectiles();
	NewNet_LoadEntities();

	for( int i = 0; i < MAX_WORMS; i++ )
		cClient->getRemoteWorms()[i].NewNet_RestoreWormState( &SavedWormState[i] );
};

// TODO: make respawning server-sided and remove this function
static CVec NewNet_FindSpot(CWorm *Worm) // Avoid name conflict with CServer::FindSpot
{
	// This should use net synced Worm->NewNet_random for now, later I'll use respawn routines from CServer
	CMap * cMap = cClient->getMap();
	int	 x, y;
	int	 px, py;
	int	 cols = cMap->getGridCols() - 1;	   // Note: -1 because the grid is slightly larger than the
	int	 rows = cMap->getGridRows() - 1;	   // level size
	int	 gw = cMap->getGridWidth();
	int	 gh = cMap->getGridHeight();

	uchar pf, pf1, pf2, pf3, pf4;
	cMap->lockFlags();
	
	// Find a random cell to start in - retry if failed
	while(true) {
		px = (int)(Worm->NewNet_random.getFloatPositive() * (float)cols);
		py = (int)(Worm->NewNet_random.getFloatPositive() * (float)rows);
		x = px; y = py;

		if( x + y < 6 )	// Do not spawn in top left corner
			continue;

		pf = *(cMap->getAbsoluteGridFlags() + y * cMap->getGridCols() + x);
		pf1 = (x>0) ? *(cMap->getAbsoluteGridFlags() + y * cMap->getGridCols() + (x-1)) : PX_ROCK;
		pf2 = (x<cols-1) ? *(cMap->getAbsoluteGridFlags() + y * cMap->getGridCols() + (x+1)) : PX_ROCK;
		pf3 = (y>0) ? *(cMap->getAbsoluteGridFlags() + (y-1) * cMap->getGridCols() + x) : PX_ROCK;
		pf4 = (y<rows-1) ? *(cMap->getAbsoluteGridFlags() + (y+1) * cMap->getGridCols() + x) : PX_ROCK;
		if( !(pf & PX_ROCK) && !(pf1 & PX_ROCK) && !(pf2 & PX_ROCK) && !(pf3 & PX_ROCK) && !(pf4 & PX_ROCK) ) {
			cMap->unlockFlags();
			return CVec((float)x * gw + gw / 2, (float)y * gh + gh / 2);
		}
	}
}

unsigned CalculatePhysics( AbsTime gameTime, KeyState_t keys[MAX_WORMS], KeyState_t keysChanged[MAX_WORMS], bool fastCalculation, bool calculateChecksum )
{
	for( int i = 0; i < MAX_WORMS; i++ )
	{
		CWorm * w = & cClient->getRemoteWorms()[i];
		if( w->isUsed() )
		{
			// Respawn dead worms
			// TODO: make this server-sided
			if( !w->getAlive() && w->getLives() != WRM_OUT )
				if( gameTime - w->getTimeofDeath() > 2.5f )
					w->Spawn( NewNet_FindSpot(w) );

			w->NewNet_GetInput( keys[i], keysChanged[i] );
			if (w->getWormState()->bShoot && w->getAlive())
				cClient->NewNet_DoLocalShot( w );
		}
	}

	cClient->NewNet_Simulation();
	return 0;
};

void DisableAdvancedFeatures()
{
	 // Disables bonuses and connect-during-game for now, 
	 // I can add bonuses but connect-during-game is complicated
	 tLXOptions->tGameInfo.bBonusesOn = false;
	 tLXOptions->tGameInfo.bAllowConnectDuringGame = false;
	 tLXOptions->tGameInfo.bAllowStrafing = false;
	 //tLXOptions->tGameInfo.fRespawnTime = 2.5f; // Should be the same for all clients
	 //tLXOptions->tGameInfo.bRespawnGroupTeams = false;
	 //tLXOptions->tGameInfo.bEmptyWeaponsOnRespawn = false;
	 //*cClient->getGameLobby() = tLXOptions->tGameInfo;
};

void CalculateCurrentState( AbsTime localTime );
bool SendNetPacket( AbsTime localTime, KeyState_t keys, CBytestream * bs );

bool Frame( CBytestream * bs )
{
	AbsTime localTime = tLX->currentTime;
	KeyState_t keys;
	if( cClient->getNumWorms() > 0 && cClient->getWorm(0)->getType() == PRF_HUMAN )
	{
		CWormHumanInputHandler * hnd = (CWormHumanInputHandler *) cClient->getWorm(0)->inputHandler();
		keys.keys[K_UP] = hnd->getInputUp().isDown();
		keys.keys[K_DOWN] = hnd->getInputDown().isDown();
		keys.keys[K_LEFT] = hnd->getInputLeft().isDown();
		keys.keys[K_RIGHT] = hnd->getInputRight().isDown();
		keys.keys[K_SHOOT] = hnd->getInputShoot().isDown();
		keys.keys[K_JUMP] = hnd->getInputJump().isDown();
		keys.keys[K_SELWEAP] = hnd->getInputWeapon().isDown();
		keys.keys[K_ROPE] = hnd->getInputRope().isDown();
		if( tLXOptions->bOldSkoolRope )
			keys.keys[K_ROPE] = ( hnd->getInputJump().isDown() && hnd->getInputWeapon().isDown() );
	};
	bool ret = SendNetPacket( localTime, keys, bs );
	CalculateCurrentState( localTime );
	return ret;
};

// --------- Net sending-receiving functions and internal stuff independent of OLX ---------

bool QuickDirtyCalculation;
bool ReCalculationNeeded;
AbsTime ReCalculationTimeMs;
// Constants
// TODO: it seems that some of these are actually not TimeDiffs but TimeDiffs/TICK_TIME (or so).
// For those, please don't use TimeDiff. Best would be to make an extra struct for them to have that clear and to seperate
// all that *TICK_TIME or /TICK_TIME calculations.
TimeDiff PingTimeMs = TimeDiff(300);	// Send at least one packet in 10 ms - 10 packets per second, huge net load
// TODO: calculate DrawDelayMs from other client pings
TimeDiff DrawDelayMs = TimeDiff(100);	// Not used currently // Delay the drawing until all packets are received, otherwise worms will teleport
TimeDiff ReCalculationMinimumTimeMs = TimeDiff(100);	// Re-calculate not faster than 10 times per second - eats CPU
TimeDiff CalculateChecksumTime = TimeDiff(5000); // Calculate checksum once per 5 seconds - should be equal for all clients

int NumPlayers = -1;
int LocalPlayer = -1;

// TODO: why is it named diff but used absolute?
AbsTime OlxTimeDiffMs; // In milliseconds
AbsTime CurrentTimeMs; // In milliseconds
AbsTime BackupTime; // In milliseconds
AbsTime ClearEventsLastTime;

struct KeysEvent_t
{
	KeyState_t keys;
	KeyState_t keysChanged;
};

// Sorted by player and time - time in milliseconds
typedef std::map< AbsTime, KeysEvent_t > EventList_t;
EventList_t Events [MAX_WORMS];
KeyState_t OldKeys[MAX_WORMS];
AbsTime LastPacketTime[MAX_WORMS];
AbsTime LastSoundPlayedTime[MAX_WORMS];
unsigned Checksum;
AbsTime ChecksumTime; // AbsTime in ms
//int InitialRandomSeed; // Used for LoadState()/SaveState()

void getKeysForTime( AbsTime t, KeyState_t keys[MAX_WORMS], KeyState_t keysChanged[MAX_WORMS] )
{
	for( int i=0; i<MAX_WORMS; i++ )
	{
		EventList_t :: const_iterator it = Events[i].upper_bound(t);
		if( it != Events[i].begin() )
		{
			--it;
			keys[i] = it->second.keys;
			keysChanged[i] = it->second.keysChanged;
		}
		else
		{
			keys[i] = KeyState_t();
			keysChanged[i] = KeyState_t();
		}
	}
};

void StartRound( unsigned randomSeed )
{
			OlxTimeDiffMs = tLX->currentTime;
			NumPlayers = 0;
			netRandom.seed(randomSeed);
			for( int i=0; i<MAX_WORMS; i++ )
			{
				Events[i].clear();
				OldKeys[i] = KeyState_t();
				LastPacketTime[i] = AbsTime();
				LastSoundPlayedTime[i] = AbsTime();
				if( cClient->getRemoteWorms()[i].isUsed() )
				{
					NumPlayers ++;
					cClient->getRemoteWorms()[i].NewNet_InitWormState(randomSeed + i);
				};
			}
			LocalPlayer = -1;
			if( cClient->getNumWorms() > 0 )
				LocalPlayer = cClient->getWorm(0)->getID();
			//InitialRandomSeed = randomSeed;

			CurrentTimeMs = 0;
			BackupTime = 0;
			ClearEventsLastTime = 0;
			Checksum = 0;
			ChecksumTime = 0;
			QuickDirtyCalculation = true;
			ReCalculationNeeded = false;
			ReCalculationTimeMs = 0;
			if( ! SavedWormState )
				SavedWormState = new CWorm[MAX_WORMS];
			SaveState();
};

void EndRound()
{
	delete [] SavedWormState;
	SavedWormState = NULL;
};

void ReCalculateSavedState()
{
	if( CurrentTimeMs - ReCalculationTimeMs < ReCalculationMinimumTimeMs || ! ReCalculationNeeded )
		return; // Limit recalculation - it is CPU-intensive
	//if( ! ReCalculationNeeded )
	//	return;

	ReCalculationTimeMs = CurrentTimeMs;
	ReCalculationNeeded = false;

	// Re-calculate physics if the packet received is from the most laggy client
	AbsTime timeMin = LastPacketTime[LocalPlayer];
	for( int f=0; f<MAX_WORMS; f++ )
		if( LastPacketTime[f] < timeMin && cClient->getRemoteWorms()[f].isUsed() )
			timeMin = LastPacketTime[f];

	//printf("ReCalculate(): BackupTime %lu timeMin %lu\n", BackupTime, timeMin);
	if( BackupTime /* + DrawDelayMs */ + TICK_TIME >= timeMin )
		return;

	QuickDirtyCalculation = false;
	if( CurrentTimeMs != BackupTime )	// Last recalc time
		RestoreState();

	while( BackupTime /* + DrawDelayMs */ + TICK_TIME < timeMin )
	{
		BackupTime += TimeDiff(TICK_TIME);
		CurrentTimeMs = BackupTime;
		bool calculateChecksum = CurrentTimeMs.time % CalculateChecksumTime.timeDiff == 0;

		KeyState_t keys[MAX_WORMS];
		KeyState_t keysChanged[MAX_WORMS];
		getKeysForTime( BackupTime, keys, keysChanged );

		unsigned checksum = CalculatePhysics( CurrentTimeMs, keys, keysChanged, false, calculateChecksum );
		if( calculateChecksum )
		{
			Checksum = checksum;
			ChecksumTime = CurrentTimeMs;
			//printf("OlxMod time %lu checksum 0x%X\n", ChecksumTime, Checksum );
		};
	};

	SaveState();
	CurrentTimeMs = BackupTime;

	// Clean up old events - do not clean them if we're the server, clients may ask for them.
	/*
	// TODO: ensure every worm has at least one event left in the array, that's why commented this code out
	if( BackupTime - ClearEventsLastTime > 100 && LocalPlayer != 0 )
	{
		ClearEventsLastTime = BackupTime;
		Events.erase(Events.begin(), Events.lower_bound( BackupTime - 2 ));
	};
	*/
	QuickDirtyCalculation = true;
};

// Should be called immediately after SendNetPacket() with the same time value
void CalculateCurrentState( AbsTime localTime )
{
	localTime.time -= OlxTimeDiffMs.time;

	ReCalculateSavedState();

	//printf("Draw() time %lu oldtime %lu\n", localTime / TICK_TIME , CurrentTimeMs / TICK_TIME );

	while( CurrentTimeMs < localTime /*- DrawDelayMs*/ )
	{
		CurrentTimeMs += TimeDiff(TICK_TIME);

		KeyState_t keys[MAX_WORMS];
		KeyState_t keysChanged[MAX_WORMS];
		getKeysForTime( CurrentTimeMs, keys, keysChanged );

		CalculatePhysics( CurrentTimeMs, keys, keysChanged, true, false );
	};
};

int NetPacketSize()
{
	// Change here if you'll modify Receive()/Send()
	return 4+1;	// First 4 bytes is time, second byte - keypress idx
}

void AddEmptyPacket( AbsTime localTime, CBytestream * bs )
{
	bs->writeInt( (int)localTime.time, 4 );  // TODO: possible overflow
	bs->writeByte( UCHAR_MAX );
}

TimeDiff EmptyPacketTime()
{
	return PingTimeMs;
}

// Returns true if data was re-calculated.
void ReceiveNetPacket( CBytestream * bs, int player )
{
	int timeDiff = bs->readInt( 4 );	// TODO: 1-2 bytes are enough, I just screwed up with calculations

	AbsTime fullTime(timeDiff);

	KeyState_t keys = OldKeys[ player ];
	int keyIdx = bs->readByte();
	if( keyIdx != UCHAR_MAX )
		keys.keys[keyIdx] = ! keys.keys[keyIdx];

	OldKeys[ player ] = keys;
	Events[ player ] [ fullTime ] .keys = keys;
	if( keyIdx != UCHAR_MAX )
		Events[ player ] [ fullTime ] .keysChanged.keys[keyIdx] = ! Events[ player ] [ fullTime ] .keysChanged.keys[keyIdx];
	LastPacketTime[ player ] = fullTime;

	ReCalculationNeeded = true;
	// We don't want to calculate with just 1 of 2 keys pressed - it will desync
	// Net engine will send them in single packet anyway, so they are coupled together
	//ReCalculateSavedState(); // Called from Frame() anyway, 
};

// Should be called for every gameloop frame with current key state, returns true if there's something to send
// Draw() should be called after this func
bool SendNetPacket( AbsTime localTime, KeyState_t keys, CBytestream * bs )
{
	//printf("SendNetPacket() time %lu\n", localTime);
	localTime.time -= OlxTimeDiffMs.time;

	if( keys == OldKeys[ LocalPlayer ] &&
		localTime - LastPacketTime[ LocalPlayer ] < PingTimeMs ) // Do not flood the net with non-changed keys
		return false;

	KeyState_t changedKeys = OldKeys[ LocalPlayer ] ^ keys;

	//printf("SendNetPacket() put keys in time %lu\n", localTime);
	bs->writeInt( (int)localTime.time, 4 );	// TODO: 1-2 bytes are enough, I just screwed up with calculations
	int changedKeyIdx = changedKeys.getFirstPressedKey();
	if( changedKeyIdx == -1 )
		bs->writeByte( UCHAR_MAX );
	else
	{
		// If we pressed 2 keys SendNetPacket() will be called two times
		bs->writeByte( changedKeyIdx );
		OldKeys[ LocalPlayer ].keys[ changedKeyIdx ] = ! OldKeys[ LocalPlayer ].keys[ changedKeyIdx ];
	}
	Events[ LocalPlayer ] [ localTime ] .keys = OldKeys[ LocalPlayer ];
	if( changedKeyIdx != -1 )
		Events[ LocalPlayer ] [ localTime ] .keysChanged.keys[changedKeyIdx] = ! Events[ LocalPlayer ] [ localTime ] .keysChanged.keys[changedKeyIdx];

	LastPacketTime[ LocalPlayer ] = localTime;

	if( NumPlayers == 1 )
		ReCalculationNeeded = true;

	return true;
};

unsigned GetChecksum( AbsTime * time )
{
	if( time )
		*time = ChecksumTime;
	return Checksum;
};

AbsTime GetCurTime()
{
	return CurrentTimeMs;
}

bool CanUpdateGameState()
{
	return !QuickDirtyCalculation;
};

bool CanPlaySound(int wormID)
{
	if( LastSoundPlayedTime[wormID] < CurrentTimeMs )
	{
		LastSoundPlayedTime[wormID] = CurrentTimeMs;
		return true;
	}
	return false;
};


// -------- Misc funcs, boring implementation of randomizer and keys bit funcs -------------

#define LCG(n) ((69069UL * n) & 0xffffffffUL)
#define MASK 0xffffffffUL

void ___Random_Seed__(unsigned s, __taus113_state_t & NetSyncedRandom_state)
{
  if (!s)
    s = 1UL;                    /* default seed is 1 */

  NetSyncedRandom_state.z1 = LCG (s);
  if (NetSyncedRandom_state.z1 < 2UL)
    NetSyncedRandom_state.z1 += 2UL;
  NetSyncedRandom_state.z2 = LCG (NetSyncedRandom_state.z1);
  if (NetSyncedRandom_state.z2 < 8UL)
    NetSyncedRandom_state.z2 += 8UL;
  NetSyncedRandom_state.z3 = LCG (NetSyncedRandom_state.z2);
  if (NetSyncedRandom_state.z3 < 16UL)
    NetSyncedRandom_state.z3 += 16UL;
  NetSyncedRandom_state.z4 = LCG (NetSyncedRandom_state.z3);
  if (NetSyncedRandom_state.z4 < 128UL)
    NetSyncedRandom_state.z4 += 128UL;

  /* Calling RNG ten times to satify recurrence condition */
  ___Random__(NetSyncedRandom_state);
  ___Random__(NetSyncedRandom_state);
  ___Random__(NetSyncedRandom_state);
  ___Random__(NetSyncedRandom_state);
  ___Random__(NetSyncedRandom_state);
  ___Random__(NetSyncedRandom_state);
  ___Random__(NetSyncedRandom_state);
  ___Random__(NetSyncedRandom_state);
  ___Random__(NetSyncedRandom_state);
  ___Random__(NetSyncedRandom_state);

  return;
};

#undef LCG
#undef MASK

	KeyState_t::KeyState_t()
	{
		for( int i=0; i<K_MAX; i++ )
			keys[i] = false;
	};

	bool KeyState_t::operator == ( const KeyState_t & k ) const
	{
		for( int i=0; i<K_MAX; i++ )
			if( keys[i] != k.keys[i] )
				return false;
		return true;
	};

	KeyState_t KeyState_t::operator & ( const KeyState_t & k ) const	// and
	{
		KeyState_t res;
		for( int i=0; i<K_MAX; i++ )
			if( keys[i] && k.keys[i] )
				res.keys[i] = true;
		return res;
	}

	KeyState_t KeyState_t::operator | ( const KeyState_t & k ) const	// or
	{
		KeyState_t res;
		for( int i=0; i<K_MAX; i++ )
			if( keys[i] || k.keys[i] )
				res.keys[i] = true;
		return res;
	}
	
	KeyState_t KeyState_t::operator ^ ( const KeyState_t & k ) const	// xor
	{
		KeyState_t res;
		for( int i=0; i<K_MAX; i++ )
			if( keys[i] != k.keys[i] )
				res.keys[i] = true;
		return res;
	}

	KeyState_t KeyState_t::operator ~ () const	// not
	{
		KeyState_t res;
		for( int i=0; i<K_MAX; i++ )
			if( ! keys[i] )
				res.keys[i] = true;
		return res;
	}

	int KeyState_t::getFirstPressedKey() const // Returns idx of first pressed key, or -1
	{
		for( int i=0; i<K_MAX; i++ )
			if( keys[i] )
				return i;
		return -1;
	}

	unsigned NetSyncedRandom::getSeed()
	{
		return (~(unsigned)time(NULL)) + SDL_GetTicks();
	};
};