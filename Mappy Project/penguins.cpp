// Brandon Howar Project 3
#include <allegro5\allegro.h>
#include <allegro5\allegro_primitives.h>
#include <allegro5\allegro_image.h>
#include "penguins.h"

Penguin::Penguin()
{
	image = al_load_bitmap("penguin1.png");
	live = false;
	speed = 1;
	boundx = al_get_bitmap_width(image);
	boundy = al_get_bitmap_height(image);
}
Penguin::~Penguin()
{
	al_destroy_bitmap(image);
}
void Penguin::DrawPenguin()
{
	if (live)
	{
		al_draw_bitmap(image, x, y, 0);
	}
}
//Starts penguin from top of screen and randomly place along X coord
void Penguin::StartPenguin(int WIDTH, int HEIGHT)
{
	if (!live)
	{
		if (rand() % 500 == 0)
		{
			live = true;
			// Makes penguins not spawn outside of bounds
			x = rand() % (WIDTH - boundx);
			y = 0;
		}
	}
}
// Drops penguin slowly down screen
void Penguin::UpdatePenguin()
{
	if (live)
	{
		y += speed;
	}

}
// Checks if penguin hit iceberg, removes lives and makes iceberg cracked
void Penguin::CollidePenguin(Iceberg& Ice)
{
	if (live)
	{
		if (x - boundx < Ice.getX() + Ice.getBoundX() &&
			x + boundx > Ice.getX() - Ice.getBoundX() &&
			y - boundy < Ice.getY() + Ice.getBoundY() &&
			y + boundy > Ice.getY() - Ice.getBoundY())
		{
			Ice.ChangeImage();
			Ice.removeLife();
			live = false;
		}
		else if (x < 0)
		{
			live = false;
		}
	}
}
