// Brandon Howar Project 5
#include <allegro5\allegro.h>
#include <allegro5\allegro_primitives.h>
#include <allegro5\allegro_image.h>
#include "iceberg.h"
#include <string>
using namespace std;

ALLEGRO_BITMAP* images[5];
int icebergIndex;

Iceberg::~Iceberg()
{
	for (int i = 0; i < 5; ++i) {
		al_destroy_bitmap(images[i]);
	}
}
Iceberg::Iceberg(int HEIGHT, int WIDTH)
{
	//Stores iceberg images into array

	images[0] = al_load_bitmap("iceberg0.png");
	images[1] = al_load_bitmap("iceberg1.png");
	images[2] = al_load_bitmap("iceberg2.png");
	images[3] = al_load_bitmap("iceberg3.png");
	images[4] = al_load_bitmap("iceberg4.png");

	status[0] = al_load_bitmap("status0.png");
	status[1] = al_load_bitmap("status1.png");
	status[2] = al_load_bitmap("status2.png");
	status[3] = al_load_bitmap("status3.png");
	status[4] = al_load_bitmap("status4.png");
	icebergIndex = 0;
	x = 0;
	y = 800;
	lives = 5;
	boundx = al_get_bitmap_width(images[icebergIndex]);
	boundy = al_get_bitmap_height(images[icebergIndex]) - 250;
	score = 0;
}
void Iceberg::DrawIceberg()
{
	// Draws iceberg based on health remaining
	al_draw_bitmap(images[icebergIndex], x, y - 50, 0);
	al_draw_bitmap(status[icebergIndex], 550, 10, 0);
}