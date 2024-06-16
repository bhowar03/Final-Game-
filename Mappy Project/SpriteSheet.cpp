// Brandon Howar Project 4
#include "SpriteSheet.h"

Sprite::Sprite()
{
	image=NULL;
}
Sprite::~Sprite()
{
	al_destroy_bitmap(image);
}
void Sprite::InitSprites(int width, int height)
{
	x = 80;
	y = -10;

	// Variables for ease
	maxDown = 3;
	maxUp = 15;
	maxLeft = 7;
	maxRight = 11;
	maxFrame = 16;
	curFrame = 0;
	frameCount = 0;
	frameDelay = 5;
	frameWidth = 64;
	frameHeight = 64;
	animationColumns = 4;
	animationDirection = 3;
	walking = false;

	image = al_load_bitmap("spritesheet2.png");
	//al_convert_mask_to_alpha(image, al_map_rgb(255,0,255));
}

void Sprite::UpdateSprites(int width, int height, int dir)
{
	int oldx = x;
	int oldy = y;
	
	if (dir != animationDirection) {
		switch (dir) {
		case 0: 
			curFrame = 1; // Down
			break;
		case 1: 
			curFrame = 4; // left
			break;
		case 2:
			curFrame = 8; // Right
			break;
		case 3:
			curFrame = 12; // Up
			break;
		}
		animationDirection = dir;
	}
	if (++frameCount > frameDelay) {
		frameCount = 0;
		switch (dir) {
		case 0:
			if (curFrame < maxDown) {
				curFrame++;
			}
			else {
				curFrame = 1;
			}
			break;
		case 1:
			if (curFrame < maxLeft) {
				curFrame++;
			}
			else {
				curFrame = 4;
			}
			break;
		case 2:
			if (curFrame < maxRight) {
				curFrame++;
			}
			else {
				curFrame = 8;
			}
			break;
		case 3:
			if (curFrame < maxUp) {
				curFrame++;
			}
			else {
				curFrame = 12;
			}
			break;
		}
	}
	switch (dir) {
	case 0:
		y += 2;
		break;
	case 1:
		x -= 2;
		break;
	case 2:
		x += 2;
		break;
	case 3:
		y -= 2;
		break;
	}
	
	if (collided(x, y) || collided(x + frameWidth, y) || collided(x, y + frameHeight) || collided(x + frameWidth, y + frameHeight)) {
		x = oldx;
		y = oldy;
	}

	// ceiling
	if (y < 32) {
		y = 33;
	}
	// Floor
	if (y + frameHeight > height) {
		y = height - frameHeight;
	}
}

bool Sprite::CollisionEndBlock()
{
	if (endValue(x + frameWidth/2, y + frameHeight + 5))
		return true;
	else
		return false;
}


void Sprite::DrawSprites(int xoffset, int yoffset)
{
	int fx = (curFrame % animationColumns) * frameWidth;
	int fy = (curFrame / animationColumns) * frameHeight;
	
	if (animationDirection == 0) {
		// Draw walking right sprite
		al_draw_bitmap_region(image, fx, fy, frameWidth, frameHeight, x - xoffset, y - yoffset, 0);
	}
	else if (animationDirection == 1) {
		// Draw walking left sprite
		al_draw_bitmap_region(image, fx, fy, frameWidth, frameHeight, x - xoffset, y - yoffset, 0);
	}
	else if (animationDirection == 2) {
		// Draw walking Up sprite
		al_draw_bitmap_region(image, fx, fy, frameWidth, frameHeight, x - xoffset, y - yoffset, 0);
	}
	else if (animationDirection == 3) {
		// Draw walking Down sprite
		al_draw_bitmap_region(image, fx, fy, frameWidth, frameHeight, x - xoffset, y - yoffset, 0);
	}
	else {
		// Draw standing still sprite
		al_draw_bitmap_region(image, 0, 0, frameWidth, frameHeight, x - xoffset, y - yoffset, 0);
	}
}

