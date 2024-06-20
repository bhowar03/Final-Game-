// Brandon Howar Project 3
#include <allegro5\allegro.h>
#include <allegro5\allegro_primitives.h>
#include <allegro5\allegro_image.h>
#include "snowballs.h"
#include "iceberg.h"
#include "cannon.h"
#include <cmath>


#define M_PI 3.1415926

Snowball::Snowball() {
	speed = 10;
	live = false;
	snowball = al_load_bitmap("snowball.png");
	sample = NULL;
	// SOUND EFFECTS
	sample = al_load_sample("pew.mp3");
	if (!sample) {
		exit(9);
	}
}
// Destroys bitmap
Snowball::~Snowball() {
	al_destroy_bitmap(snowball);
	al_destroy_sample(sample);

}

// Draws snowball like arrow
void Snowball::DrawSnowball() {
	if (live)
		al_draw_rotated_bitmap(snowball, al_get_bitmap_width(snowball) / 2, al_get_bitmap_height(snowball) / 2, x, y, 90, 0);
}
// Fires the snowball from the cannon and gets the direction based on cannon angle
void Snowball::FireSnowball(Cannon& cannon) {
	if (!live)
	{

		x = cannon.getX();
		y = cannon.getY();
		angle = ((cannon.getAngle() + 64.0) / 0.711) * ((2 * M_PI) / 360.0);
		live = true;
		al_play_sample(sample, 1, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
	}
}
// Updates the snowball to move at an angle
// Set live to false when goes off screen
void Snowball::UpdateSnowball(int WIDTH) {
	if (live)
	{
		x -= speed * cos(angle);
		y -= speed * sin(angle);
		if (x > WIDTH || x < 0 || y < 0)
			live = false;
	}

}
// SImilar to Kirby lab handling collision and incrementing score
void Snowball::CollideSnowball(Penguin penguins[], int cSize, Iceberg& Ice)
{
	if (live)
	{
		for (int j = 0; j < cSize; j++)
		{
			if (penguins[j].getLive())
			{
				if (x > (penguins[j].getX() - penguins[j].getBoundX()) &&
					x < (penguins[j].getX() + penguins[j].getBoundX()) &&
					y >(penguins[j].getY() - penguins[j].getBoundY()) &&
					y < (penguins[j].getY() + penguins[j].getBoundY()))
				{
					live = false;

					penguins[j].setLive(false);
					// Updates score when hitting a penguin
					Ice.Score();
				}
			}
		}
	}

}