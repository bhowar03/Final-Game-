// Brandon Howar Project 5
#include "cannon.h"
#include <allegro5\allegro.h>
#include <allegro5\allegro_primitives.h>

#include <allegro5\allegro_image.h>
#include "iceberg.h"
#include <string>

Cannon::~Cannon()
{
	al_destroy_bitmap(cannon);
}
Cannon::Cannon(int HEIGHT, int WIDTH)
{
	cannon = al_load_bitmap("cannon.png");
	captain = al_load_bitmap("captain.png");
	x = (WIDTH / 2);
	y = HEIGHT - 100;
	angle = 0.0f;
}
void Cannon::DrawCannon() {
	al_draw_rotated_bitmap(cannon, al_get_bitmap_width(cannon) / 2, al_get_bitmap_height(cannon) / 2, x, y, ALLEGRO_PI * angle / 180.0, 0);
	// Scaled
	al_draw_scaled_bitmap(captain, 0, 0, al_get_bitmap_width(captain), al_get_bitmap_height(captain), x+50, y, al_get_bitmap_width(captain) * 1.2, al_get_bitmap_height(captain) * 1.2, 0);
}

void Cannon::MoveLeft()
// Angles cannon towards left
{
	angle -= 0.8f;
	if (angle < -60.0f)
		angle = -60.0f;
}
void Cannon::MoveRight()
// Angles cannon towards right
{
	angle += 0.8f;
	if (angle > 60.0f)
		angle = 60.0f;
}
//Keeps movement
void Cannon::updateAngle(float angleChange) {
	angle += angleChange;
	if (angle < -60) angle = -60;
	else if (angle > 60) angle = 60;
}