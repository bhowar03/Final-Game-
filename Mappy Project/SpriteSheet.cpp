// Brandon Howar Lab 11
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
	frameDelay = 10;
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
	walking = true;
	if (dir == 1) { //right key
		animationDirection = 1;
		x += 2;
		if (++frameCount > frameDelay)
		{
			frameCount = 0;
			if (++curFrame > maxRight)
				curFrame = 8; // Start Walking
		}
	}
	else if (dir == 2) { //left key
		animationDirection = 0;
		x -= 2;
		if (++frameCount > frameDelay)
		{
			frameCount = 0;
			if (++curFrame > maxLeft && animationDirection == 0)
				curFrame = 4;
		}
	}
	else if (dir == 3) { //Up key
		animationDirection = 2;
		y -= 2;
		if (++frameCount > frameDelay)
		{
			frameCount = 0;
			if (++curFrame > maxUp)
				curFrame = 12;
		}
	}
	else if (dir == 4) { //Down key
		animationDirection = 3;
		y += 2;
		if (++frameCount > frameDelay)
		{
			frameCount = 0;
			if (++curFrame > maxDown)
				curFrame = 0;
		}
	}
	else {
		walking = false;
		if (walking == false) {
			if (animationDirection == 1)
				curFrame = 8; // Stand still facing right
			else if (animationDirection == 0)
				curFrame = 4; // Stand still facing left
			else if (animationDirection == 2)
				curFrame = 12; // Stand still facing up
			else if (animationDirection == 3)
				curFrame = 0; // Stand still facing down
		}
	}
	

	//check for collided with foreground tiles
	if (animationDirection == 0)
	{
		if (collided(x, y + frameHeight)) { // Collision detection to the left
			x = oldx;
			y = oldy;
		}
	}
	else if (animationDirection == 1)
	{
		if (collided(x + frameWidth, y + frameHeight)) { // Collision detection to the right
			x = oldx;
			y = oldy;
		}
	}
	else if (animationDirection == 2)
	{
		if (collided(x + frameWidth, y + frameHeight)) { // Collision detection upwards
			x = oldx;
			y = oldy;
		}
	}
	else if (animationDirection == 3)
	{
		if (collided(x + frameWidth, y + frameHeight)) { // Collision detection downwards
			x = oldx;
			y = oldy;
		}
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
	//if (walking) {
	//	// Draw Walking
	//	al_draw_bitmap_region(image, fx, fy, frameWidth, frameHeight, x - xoffset, y - yoffset, 0);
	//}
	if (animationDirection == 1) {
		// Draw walking right sprite
		al_draw_bitmap_region(image, fx, fy, frameWidth, frameHeight, x - xoffset, y - yoffset, 0);
	}
	else if (animationDirection == 0) {
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

