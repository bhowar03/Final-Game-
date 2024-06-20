// Brandon Howar Project 5
#ifndef PENGUINSH
#define PENGUINSH
#include "iceberg.h"
class Penguin
{
public:
	Penguin();
	~Penguin();
	void DrawPenguin();
	void StartPenguin(int WIDTH, int HEIGHT);
	void UpdatePenguin();
	void CollidePenguin(Iceberg& Ice);
	int getBoundX() { return boundx; }
	int getBoundY() { return boundy; }
	int getX() { return x; }
	int getY() { return y; }
	bool getLive() { return live; }
	void setLive(bool l) { live = l; }
private:
	int x;
	int y;
	bool live;
	int speed;
	int boundx;
	int boundy;

	ALLEGRO_BITMAP* image;
};
#endif