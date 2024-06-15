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
	isJumping = false;
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
	else if (isJumping) {// jUmping
		animationDirection = 2;
		isJumping = true;
		if (++frameCount > frameDelay) {
			frameCount = 0;
			if (++curFrame > maxJumpFrame) {
				curFrame = 9; //First frame of jump animation
			}
		}
	}
	else if (dir == 3) {// Climbing right
		animationDirection = 1;
		if (ClimableCollision()) {
			if (++frameCount > frameDelay) {
				frameCount = 0;
				if (++curFrame > maxWalkFrame) 
					curFrame = 2;
				y -= 32;
				x += 32;
				jumping(32, 1500);
				
			}
		}
	}
	else if (dir == 4) { // Climbing up left
		animationDirection = 0;
		if (ClimableCollision())
		{
			if (++frameCount > frameDelay)
			{
				frameCount = 0;
				if (++curFrame > maxWalkFrame)
					curFrame = 2; //start of walking frames
				y -= 32; // Climb up at a speed of 32 units per frame
				x -= 32;
				jumping(32, 1500);
			}
		}
	}
	else if (dir == 5) { // Climbing down
		if (ThroughableCollision())
		{
			y += 36; // Climb down a certain obj
		}
	}
	else { //represent that they hit the space bar and that mean direction = 0
		
		if (isJumping) { // Last frame jumping
			curFrame = 0;
		}
		isJumping = false;
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

bool Sprite::ClimableCollision()
{
	if (Climbable(x + frameWidth / 2, y + frameHeight + 5)) {
		//cout << "CLIMAB\n";
		return true;
	}
	else {
		return false;
	}
}
bool Sprite::ThroughableCollision()
{
	if (Throughable(x + frameWidth / 2, y + frameHeight + 5)) {
	//	cout << "THROUGH\n";
		return true;
	}
	else {
		return false;

	}
}
void Sprite::DrawSprites(int xoffset, int yoffset)
{
	int fx = (curFrame % animationColumns) * frameWidth;
	int fy = (curFrame / animationColumns) * frameHeight;

	if (isJumping) {
		// Draw jumping sprite
		al_draw_bitmap_region(image, fx, fy, frameWidth, frameHeight, x - xoffset, y - yoffset, 0);
	}
	else if (animationDirection == 1) {
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

int Sprite::jumping(int jump, const int JUMPIT)
{
	//handle jumping
	if (jump==JUMPIT) { 
		if (!collided(x + frameWidth/2, y + frameHeight + 5))
			jump = 0; 
	}
	else
	{
		y -= jump/3; 
		jump--; 
		curFrame=9;
	}

	if (jump<0) 
	{ 
		if (collided(x + frameWidth/2,  y + frameHeight))
		{ 
			jump = JUMPIT; 
			while (collided(x + frameWidth/2,y + frameHeight))
			{
				y -= 3;
			}
			// Resets to standing
			curFrame = 0;
			animationDirection = 1;

		} 
	}
	return jump;
}