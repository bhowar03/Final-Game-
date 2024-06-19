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
	maxDown = 2;
	maxUp = 5;
	maxLeft = 11;
	maxRight = 8;
	maxFrame = 12;
	curFrame = 0;
	frameCount = 0;
	frameDelay = 5;
	frameWidth = 43;
	frameHeight = 43;
	animationColumns = 6;
	animationDirection = 3;
	walking = false;

	image = al_load_bitmap("penguinsheet.png");
	al_convert_mask_to_alpha(image, al_map_rgb(0,0,255));
}

void Sprite::UpdateSprites(int width, int height, int dir)
{
	int oldx = x;
	int oldy = y;
	
	if (dir != animationDirection) {
		switch (dir) {
		case 0: 
			curFrame = 0; // Down
			break;
		case 1: 
			curFrame = 9; // left
			break;
		case 2:
			curFrame = 6; // Right
			break;
		case 3:
			curFrame = 3; // Up
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
				curFrame = 0; // first frame of down
			}
			break;
		case 1:
			if (curFrame < maxLeft) {
				curFrame++;
			}
			else {
				curFrame = 9; // first of left
			}
			break;
		case 2:
			if (curFrame < maxRight) {
				curFrame++;
			}
			else {
				curFrame = 6; // first of right
			}
			break;
		case 3:
			if (curFrame < maxUp) {
				curFrame++;
			}
			else {
				curFrame = 3; // first of up
			}
			break;
		}
	}
	switch (dir) {
	case 0:
		y += 2; // Down
		break;
	case 1:
		x -= 2; // left
		break;
	case 2:
		x += 2; // right
		break;
	case 3:
		y -= 2; // UP
		break;
	}
	
	if (collided(x, y) || collided(x + frameWidth, y) || collided(x, y + frameHeight) || collided(x + frameWidth, y + frameHeight)) { // collides
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

bool Sprite::endLevel() // end of level
{
	if (endLevelValue(x + frameWidth/2, y + frameHeight + 5))
		return true;
	else
		return false;
}
bool Sprite::endGame() // end of level
{
	if (endValue(x + frameWidth / 2, y + frameHeight + 5))
		return true;
	else
		return false;
}
bool Sprite::point1() // end of level
{
	if (points1(x + frameWidth / 2, y + frameHeight + 5))
		return true;
	else
		return false;
}
bool Sprite::point2() // end of level
{
	if (points2(x + frameWidth / 2, y + frameHeight + 5))
		return true;
	else
		return false;
}
bool Sprite::point3() // end of level
{
	if (points3(x + frameWidth / 2, y + frameHeight + 5))
		return true;
	else
		return false;
}
bool Sprite::point4() // end of level
{
	if (points4(x + frameWidth / 2, y + frameHeight + 5))
		return true;
	else
		return false;
}
bool Sprite::point5() // end of level
{
	if (points5(x + frameWidth / 2, y + frameHeight + 5))
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
// Setters
void Sprite::setX(int x) {
	this->x = x;
}
void Sprite::setY(int y) {
	this->y = y;
}