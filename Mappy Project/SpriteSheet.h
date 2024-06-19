// Brandon Howar Project 4

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <iostream>
using namespace std;
class Sprite
{
	friend int collided(int x, int y);
	friend bool endValue(int x, int y);
	friend bool endLevelValue(int x, int y);
	friend bool points1(int x, int y);
	friend bool points2(int x, int y);
	friend bool points3(int x, int y);
	friend bool points4(int x, int y);
	friend bool points5(int x, int y);

public:
	Sprite();
	~Sprite();
	void InitSprites(int width, int height);
	void UpdateSprites(int width, int height, int dir); //dir 1 = right, 0 = left, 2 = Standing Still
	void DrawSprites(int xoffset, int yoffset);
	float getX() {return x;}
	float getY() {return y;}

	void setX(int x); // Setters
	void setY(int y);
	int getWidth() {return frameWidth;}
	int getHeight() {return frameHeight;}
	bool endLevel();
	bool endGame();
	bool point1();
	bool point2();
	bool point3();
	bool point4();
	bool point5();
	

private:
	float x;
	float y;
	int points;
	int maxFrame;
	int maxUp, maxRight, maxDown, maxLeft;
	int curFrame;
	int frameCount;
	int frameDelay;
	int frameWidth;
	int frameHeight;
	int animationColumns;
	int animationRows;
	int animationDirection;
	// Added
	int maxWalkFrame;
	bool walking;
	
	
	ALLEGRO_BITMAP *image;
};