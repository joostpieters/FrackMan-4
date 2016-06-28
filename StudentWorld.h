#ifndef STUDENTWORLD_H_
#define STUDENTWORLD_H_

#include "GameWorld.h"
#include "GameConstants.h"
#include "GraphObject.h"
#include <vector>
#include <string>

class Actor;
class Dirt;
class FrackMan;
class Protester;

class StudentWorld : public GameWorld
{
public:
	enum Object { boulder, gold, oil, sonar, water };

	StudentWorld(std::string assetDir);
	virtual ~StudentWorld();

	virtual int init();
	virtual int move();
	virtual void cleanUp();

	void dirToCoord(Actor* p, int& x, int& y, int distance = 1) const;
	bool moveBlocksDigger(Actor* p, int distance = 1) const;
	bool moveBlocksBoulder(Actor* p, int distance = 1) const;
	bool moveBlocksSquirt(Actor* p, int distance = 1) const;
	bool moveBlocksProtester(Actor* p, int distance = 1) const;
	bool moveBlocksProtester(int x, int y) const;
	bool digIfNeeded(Actor* p);
	void squirtIfNeeded();
	void dropGoldIfNeeded();
	void activateIfNeeded(Actor* p, Object type);
	void shout(Actor* p);
	bool annoyAllNearbyAgents(Actor* p, int radius, int points, bool exemptPlayer = false);
	void revealIfNearby(Actor* p, int distance = 4);
	void revealAllNearby(int distance = 12);
	bool actorNextToPlayer(Actor* p, int distance = 4) const;
	bool actorFacingPlayer(Actor* p) const;
	void changeDirToPlayer(Actor* p);
	bool unblockedLineOfSightToPlayer(Actor* p) const;
	bool atIntersection(Actor* p) const;
	void decrementProtesters();

	void updateExitHeatMap();
	void updatePlayerHeatMap();
	void setExitHeatMap(bool k) {m_updateExitHeatMap = k;}
	void setPlayerHeatMap(bool k) {m_updatePlayerHeatMap = k;}
	GraphObject::Direction findBestMoveToExit(Protester* p) const;
	GraphObject::Direction findBestMoveToPlayer(Protester* p) const;
	void incrementNumLeavers() {m_numLeavers++; m_updateExitHeatMap = true;}
	void decrementNumLeavers() {if (m_numLeavers > 0) m_numLeavers--;}
	void decrementNumTrackers() {if (m_numTrackers > 0) m_numTrackers--;}

private:
	void addActor(Actor* p);
	void fillFieldWithDirt();
	void createMineShaft();
	void distributeObjects(int num, Object type);
	void setDisplayText();
	void removeDeadActors();
	bool withinBounds(int x, int y) const;
	bool moveWithinBounds(int x, int y) const;
	bool square4x4HasDirt(int x, int y) const;
	bool square4x1HasDirt(int x, int y) const;
	bool square4x4HasBoulder(int x, int y, Actor* p = nullptr) const;
	bool withinRadius(int x1, int y1, int x2, int y2, int radius) const;
	bool playerCompletedLevel() const;

	void initializeExitHeatMap();
	void initializePlayerHeatMap();
	int numLeaversSet(int x, int y, int distance);
	int numTrackersSet(int x, int y, int distance);

private:
	class Coord
	{
	public:
		Coord(int xx, int yy) : m_x(xx), m_y(yy) {}
		int x() const { return m_x; }
		int y() const { return m_y; }
	private:
		int m_x;
		int m_y;
	};
	std::vector<Actor*>	m_actors;
	Dirt*			m_field[VIEW_WIDTH][VIEW_HEIGHT];
	FrackMan*		m_player;
	int			m_numOil;
	int			m_numProtesters;
	int			m_protesterCooldown;
	int			m_numLeavers;
	int			m_numTrackers;
	bool			m_updateExitHeatMap;
	bool			m_updatePlayerHeatMap;
	int			m_exitHeatMap[VIEW_WIDTH - SPRITE_WIDTH + 1][VIEW_HEIGHT - SPRITE_HEIGHT + 1];
	int			m_playerHeatMap[VIEW_WIDTH - SPRITE_WIDTH + 1][VIEW_HEIGHT - SPRITE_HEIGHT + 1];
};

#endif // STUDENTWORLD_H_
