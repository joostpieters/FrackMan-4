#include "StudentWorld.h"
#include "Actor.h"
#include <string>
#include <cstdlib>	// abs
#include <queue>	// pathfinding
using namespace std;

GameWorld* createStudentWorld(string assetDir)
{
	return new StudentWorld(assetDir);
}

///////////////////////////////////////////
//	PUBLIC MEMBER FUNCTIONS
///////////////////////////////////////////

////////// CONSTRUCTOR, DESTRUCTOR

StudentWorld::StudentWorld(string assetDir)
	: GameWorld(assetDir), m_numOil(0), m_player(nullptr), m_numProtesters(0), m_protesterCooldown(0)
{
	// initialize m_field
	for (int x = 0; x < VIEW_WIDTH; x++)
		for (int y = 0; y < VIEW_HEIGHT; y++)
			m_field[x][y] = nullptr;

	m_numLeavers = 0;
	m_numTrackers = 0;
	m_updateExitHeatMap = true;
	m_updatePlayerHeatMap = true;
	// initialize heatmaps
	initializeExitHeatMap();
}

StudentWorld::~StudentWorld()
{
	cleanUp();
}

////////// INIT(), MOVE(), CLEANUP()

int StudentWorld::init()
{
	// Create frackman and insert at proper location
	Actor* player = new FrackMan(this);
	addActor(player);
	m_player = dynamic_cast<FrackMan*>(m_actors[0]);

	// Create all dirt objects
	fillFieldWithDirt();
	createMineShaft();

	// Distribute Boulders
	int numBoulders = getLevel() / 2 + 2 < 6 ? getLevel() / 2 + 2 : 6;
	distributeObjects(numBoulders, boulder);

	// Distribute Gold Nuggets
	int numGold = 5 - getLevel() / 2 > 2 ? 5 - getLevel() / 2 : 2;
	distributeObjects(numGold, gold);

	// Distribute Oil Barrels
	int numOil = getLevel() + 2 < 20 ? getLevel() + 2 : 20;
	distributeObjects(numOil, oil);
	m_numOil += numOil;

	return GWSTATUS_CONTINUE_GAME;
}

int StudentWorld::move()
{
	// Update the Game Status Line
	setDisplayText();

	// Check if FrackMan is dead
	if (!m_player->isAlive())
	{
		playSound(SOUND_PLAYER_GIVE_UP);
		decLives();
		return GWSTATUS_PLAYER_DIED;
	}

	// Ask all actors to do something
	unsigned int size = m_actors.size();
	for (unsigned int i = 0; i < size; i++)
	{
		if (m_actors[i]->isAlive())
		{
			m_actors[i]->doSomething();
			if (playerCompletedLevel())
				return GWSTATUS_FINISHED_LEVEL;
			else if (!m_player->isAlive())
			{
				playSound(SOUND_PLAYER_GIVE_UP);
				decLives();
				return GWSTATUS_PLAYER_DIED;
			}
		}
	}

	// Add Protesters if needed
	if (!m_protesterCooldown) // == 0
	{
		int P = 15 < 2 + getLevel() * 1.5 ? 15 : 2 + getLevel() * 1.5;
		if (m_numProtesters < P)
		{
			Actor* p = nullptr;
			int probability = 90 < getLevel() * 10 + 30 ? 90 : getLevel() * 10 + 30;
			if (rand() % 100 < probability)
				p = new RegularProtester(this);
			else
			{
				p = new HardcoreProtester(this);
				m_numTrackers++;
			}
			addActor(p);
			m_numProtesters++;
		}
		m_protesterCooldown = 25 > 200 - getLevel() ? 25 : 200 - getLevel();
	}
	else
		m_protesterCooldown--;
	// Add goodies
	int G = getLevel() * 25 + 300;
	int chance = rand() % G;
	if (!chance) // 1 in G
	{
		int chance2 = rand() % 5;
		if (!chance2) // 1 in 5
		{
			Actor* p = new Sonar(this, 0, 60);
			addActor(p);
		}
		else
		{
			int x, y;
			for (;;)
			{
				x = rand() % (VIEW_WIDTH - SPRITE_WIDTH);
				y = rand() % (VIEW_WIDTH - SPRITE_WIDTH);
				if (!square4x4HasDirt(x, y))
					break;
			}
			Actor* p = new WaterPool(this, x, y);
			addActor(p);
		}
	}

	// Remove newly-dead actors after each tick
	removeDeadActors();

	// Update game counters
	m_updatePlayerHeatMap = true;

	// If player completed the level, return level finished
	if (playerCompletedLevel())
		return GWSTATUS_FINISHED_LEVEL;

	// Continue to next tick
	return GWSTATUS_CONTINUE_GAME;
}

void StudentWorld::cleanUp()
{
	m_player = nullptr;
	m_numOil = 0;
	m_numProtesters = 0;
	m_protesterCooldown = 0;
	m_numLeavers = 0;
	m_numTrackers = 0;
	m_updateExitHeatMap = true;
	m_updatePlayerHeatMap = true;
	// Free allocated actors and empty the vector
	while (!m_actors.empty())
	{
		delete m_actors.back();
		m_actors.pop_back();
	}
	// Free all allocated dirt
	for (int x = 0; x < VIEW_WIDTH; x++)
		for (int y = 0; y < VIEW_HEIGHT; y++)
			delete m_field[x][y];
}

////////// OTHER PUBLIC MEMBER FUNCTIONS

void StudentWorld::dirToCoord(Actor* p, int& x, int& y, int distance) const
{
	GraphObject::Direction d = p->getDirection();
	switch (d)
	{
	case GraphObject::up:
		x = 0;	y = distance;	break;
	case GraphObject::down:
		x = 0;	y = -distance;	break;
	case GraphObject::left:
		x = -distance;	y = 0;	break;
	case GraphObject::right:
		x = distance;	y = 0;	break;
	case GraphObject::none:
		x = 0;	y = 0;		break;
	}
	x += p->getX();
	y += p->getY();
}

bool StudentWorld::moveBlocksDigger(Actor* p, int distance) const
{
	int x, y;
	dirToCoord(p, x, y, distance);
	return !moveWithinBounds(x, y) || square4x4HasBoulder(x, y, p);
}

bool StudentWorld::moveBlocksBoulder(Actor* p, int distance) const
{
	int x, y;
	dirToCoord(p, x, y, distance);
	return square4x1HasDirt(x, y) || moveBlocksDigger(p, distance);
}

bool StudentWorld::moveBlocksSquirt(Actor* p, int distance) const
{
	int x, y;
	dirToCoord(p, x, y, distance);
	return square4x4HasDirt(x, y) || moveBlocksDigger(p, distance);
}


bool StudentWorld::moveBlocksProtester(Actor* p, int distance) const
{
	return moveBlocksSquirt(p, distance);
}

bool StudentWorld::moveBlocksProtester(int x, int y) const
{
	return !moveWithinBounds(x, y) || square4x4HasDirt(x, y) || square4x4HasBoulder(x, y);
}

bool StudentWorld::digIfNeeded(Actor* p)
{
	bool dug = false;
	for (int x = p->getX(); x < p->getX() + 4; x++)
	{
		for (int y = p->getY(); y < p->getY() + 4; y++)
		{
			if (m_field[x][y] != nullptr)
			{
				delete m_field[x][y];
				m_field[x][y] = nullptr;
				dug = true;
				m_updateExitHeatMap = true;
			}
		}
	}
	return dug;
}

void StudentWorld::squirtIfNeeded()
{
	if (!moveBlocksSquirt(m_player, 4))
	{
		int x, y;
		dirToCoord(m_player, x, y, 4);
		Actor* p = new Squirt(this, x, y, m_player->getDirection());
		addActor(p);
	}
	playSound(SOUND_PLAYER_SQUIRT);
}

void StudentWorld::dropGoldIfNeeded()
{
	if (m_player->getGold())
	{
		Actor* p = new DroppedGold(this, m_player->getX(), m_player->getY());
		addActor(p);
	}
}

void StudentWorld::activateIfNeeded(Actor* p, Object type)
{
	if (p->playerCanPickUp())
	{
		if (withinRadius(p->getX(), p->getY(), m_player->getX(), m_player->getY(), 3))
		{
			switch (type)
			{
			case oil:
				playSound(SOUND_FOUND_OIL);
				increaseScore(1000);
				m_numOil--;
				break;
			case gold:
				playSound(SOUND_GOT_GOODIE);
				increaseScore(10);
				m_player->addGold();
				break;
			case sonar:
				playSound(SOUND_GOT_GOODIE);
				increaseScore(75);
				m_player->addSonar();
				break;
			case water:
				playSound(SOUND_GOT_GOODIE);
				increaseScore(100);
				m_player->addWater();
				break;
			}
			p->setDead();
		}
	}
	else
	{
		for (unsigned int i = 1; i < m_actors.size(); i++) // start at 1 to avoid player
		{
			if (m_actors[i]->canPickThingsUp() /* Agent */
				&& withinRadius(p->getX(), p->getY(), m_actors[i]->getX(), m_actors[i]->getY(), 3))
			{
				Protester* protester = dynamic_cast<Protester*>(m_actors[i]);
				if (!protester->isLeaving())
				{
					p->setDead();
					playSound(SOUND_PROTESTER_FOUND_GOLD);
					protester->addGold();
					return;
				}
			}
		}
	}
}

void StudentWorld::shout(Actor* p)
{
	playSound(SOUND_PROTESTER_YELL);
	m_player->annoy(2, p);
}

bool StudentWorld::annoyAllNearbyAgents(Actor* p, int radius, int points, bool exemptPlayer)
{
	bool annoyed = false;
	unsigned int i = exemptPlayer ? 1 : 0;
	for (; i < m_actors.size(); i++)
	{
		if (m_actors[i]->canPickThingsUp() /* Agent */
			&& withinRadius(p->getX(), p->getY(), m_actors[i]->getX(), m_actors[i]->getY(), 3))
		{
			m_actors[i]->annoy(points, p);
			annoyed = true;
		}
	}
	return annoyed;
}

void StudentWorld::revealIfNearby(Actor* p, int distance)
{
	if (p->isVisible())
		return;
	if (withinRadius(p->getX(), p->getY(), m_player->getX(), m_player->getY(), distance))
		p->setVisible(true);
}

void StudentWorld::revealAllNearby(int distance)
{
	for (unsigned int i = 1; i < m_actors.size(); i++) // start at 1 to skip player
		revealIfNearby(m_actors[i], distance);
}

bool StudentWorld::actorNextToPlayer(Actor* p, int distance) const
{
	return withinRadius(p->getX(), p->getY(), m_player->getX(), m_player->getY(), distance);
}

bool StudentWorld::actorFacingPlayer(Actor* p) const
{
	int x = p->getX() - m_player->getX();
	int y = p->getY() - m_player->getY();

	GraphObject::Direction dir = p->getDirection();
	switch (dir)
	{
	case GraphObject::up:
		return y < 0 && abs(x) <= abs(y);
	case GraphObject::down:
		return y > 0 && abs(x) <= abs(y);
	case GraphObject::left:
		return x > 0 && abs(x) >= abs(y);
	case GraphObject::right:
		return x < 0 && abs(x) >= abs(y);
	default:
		return false;
	}
}

void StudentWorld::changeDirToPlayer(Actor* p)
{
	int x = p->getX() - m_player->getX();
	int y = p->getY() - m_player->getY();
	// UP
	if (y < 0 && abs(x) <= abs(y))
		p->setDirection(GraphObject::up);
	// DOWN
	if (y > 0 && abs(x) <= abs(y))
		p->setDirection(GraphObject::down);
	// LEFT
	if (x > 0 && abs(x) >= abs(y))
		p->setDirection(GraphObject::left);
	// RIGHT
	if (x < 0 && abs(x) >= abs(y))
		p->setDirection(GraphObject::right);
}

bool StudentWorld::unblockedLineOfSightToPlayer(Actor* p) const
{
	// Vertical
	if (p->getX() == m_player->getX())
	{
		int x = p->getX(),
			y = p->getY() < m_player->getY() ? p->getY() : m_player->getY();
		for (int i = 0; i < abs(p->getY() - m_player->getY()); i++)
			if (moveBlocksProtester(x, y + i))
				return false;
		return true;
	}
	// Horizontal
	else if (p->getY() == m_player->getY())
	{
		int x = p->getX() < m_player->getX() ? p->getX() : m_player->getX(),
			y = p->getY();
		for (int i = 0; i < abs(p->getX() - m_player->getX()); i++)
			if (moveBlocksProtester(x + i, y))
				return false;
		return true;
	}
	else
		return false;
}

void StudentWorld::updateExitHeatMap()
{
	if (!m_updateExitHeatMap)
		return;
	
	initializeExitHeatMap();
	queue<Coord> q;
	q.push(Coord(60,60));
	m_exitHeatMap[60][60] = 0;
	int distance = 1, paths = 1, curPath = 1, targets = 0;
	while (!q.empty() && targets < m_numLeavers)
	{
		int x = q.front().x(), y = q.front().y(), numPushed = 0;
		q.pop();
		// UP
		if (!moveBlocksProtester(x, y + 1) && m_exitHeatMap[x][y + 1] == -1)
		{
			q.push(Coord(x, y + 1));	numPushed++;
			m_exitHeatMap[x][y + 1] = distance;
			targets += numLeaversSet(x, y + 1, distance);
		}
		// DOWN
		if (!moveBlocksProtester(x, y - 1) && m_exitHeatMap[x][y - 1] == -1)
		{
			q.push(Coord(x, y - 1));	numPushed++;
			m_exitHeatMap[x][y - 1] = distance;
			targets += numLeaversSet(x, y - 1, distance);
		}
		// LEFT
		if (!moveBlocksProtester(x - 1, y) && m_exitHeatMap[x - 1][y] == -1)
		{
			q.push(Coord(x - 1, y));	numPushed++;
			m_exitHeatMap[x - 1][y] = distance;
			targets += numLeaversSet(x - 1, y, distance);
		}
		// RIGHT
		if (!moveBlocksProtester(x + 1, y) && m_exitHeatMap[x + 1][y] == -1)
		{
			q.push(Coord(x + 1, y));	numPushed++;
			m_exitHeatMap[x + 1][y] = distance;
			targets += numLeaversSet(x + 1, y, distance);
		}

		paths += numPushed - 1;
		if (curPath == 1)
		{
			distance++;
			curPath = paths;
		}
		else
			curPath--;
	}
	m_updateExitHeatMap = false;
}

void StudentWorld::updatePlayerHeatMap()
{
	if (!m_updatePlayerHeatMap)
		return;
	
	initializePlayerHeatMap();
	queue<Coord> q;
	q.push(Coord(m_player->getX(), m_player->getY()));
	m_playerHeatMap[m_player->getX()][m_player->getY()] = 0;
	int distance = 1, paths = 1, curPath = 1, targets = 0;
	while (!q.empty() && targets < m_numTrackers)
	{
		int x = q.front().x(), y = q.front().y(), numPushed = 0;
		q.pop();
		// UP
		if (!moveBlocksProtester(x, y + 1) && m_playerHeatMap[x][y + 1] == -1)
		{
			q.push(Coord(x, y + 1));	numPushed++;
			m_playerHeatMap[x][y + 1] = distance;
			targets += numTrackersSet(x, y + 1, distance);
		}
		// DOWN
		if (!moveBlocksProtester(x, y - 1) && m_playerHeatMap[x][y - 1] == -1)
		{
			q.push(Coord(x, y - 1));	numPushed++;
			m_playerHeatMap[x][y - 1] = distance;
			targets += numTrackersSet(x, y - 1, distance);
		}
		// LEFT
		if (!moveBlocksProtester(x - 1, y) && m_playerHeatMap[x - 1][y] == -1)
		{
			q.push(Coord(x - 1, y));	numPushed++;
			m_playerHeatMap[x - 1][y] = distance;
			targets += numTrackersSet(x - 1, y, distance);
		}
		// RIGHT
		if (!moveBlocksProtester(x + 1, y) && m_playerHeatMap[x + 1][y] == -1)
		{
			q.push(Coord(x + 1, y));	numPushed++;
			m_playerHeatMap[x + 1][y] = distance;
			targets += numTrackersSet(x + 1, y, distance);
		}

		paths += numPushed - 1;
		if (curPath == 1)
		{
			distance++;
			curPath = paths;
		}
		else
			curPath--;
	}
	m_updatePlayerHeatMap = false;
}

GraphObject::Direction StudentWorld::findBestMoveToExit(Protester* p) const
{
	int x = p->getX(), y = p->getY();
	  // UP
	if (moveWithinBounds(x, y + 1)
		&& m_exitHeatMap[x][y + 1] == p->getDistanceFromTarget() - 1)
		return GraphObject::up;
	  // DOWN
	else if (moveWithinBounds(x, y - 1)
		&& m_exitHeatMap[x][y - 1] == p->getDistanceFromTarget() - 1)
		return GraphObject::down;
	  // LEFT
	else if (moveWithinBounds(x - 1, y)
		&& m_exitHeatMap[x - 1][y] == p->getDistanceFromTarget() - 1)
		return GraphObject::left;
	  // RIGHT
	else if (moveWithinBounds(x + 1, y)
		&& m_exitHeatMap[x + 1][y] == p->getDistanceFromTarget() - 1)
		return GraphObject::right;
	return GraphObject::none;
}

GraphObject::Direction StudentWorld::findBestMoveToPlayer(Protester* p) const
{
	int x = p->getX(), y = p->getY();
	// UP
	if (moveWithinBounds(x, y + 1)
		&& m_playerHeatMap[x][y + 1] == p->getDistanceFromTarget() - 1)
		return GraphObject::up;
	// DOWN
	else if (moveWithinBounds(x, y - 1)
		&& m_playerHeatMap[x][y - 1] == p->getDistanceFromTarget() - 1)
		return GraphObject::down;
	// LEFT
	else if (moveWithinBounds(x - 1, y)
		&& m_playerHeatMap[x - 1][y] == p->getDistanceFromTarget() - 1)
		return GraphObject::left;
	// RIGHT
	else if (moveWithinBounds(x + 1, y)
		&& m_playerHeatMap[x + 1][y] == p->getDistanceFromTarget() - 1)
		return GraphObject::right;
	return GraphObject::none;
}

bool StudentWorld::atIntersection(Actor* p) const
{
	GraphObject::Direction dir = p->getDirection();
	switch (dir)
	{
	case GraphObject::up:
	case GraphObject::down:
		return !moveBlocksProtester(p->getX() - 1, p->getY())
			|| !moveBlocksProtester(p->getX() + 1, p->getY());
	case GraphObject::left:
	case GraphObject::right:
		return !moveBlocksProtester(p->getX(), p->getY() - 1)
			|| !moveBlocksProtester(p->getX(), p->getY() + 1);
	default:
		return false;
	}
}

void StudentWorld::decrementProtesters()
{
	if (m_numProtesters > 0)
		m_numProtesters--;
}

///////////////////////////////////////////
//	PRIVATE HELPER FUNCTIONS
///////////////////////////////////////////

void StudentWorld::addActor(Actor* p)
{
	m_actors.push_back(p);
}

void StudentWorld::fillFieldWithDirt()
{
	for (int x = 0; x < VIEW_WIDTH; x++)
		for (int y = 0; y < VIEW_HEIGHT - SPRITE_HEIGHT; y++)
			m_field[x][y] = new Dirt(this, x, y);
}

void StudentWorld::createMineShaft()
{
	for (int x = 30; x < 34; x++)
	{
		for (int y = 4; y < 60; y++)
		{
			delete m_field[x][y];
			m_field[x][y] = nullptr;
		}
	}
}

void StudentWorld::distributeObjects(int num, Object type)
{
	for (int i = 0; i < num;)
	{
		int x = rand() % (VIEW_WIDTH - SPRITE_WIDTH), y;
		if (type == boulder)
			y = 20 + rand() % 36;
		else
			y = rand() % (VIEW_HEIGHT - 2*SPRITE_HEIGHT);
		  // Object cannot be in mine shaft
		if (x >= 26 && x <= 34)
			continue;
		  // If coordinates are within a radius 6 of another object, choose another
		bool tooclose = false;
		for (unsigned int k = 0; k < m_actors.size(); k++)
		{
			if (withinRadius(x, y, m_actors[k]->getX(), m_actors[k]->getY(), 6))
			{
				tooclose = true;
				break;
			}
		}
		if (tooclose)
			continue;
		  // Add object to the game
		Actor* p = nullptr;
		switch (type)
		{
		case boulder:
			p = new Boulder(this, x, y);
			digIfNeeded(p);
			break;
		case gold:
			p = new Gold(this, x, y);
			break;
		case oil:
			p = new OilBarrel(this, x, y);
			break;
		}
		addActor(p);
		i++;
	}
}

void StudentWorld::setDisplayText()
{
	int score = getScore() <= 999999 ? getScore() : 999999;
	int level = getLevel() <= 99 ? getLevel() : 99;
	int lives = getLives() <= 9 ? getLives() : 9;
	int health = m_player->getHP() * 100 / m_player->getMaxHP();
	int water = m_player->getWater() <= 99 ? m_player->getWater() : 99;
	int gold = m_player->getGold() <= 99 ? m_player->getGold() : 99;
	int sonar = m_player->getSonar() <= 99 ? m_player->getSonar() : 99;
	int oil = m_numOil <= 99 ? m_numOil : 99;
	
	char buffer[86];	//[5+6+2      + 5+2+2  + 7+1+2   + 6+4+2     + 5+2+2  + 5+2+2  + 7+2+2    + 10+2      +1]
				// Scr:	321000  Lvl: _2  Lives: 3  Hlth: _80%  Wtr: _2  Gld: _3  Sonar: _1  Oil Left: _2
	sprintf(buffer, "Scr: %06d  Lvl: %2d  Lives: %1d  Hlth: %3d%%  Wtr: %2d  Gld: %2d  Sonar: %2d  Oil Left: %2d",
		score, level, lives, health, water, gold, sonar, oil);
	string s = buffer;
	setGameStatText(s);
}

void StudentWorld::removeDeadActors()
{
	for (unsigned int i = 0; i < m_actors.size();)
	{
		if (m_actors[i]->isAlive())
			i++;
		else
		{
			delete m_actors[i];
			m_actors.erase(m_actors.begin() + i);
		}
	}
}

bool StudentWorld::withinBounds(int x, int y) const
{
	return x >= 0 && x < VIEW_WIDTH
		&& y >= 0 && y < VIEW_HEIGHT;
}

bool StudentWorld::moveWithinBounds(int x, int y) const
{
	return x >= 0 && x <= VIEW_WIDTH - SPRITE_WIDTH
		&& y >= 0 && y <= VIEW_HEIGHT - SPRITE_HEIGHT;
}

bool StudentWorld::square4x4HasDirt(int x, int y) const
{
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			if (withinBounds(x+i,y+j) && m_field[x+i][y+j] != nullptr)
				return true;
	return false;
}

bool StudentWorld::square4x1HasDirt(int x, int y) const
{
	for (int i = 0; i < 4; i++)
		if (withinBounds(x + i, y) && m_field[x + i][y] != nullptr)
			return true;
	return false;
}

bool StudentWorld::square4x4HasBoulder(int x, int y, Actor* p) const
{
	for (unsigned int i = 1; i < m_actors.size(); i++)	// start at 1 to avoid player
		if (m_actors[i] != p && m_actors[i]->blocksPlayer() /* boulder */
			&& withinRadius(x, y, m_actors[i]->getX(), m_actors[i]->getY(), 3))
			return true;
	return false;
}

bool StudentWorld::withinRadius(int x1, int y1, int x2, int y2, int radius) const
{
	// a^2 + b^2 = c^2
	int x = x2 - x1;
	int y = y2 - y1;
	return x*x + y*y <= radius*radius;
}

bool StudentWorld::playerCompletedLevel() const
{
	return m_numOil == 0;
}

void StudentWorld::initializeExitHeatMap()
{
	for (int x = 0; x < VIEW_WIDTH - SPRITE_WIDTH + 1; x++)
		for (int y = 0; y < VIEW_HEIGHT - SPRITE_HEIGHT + 1; y++)
			m_exitHeatMap[x][y] = -1;
}

void StudentWorld::initializePlayerHeatMap()
{
	for (int x = 0; x < VIEW_WIDTH - SPRITE_WIDTH + 1; x++)
		for (int y = 0; y < VIEW_HEIGHT - SPRITE_HEIGHT + 1; y++)
			m_playerHeatMap[x][y] = -1;
}

int StudentWorld::numLeaversSet(int x, int y, int distance)
{
	int found = 0;
	for (int i = 1; i < m_actors.size(); i++) // start at 1 to avoid player
	{
		if (m_actors[i]->canPickThingsUp() /* Protester */
			&& m_actors[i]->getX() == x && m_actors[i]->getY() == y)
		{
			Protester* p = dynamic_cast<Protester*>(m_actors[i]);
			if (p->isLeaving())
			{
				p->setDistanceFromTarget(distance);
				found++;
			}
		}
	}
	return found;
}

int StudentWorld::numTrackersSet(int x, int y, int distance)
{
	int found = 0;
	for (int i = 1; i < m_actors.size(); i++) // start at 1 to avoid player
	{
		if (m_actors[i]->canTrackPlayer() /* Hardcore */
			&& m_actors[i]->getX() == x && m_actors[i]->getY() == y)
		{
			Protester* p = dynamic_cast<Protester*>(m_actors[i]);
			if (!p->isLeaving())
			{
				p->setDistanceFromTarget(distance);
				found++;
			}
		}
	}
	return found;
}
