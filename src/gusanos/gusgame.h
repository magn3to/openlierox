#ifndef GAME_H
#define GAME_H

#include "level.h"
#include "message_queue.h"
#include "object_grid.h"
#include "FeatureList.h"
#include "OlxVariable.h"

#ifndef DEDICATED_ONLY
#include "gusanos/allegro.h"
#endif
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <boost/shared_ptr.hpp>
using boost::shared_ptr;
#include "netstream.h"

class CWormInputHandler;
class CWorm;
class BaseAction;
struct PlayerOptions;
class WeaponType;
class Particle;
class PartType;
class Explosion;
class BitStream;
struct LuaEventDef;
#ifndef DEDICATED_ONLY
class Sound;
class Font;
#endif
class CMap;

class CWormHumanInputHandler;

struct Options
{
	void registerInConsole();
	
	OlxSpeedVar<FT_RopeSpeed> ninja_rope_shootSpeed;
	float ninja_rope_pullForce;
	float ninja_rope_startDistance;
	float ninja_rope_maxLength;
	OlxSpeedVar<FT_WormMaxGroundMoveSpeed> worm_maxSpeed;
	OlxAccelVar<FT_WormAcceleration> worm_acceleration;
	OlxVar<float,FT_WormAirAccelerationFactor> worm_airAccelerationFactor;
	OlxWormFrictionVar<FT_WormGroundFriction> worm_friction;
	OlxWormFrictionVar<FT_WormSimpleFriction> worm_airFriction;
	OlxAccelVar<FT_WormGravity> worm_gravity;
	OlxVar<float,FT_WormBounceQuotient> worm_bounceQuotient;
	OlxVar<float,FT_WormBounceLimit> worm_bounceLimit;
	OlxNegatedSpeedVar<FT_WormJumpForce> worm_jumpForce;
	OlxBoolNegatedVar<FT_WormWallHugging> worm_disableWallHugging;
	OlxVar<int,FT_WormWeaponHeight> worm_weaponHeight;
	OlxVar<int,FT_WormHeight> worm_height;
	OlxVar<int,FT_WormWidth> worm_width;
	OlxVar<int,FT_WormMaxClimb> worm_maxClimb;
	OlxVar<float,FT_WormBoxRadius> worm_boxRadius;
	OlxVar<float,FT_WormBoxTop> worm_boxTop;
	OlxVar<float,FT_WormBoxBottom> worm_boxBottom;
	int maxRespawnTime;
	int minRespawnTime;
	int maxWeaponsVar;
	size_t maxWeapons;
	int splitScreenVar;
	bool splitScreen;
	std::string rConPassword;
	bool showDeathMessages;
	bool logDeathMessages;
	
	int showMapDebug;
};

struct LevelEffectEvent
{
	LevelEffectEvent( int index_, int x_, int y_ ) : index(index_), x(x_), y(y_)
	{
	}
	int index;
	int x,y;
	
};

struct ScreenMessage
{
	enum Type
	{
		Death,
		Chat,
	};
	
	ScreenMessage(Type type_, std::string const& str_, int timeOut_ = 400)
	: type(type_), str(str_), timeOut(timeOut_)
	{
	}
	
	Type type;
	std::string str;
	int timeOut;
};

class GusGame
{
public:
	static const size_t MAX_LOCAL_PLAYERS = 2;
	
	
		
	static Net_ClassID  classID;

	enum PLAYER_TYPE
	{
		OWNER = 0,
		PROXY,
		AI,
	};
	
	enum ResetReason
	{
		ServerQuit,
		ServerChangeMap,
		Kicked,
		LoadingLevel,
		IncompatibleProtocol,
		IncompatibleData,
	};
	
	enum Error
	{
		ErrorNone = 0,
		ErrorMapNotFound,
		ErrorMapLoading,
		ErrorModNotFound,
		ErrorModLoading
	};
		
	GusGame();
	~GusGame();
	
	bool init();
	void parseCommandLine(int argc, char** argv);
	
	void think();
	
	bool setMod(const std::string& mod);
	void loadWeapons();
	void reset(ResetReason reason);
	void unload();
	void reinit();
	void error(Error err);
	bool loadMod(bool doLoadWeapons = true);
	bool isLoaded();
	bool isLevelLoaded();
	void refreshResources(std::string const& levelPath);
	void refreshLevels();
	void refreshMods();
	bool reloadModWithoutMap();
	void createNetworkPlayers();
	bool loadModWithoutMap();
	bool changeLevel(ResourceLocator<CMap>::BaseLoader* loader, const std::string& path, CMap* m = NULL);
	bool changeLevel(const std::string& levelName, bool refresh = true);
	bool changeLevelCmd(const std::string& level);
	bool hasLevel(std::string const& level);
	bool hasMod(std::string const& mod);
	void runInitScripts();
	void addBot();
	CWormInputHandler* findPlayerWithID( Net_NodeID ID );
	CWormInputHandler* addPlayer( PLAYER_TYPE type, CWorm* worm );
	CWorm* addWorm(bool isAuthority); // Creates a worm class depending on the network condition.
	//static Net_Node* getNode();
	static void sendLuaEvent(LuaEventDef* event, eNet_SendMode mode, Net_U8 rules, BitStream* data, Net_ConnID connID);
	
	void assignNetworkRole( bool authority );
	void removeNode();
	
	void applyLevelEffect( LevelEffect* effect, int x, int y );
	
	void sendRConMsg( std::string const & message );
	void displayChatMsg( std::string const& owner, std::string const& message);
	void displayKillMsg( CWormInputHandler* killed, CWormInputHandler* killer );
	void displayMessage( ScreenMessage const& msg );
	
	CMap& level();
	bool isEngineNeeded();
	
	std::vector<WeaponType*> weaponList;
	Options options;
	std::vector<shared_ptr<PlayerOptions> > playerOptions;
	std::set<std::string> modList;
	
	void insertExplosion( Explosion* explosion );
	
	std::map< std::string, BaseAction*(*)( const std::vector< std::string > & ) > actionList;
	//HashTable< std::string, BaseAction*(*)( const std::vector< std::string > & ) > actionList;
	
	PartType* NRPartType;
	PartType* deathObject;
	PartType* digObject;
		
	const std::string& getMod();
	std::string const& getModPath();
	std::string const& getDefaultPath();

#ifndef DEDICATED_ONLY
	Sound* chatSound;
	Font *infoFont;
#endif
	std::list<ScreenMessage> messages;

	unsigned long stringToIndex(std::string const& str);
	
	std::string const& indexToString(unsigned long idx);
	
	std::string const& getModName();
	
	static void addCRCs(BitStream* req);
	static bool checkCRCs(BitStream& data);
	
	MessageQueue msg;
	
	mq_define_message(ChangeLevel, 0, (std::string level_))
		: level(level_)
		{
			
		}
		
		std::string level;
	mq_end_define_message()
	
	mq_define_message(ChangeLevelReal, 1, (std::string level_))
		: level(level_)
		{
			
		}
		
		std::string level;
	mq_end_define_message()
	
private:
	void prepareLoad(const std::string& path);
	void finishLoad();
};

extern GusGame gusGame;

#endif // _GAME_H_
