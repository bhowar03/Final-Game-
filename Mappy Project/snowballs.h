// Brandon Howar Project 5
#include "penguins.h"
#include "cannon.h"
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

class Snowball
{
public:
	Snowball();
	~Snowball();
	void DrawSnowball();
	void FireSnowball(Cannon& cannon);
	void UpdateSnowball(int WIDTH);
	void CollideSnowball(Penguin ghosts[], int cSize, Iceberg& Player);
private:
	int x;
	int y;
	bool live;
	float angle;
	int speed;
	ALLEGRO_BITMAP* snowball;
	ALLEGRO_SAMPLE* sample;
};

