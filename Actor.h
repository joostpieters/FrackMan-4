#ifndef ACTOR_H_
#define ACTOR_H_

#include "GraphObject.h"

class StudentWorld;

/////////////////////////////////
//	Actor
//		Agent
//			FrackMan
//			Protester
//				RegularProtester
//				HardcoreProtester
//		Dirt
//		Boulder
//		Squirt
//		ActivatingObject
//			OilBarrel
//			Gold
//			Temporary
//				DroppedGold
//				Sonar
//				WaterPool
/////////////////////////////////
						

//////////	Actor

class Actor : public GraphObject
{
public:
	Actor(StudentWorld* ip, int imageID, int startX, int startY, Direction dir = right, double size = 1.0, unsigned int depth = 0)
		: GraphObject(imageID, startX, startY, dir, size, depth), m_world(ip), m_alive(true) {}
	virtual ~Actor() {}
	StudentWorld* getWorld() const;
	bool isAlive() const { return m_alive; }
	void setDead() { m_alive = false; }
	void makePerpendicularTurn();
	void make180Turn();
	bool move();
	virtual void annoy(unsigned int amt, Actor* annoyer) { return; }
	virtual bool blocksPlayer() const { return false; }
	virtual bool canPickThingsUp() const { return false; }
	virtual bool playerCanPickUp() const { return false; }
	virtual bool canTrackPlayer() const { return false; }
	virtual void doSomething() = 0;
private:
	virtual bool canMove() { return false; }
private:
	StudentWorld* m_world;
	bool m_alive;
};

//////////	>	Agent

class Agent : public Actor
{
public:
	Agent(StudentWorld* ip, int imageID, int startX, int startY, int hp, Direction dir = right, double size = 1.0, unsigned int depth = 0)
		: Actor(ip, imageID, startX, startY, dir, size, depth), m_hp(hp), m_maxhp(hp)
	{
		setVisible(true);
	}
	virtual ~Agent() {}
	int getHP() const { return m_hp; }
	int getMaxHP() const { return m_maxhp; }
	virtual bool canPickThingsUp() const { return true; }
	virtual void annoy(unsigned int amt, Actor* annoyer);
	virtual void addGold() = 0;
private:
	virtual void annoyAux(Actor* annoyer) = 0;
private:
	int m_hp;
	int m_maxhp;
};

//////////	>	>	FrackMan

class FrackMan : public Agent
{
public:
	FrackMan(StudentWorld* ip)
		: Agent(ip, IID_PLAYER, 30, 60, 10), m_water(5), m_sonar(1), m_gold(0) {}
	virtual ~FrackMan() {}
	int getWater() const { return m_water; }
	int getSonar() const { return m_sonar; }
	int getGold() const { return m_gold; }
	void addWater() { m_water += 5; }
	void addSonar() { m_sonar++; }
	virtual void addGold() { m_gold++; }
	virtual void doSomething();
private:
	virtual bool canMove();
	virtual void annoyAux(Actor* annoyer);
private:
	int m_water;
	int m_sonar;
	int m_gold;
};

//////////	>	>	Protester

class Protester : public Agent
{
public:
	Protester(StudentWorld* ip, int imageID, int hp, int points);
	virtual ~Protester();
	bool rest() const { return m_restingTicks > 0; }
	bool isLeaving() const { return m_leaving; }
	void setRest(int k = -1);
	void setLeaving() { m_leaving = true; }
	virtual void annoy(unsigned int amt, Actor* annoyer);
	virtual void doSomething();
	void setDistanceFromTarget(int distance);
	int getDistanceFromTarget() const;
private:
	void changeDirAndMove();
	virtual bool canMove();
	virtual void annoyAux(Actor* annoyer);
	virtual bool doSomethingAux() { return false; }
private:
	bool m_leaving;
	int m_points;
	int m_distance;
	int m_restingTicks;
	int m_shoutCooldown;
	int m_turnCooldown;
	int m_distanceFromTarget;
};

//////////	>	>	>	RegularProtester

class RegularProtester : public Protester
{
public:
	RegularProtester(StudentWorld* ip)
		: Protester(ip, IID_PROTESTER, 5, 100) {}
	virtual ~RegularProtester() {}
	virtual void addGold();
};

//////////	>	>	>	HardcoreProtester

class HardcoreProtester : public Protester
{
public:
	HardcoreProtester(StudentWorld* ip)
		: Protester(ip, IID_HARD_CORE_PROTESTER, 20, 250) {}
	virtual ~HardcoreProtester() {}
	virtual void addGold();
	virtual bool canTrackPlayer() const { return true; }
private:
	virtual bool doSomethingAux();
};

//////////	>	Dirt

class Dirt : public Actor
{
public:
	Dirt(StudentWorld* ip, int startX, int startY)
		: Actor(ip, IID_DIRT, startX, startY, right, .25, 3)
	{
		setVisible(true);
	}
	virtual ~Dirt() {}
	virtual void doSomething() {return;}
};

//////////	>	Boulder

class Boulder : public Actor
{
public:
	enum State { stable, waiting, falling };

	Boulder(StudentWorld* ip, int startX, int startY)
		: Actor(ip, IID_BOULDER, startX, startY, down, 1.0, 1), m_state(stable), wait_time(30)
	{
		setVisible(true);
	}
	virtual ~Boulder();
	virtual bool blocksPlayer() const { return true; }
	virtual void doSomething();
private:
	virtual bool canMove();
private:
	State m_state;
	int wait_time;
};

//////////	>	Squirt

class Squirt : public Actor
{
public:
	Squirt(StudentWorld* ip, int startX, int startY, Direction dir)
		: Actor(ip, IID_WATER_SPURT, startX, startY, dir, 1.0, 1), m_distance(4)
	{
		setVisible(true);
	}
	virtual ~Squirt() {}
	virtual void doSomething();
private:
	virtual bool canMove();
private:
	int m_distance;
};

//////////	>	ActivatingObject

class ActivatingObject : public Actor
{
public:
	ActivatingObject(StudentWorld* ip, int imageID, int startX, int startY)
		: Actor(ip, imageID, startX, startY, right, 1.0, 2) {}
	virtual ~ActivatingObject() {}
	virtual void doSomething();
private:
	virtual void doSomethingAux() = 0;

};

//////////	>	>	OilBarrel

class OilBarrel : public ActivatingObject
{
public:
	OilBarrel(StudentWorld* ip, int startX, int startY)
		: ActivatingObject(ip, IID_BARREL, startX, startY) {}
	virtual ~OilBarrel() {}
	virtual bool playerCanPickUp() const {return true;}
private:
	virtual void doSomethingAux();
};

//////////	>	>	Gold

class Gold : public ActivatingObject
{
public:
	Gold(StudentWorld* ip, int startX, int startY)
		: ActivatingObject(ip, IID_GOLD, startX, startY) {}
	virtual ~Gold() {}
	virtual bool playerCanPickUp() const {return true;}
private:
	virtual void doSomethingAux();
};

//////////	>	>	Temporary

class Temporary : public ActivatingObject
{
public:
	Temporary(StudentWorld* ip, int imageID, int startX, int startY, int ticks)
		: ActivatingObject(ip, imageID, startX, startY), m_ticks(ticks) {}
	virtual ~Temporary() {}
private:
	virtual void doSomethingAux();
	virtual void doSomethingAux2() = 0;
private:
	int m_ticks;
};

//////////	>	>	>	DroppedGold

class DroppedGold : public Temporary
{
public:
	DroppedGold(StudentWorld* ip, int startX, int startY)
		: Temporary(ip, IID_GOLD, startX, startY, 100)
	{
		setVisible(true);
	}
	virtual ~DroppedGold() {}
private:
	virtual void doSomethingAux2();
};

//////////	>	>	>	Sonar

class Sonar : public Temporary
{
public:
	Sonar(StudentWorld* ip, int startX, int startY);
	virtual ~Sonar() {}
	virtual bool playerCanPickUp() const {return true;}
private:
	virtual void doSomethingAux2();
};

//////////	>	>	>	WaterPool

class WaterPool : public Temporary
{
public:
	WaterPool(StudentWorld* ip, int startX, int startY);
	virtual ~WaterPool() {}
	virtual bool playerCanPickUp() const {return true;}
private:
	virtual void doSomethingAux2();
};

#endif // ACTOR_H_
