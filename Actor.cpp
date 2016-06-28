#include "Actor.h"
#include "StudentWorld.h"

//////////	Actor

StudentWorld* Actor::getWorld() const
{
	return m_world;
}

void Actor::makePerpendicularTurn()
{
	switch (getDirection())
	{
	case up:
	case down:
		if (rand() % 2)
			setDirection(right);
		else
			setDirection(left);
		break;
	case left:
	case right:
		if (rand() % 2)
			setDirection(up);
		else
			setDirection(down);
		break;
	}
}

void Actor::make180Turn()
{
	switch (getDirection())
	{
	case up:
		setDirection(down);		break;
	case down:
		setDirection(up);		break;
	case left:
		setDirection(right);	break;
	case right:
		setDirection(left);		break;
	}
}

bool Actor::move()
{
	if (!canMove())
		return false;
	int x, y;
	getWorld()->dirToCoord(this, x, y);
	moveTo(x, y);
	return true;
}

//////////	>	Agent

void Agent::annoy(unsigned int amt, Actor* annoyer)
{
	m_hp -= amt;
	annoyAux(annoyer);
}

//////////	>	>	FrackMan

void FrackMan::doSomething()
{
	if (!isAlive())
		return;

	  // Check if 4x4 overlaps with any dirt. If yes, dig.
	if (getWorld()->digIfNeeded(this))
		getWorld()->playSound(SOUND_DIG);

	  // Check if player pressed a key
	int value;
	if (getWorld()->getKey(value))
	{
		switch (value)
		{
		case KEY_PRESS_UP:
			if (getDirection() == up)
				move();
			else
				setDirection(up);
			break;
		case KEY_PRESS_DOWN:
			if (getDirection() == down)
				move();
			else
				setDirection(down);
			break;
		case KEY_PRESS_LEFT:
			if (getDirection() == left)
				move();
			else
				setDirection(left);
			break;
		case KEY_PRESS_RIGHT:
			if (getDirection() == right)
				move();
			else
				setDirection(right);
			break;
		case KEY_PRESS_ESCAPE:
			setDead();
			break;
		case KEY_PRESS_SPACE:
			if (m_water)
			{
				getWorld()->squirtIfNeeded();
				m_water--;
			}
			break;
		case 'z':
		case 'Z':
			if (m_sonar)
			{
				getWorld()->playSound(SOUND_SONAR);
				getWorld()->revealAllNearby();
				m_sonar--;
			}
			break;
		case KEY_PRESS_TAB:
			if (m_gold)
			{
				getWorld()->dropGoldIfNeeded();
				m_gold--;
			}
			break;
		}
	}
}

bool FrackMan::canMove()
{
	return !getWorld()->moveBlocksDigger(this);
}

void FrackMan::annoyAux(Actor* annoyer)
{
	if (getHP() <= 0)
		setDead();
}

//////////	>	>	Protester

Protester::Protester(StudentWorld* ip, int imageID, int hp, int points)
	: Agent(ip, imageID, 60, 60, hp, left)
{
	m_leaving = false;
	m_points = points;
	m_distance = 8 + rand() % 52;
	m_restingTicks = 3 - ip->getLevel() / 4 > 0 ? 3 - ip->getLevel() / 4 : 0;
	m_shoutCooldown = 0;
	m_turnCooldown = 0;
	m_distanceFromTarget = 0;
	setVisible(true);
}

Protester::~Protester()
{
	getWorld()->decrementProtesters();
	getWorld()->decrementNumLeavers();
}

void Protester::setRest(int k)
{
	if (k < 0)
		m_restingTicks = 3 - getWorld()->getLevel() / 4 > 0 ? 3 - getWorld()->getLevel() / 4 : 0;
	else
		m_restingTicks = k;
}

void Protester::annoy(unsigned int amt, Actor* annoyer)
{
	if (!m_leaving)
		Agent::annoy(amt, annoyer);
}

void Protester::doSomething()
{
	if (!isAlive())
		return;

	if (rest())
	{
		m_restingTicks--;
		return;
	}

	// Update non-resting counters
	m_shoutCooldown--;
	m_turnCooldown--;
	setRest();

	// Protester is leaving the field
	if (isLeaving())
	{
		if (getX() == 60 && getY() == 60)
			setDead();
		else
		{
			getWorld()->updateExitHeatMap();
			Direction dir = getWorld()->findBestMoveToExit(this);
			if (dir != none)
			{
				setDirection(dir);
				move();
				m_distanceFromTarget--;
			}
		}
		return;
	}
	// Protester is next to, and facing, player
	if (getWorld()->actorNextToPlayer(this) && getWorld()->actorFacingPlayer(this))
	{
		if (m_shoutCooldown <= 0)
		{
			getWorld()->shout(this);
			m_shoutCooldown = 15;
			m_shoutCooldown = 0;
			setRest(15 * (3 - getWorld()->getLevel() / 4 > 0 ? 3 - getWorld()->getLevel() / 4 : 0));
			return;
		}
	}
	// If Protester is Hardcore, check if he can track down player
	if (canTrackPlayer() && !getWorld()->actorNextToPlayer(this))
	{
		if (doSomethingAux())
		{
			m_distanceFromTarget--;
			return;
		}
	}
	// Protester is a straight shot from the player
	if (!getWorld()->actorNextToPlayer(this) && getWorld()->unblockedLineOfSightToPlayer(this))
	{
		getWorld()->changeDirToPlayer(this);
		move();
		m_distance = 0;
		return;
	}
	// Decrement available distance, choose a new direction
	m_distance--;
	if (m_distance <= 0)
	{
		changeDirAndMove();
		m_distance = 8 + rand() % 52;
	}
	// Protester is at an intersection
	else if (getWorld()->atIntersection(this) && m_turnCooldown <= 0)
	{
		makePerpendicularTurn();
		if (getWorld()->moveBlocksProtester(this))
			make180Turn();
		m_distance = 8 + rand() % 52;
		m_turnCooldown = 200;
	}
	// Protester attempts to move 1 step
	else if (!move())
		m_distance = 0;
}

void Protester::setDistanceFromTarget(int distance)
{
	m_distanceFromTarget = distance;
}

int Protester::getDistanceFromTarget() const
{
	return m_distanceFromTarget;
}

void Protester::changeDirAndMove()
{
	int dir = rand() % 4; // random direction
	for (int i = 0; i < 4; i++, dir = (dir + 1) % 4)
	{
		// check if it can move in any new direction, if yes move there
		setDirection(static_cast<Direction>(dir + 1));
		if (!getWorld()->moveBlocksProtester(this))
		{
			move();
			m_distance--;
			break;
		}
	}
}

bool Protester::canMove()
{
	return !getWorld()->moveBlocksProtester(this);
}

void Protester::annoyAux(Actor* annoyer)
{
	if (getHP() > 0)
	{
		getWorld()->playSound(SOUND_PROTESTER_ANNOYED);
		setRest(50 > 100 - getWorld()->getLevel() * 10 ? 50 : 100 - getWorld()->getLevel() * 10);
	}
	else
	{
		setLeaving();
		getWorld()->incrementNumLeavers();
		if (canTrackPlayer())
			getWorld()->decrementNumTrackers();
		getWorld()->playSound(SOUND_PROTESTER_GIVE_UP);
		setRest(0);
		if (annoyer->blocksPlayer())	// boulder
			getWorld()->increaseScore(500);
		else
			getWorld()->increaseScore(m_points);
	}
}

//////////	>	>	>	RegularProtester

void RegularProtester::addGold()
{
	getWorld()->increaseScore(25);
	setLeaving();
}

//////////	>	>	>	HardcoreProtester

void HardcoreProtester::addGold()
{
	getWorld()->increaseScore(50);
	setRest(50 > 100 - getWorld()->getLevel() * 10 ? 50 : 100 - getWorld()->getLevel() * 10);
}

bool HardcoreProtester::doSomethingAux()
{
	getWorld()->updatePlayerHeatMap();
	int M = 16 + getWorld()->getLevel() * 2;
	if (getDistanceFromTarget() > M)
		return false;
	Direction dir = getWorld()->findBestMoveToPlayer(this);
	if (dir != none)
	{
		setDirection(dir);
		move();
	}
}

//////////	>	Boulder

Boulder::~Boulder()
{
	getWorld()->setExitHeatMap(true);
}

void Boulder::doSomething()
{
	if (!isAlive())
		return;

	if (m_state == stable)
	{
		if (!getWorld()->moveBlocksBoulder(this))
			m_state = waiting;
	}
	else if (m_state == waiting)
	{
		if (wait_time)
			wait_time--;
		else
		{
			m_state = falling;
			getWorld()->playSound(SOUND_FALLING_ROCK);
		}
	}
	else // falling
	{
		getWorld()->annoyAllNearbyAgents(this, 3, 100);
		getWorld()->setExitHeatMap(true);
		if (!move())
			setDead();
	}
}

bool Boulder::canMove()
{
	return !getWorld()->moveBlocksBoulder(this);
}

//////////	>	Squirt

void Squirt::doSomething()
{
	if (getWorld()->annoyAllNearbyAgents(this, 3, 2, true))
		setDead();
	if (!m_distance)
		setDead();
	else
	{
		if (!move())
			setDead();
		else
			m_distance--;
	}
}

bool Squirt::canMove()
{
	return !getWorld()->moveBlocksSquirt(this);
}

//////////	>	ActivatingObject

void ActivatingObject::doSomething()
{
	if (!isAlive())
		return;
	getWorld()->revealIfNearby(this);
	doSomethingAux();
}

//////////	>	>	OilBarrel

void OilBarrel::doSomethingAux()
{
	getWorld()->activateIfNeeded(this, StudentWorld::oil);
}

//////////	>	>	Gold

void Gold::doSomethingAux()
{
	getWorld()->activateIfNeeded(this, StudentWorld::gold);
};

//////////	>	>	Temporary

void Temporary::doSomethingAux()
{
	doSomethingAux2();
	if (!m_ticks)
		setDead();
	else
		m_ticks--;
}

//////////	>	>	>	DroppedGold

void DroppedGold::doSomethingAux2()
{
	getWorld()->activateIfNeeded(this, StudentWorld::gold);
}

//////////	>	>	>	Sonar

Sonar::Sonar(StudentWorld* ip, int startX, int startY)
	: Temporary(ip, IID_SONAR, startX, startY, 100 > 300 - 10 * ip->getLevel() ? 100 : 300 - 10 * ip->getLevel())
{
	setVisible(true);
}

void Sonar::doSomethingAux2()
{
	getWorld()->activateIfNeeded(this, StudentWorld::sonar);
}

//////////	>	>	>	WaterPool

WaterPool::WaterPool(StudentWorld* ip, int startX, int startY)
	: Temporary(ip, IID_WATER_POOL, startX, startY, 100 > 300 - 10 * ip->getLevel() ? 100 : 300 - 10 * ip->getLevel())
{
	setVisible(true);
}

void WaterPool::doSomethingAux2()
{
	getWorld()->activateIfNeeded(this, StudentWorld::water);
}
