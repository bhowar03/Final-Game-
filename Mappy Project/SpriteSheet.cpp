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
	maxWalkFrame = 8;
	maxFrame = 12;
	curFrame = 0;
	frameCount = 0;
	frameDelay = 6;
	frameWidth = 50;
	frameHeight = 64;
	animationColumns = 8;
	animationDirection = 1;

	image = al_load_bitmap("guy.bmp");
	al_convert_mask_to_alpha(image, al_map_rgb(255,0,255));
}

void Sprite::UpdateSprites(int width, int height, int dir)
{
	int oldx = x;
	int oldy = y;

	if (dir == 1) { //right key
		animationDirection = 1;
		x += 2;
		if (++frameCount > frameDelay)
		{
			frameCount = 0;
			if (++curFrame > maxWalkFrame)
				curFrame = 2; // Start Walking
		}
	}
	else if (dir == 0) { //left key
		animationDirection = 0;
		x -= 2;
		if (++frameCount > frameDelay)
		{
			frameCount = 0;
			if (++curFrame > maxWalkFrame)
				curFrame = 2;
		}
	}
	

	//check for collided with foreground tiles
	if (animationDirection==0)
	{ 
		if (collided(x, y + frameHeight)) { //collision detection to the left
			x = oldx; 
			y= oldy;
		}

	}
	else if (animationDirection ==1)
	{ 
		if (collided(x + frameWidth, y + frameHeight)) { //collision detection to the right
			x = oldx; 
			y= oldy;
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
	if (animationDirection == 1) {
		// Draw walking right sprite
		al_draw_bitmap_region(image, fx, fy, frameWidth, frameHeight, x - xoffset, y - yoffset, 0);
	}
	else if (animationDirection == 0) {
		// Draw walking left sprite
		al_draw_bitmap_region(image, fx, fy, frameWidth, frameHeight, x - xoffset, y - yoffset, ALLEGRO_FLIP_HORIZONTAL);
	}
	else {
		// Draw standing still sprite
		al_draw_bitmap_region(image, 0, 0, frameWidth, frameHeight, x - xoffset, y - yoffset, 0);
	}
}

