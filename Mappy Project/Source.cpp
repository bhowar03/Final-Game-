// Brandon Howar Project 4
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include "SpriteSheet.h"
#include "mappy_A5.h"
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <iostream>
#include "iceberg.h"
#include "penguins.h"
#include "snowballs.h"
#include "cannon.h"
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

using namespace std;

int collided(int x, int y);  //Tile Collision
bool endValue(int x, int y); //End Block with the User Value = 5
bool endLevelvalue(int x, int y); // value 7
bool points1(int x, int y);
bool points2(int x, int y);
bool points3(int x, int y);
bool points4(int x, int y);
bool points5(int x, int y);



int runScroller(void);

int main(void)
{
	//Initial Variables
	const int WIDTH = 720;
	const int HEIGHT = 850;
	const int NUM_snowballs = 5;
	const int NUM_penguins = 10;
	enum KEYS { LEFT, RIGHT, SPACE };
	bool keys[3] = { false, false, false };

	bool done = false;
	bool redraw = true;
	const int FPS = 60;

	//Initialization Functions
	if (!al_init())
		return -1;

	//Allegro variables
	ALLEGRO_DISPLAY* display = NULL;
	ALLEGRO_EVENT_QUEUE* event_queue = NULL;
	ALLEGRO_TIMER* timer = NULL;
	ALLEGRO_FONT* font = NULL;
	ALLEGRO_BITMAP* background = NULL;

	//Allegro Initilization
	al_install_keyboard();
	al_init_image_addon();
	al_init_font_addon();
	al_init_ttf_addon();
	al_init_acodec_addon();
	al_install_audio();

	// Background/display/font
	display = al_create_display(WIDTH, HEIGHT);
	background = al_load_bitmap("background.png");
	font = al_load_ttf_font("arial.ttf", 32, 0);

	if (!display) {
		return 'hi';
	}

	if (!font) {
		cout << "Could not load 'arial.ttf'";
		return -1;
	}

	
	if (!al_install_mouse()) {
		cout << "could not load mouse";
		return -1;
	}

	if (!background) {
		cout << "could not load background!";
		return -1;
	}
	//Object variables
	Iceberg myIceberg(HEIGHT, WIDTH);
	Cannon myCannon(HEIGHT, WIDTH);
	Snowball mySnowball[NUM_snowballs];
	Penguin myPenguins[NUM_penguins];


	event_queue = al_create_event_queue();
	timer = al_create_timer(1.0 / FPS);

	srand(time(NULL));

	al_register_event_source(event_queue, al_get_keyboard_event_source());
	al_register_event_source(event_queue, al_get_timer_event_source(timer));
	al_register_event_source(event_queue, al_get_display_event_source(display));
	al_start_timer(timer);
	// Intro
	al_draw_textf(font, al_map_rgb(255, 255, 255), WIDTH / 2, HEIGHT / 2, ALLEGRO_ALIGN_CENTER, "Left and Right Arrow Keys to Aim, Space to shoot.");
	al_draw_textf(font, al_map_rgb(255, 255, 255), WIDTH / 2, HEIGHT / 2 + 40, ALLEGRO_ALIGN_CENTER, "Eliminate 25 Penguins Before Out of Life");
	al_flip_display();
	al_rest(5.0);

	while (!done)
	{
		// Score and Remaining Health
		al_draw_textf(font, al_map_rgb(255, 255, 255), 10, 10, ALLEGRO_ALIGN_LEFT, "Health: %d   Score: %d", myIceberg.getLives(), myIceberg.getScore());


		ALLEGRO_EVENT ev;
		al_wait_for_event(event_queue, &ev);

		//Main Logic Usage
		if (ev.type == ALLEGRO_EVENT_TIMER)
		{
			redraw = true;
			// Moves left or right based on keys
			if (keys[LEFT])
				myCannon.MoveLeft();
			if (keys[RIGHT])
				myCannon.MoveRight();
			// Spawns penguins, moves penguins down screen, checks for collides etc.
			for (int i = 0; i < NUM_snowballs; i++)
				mySnowball[i].UpdateSnowball(WIDTH);
			for (int i = 0; i < NUM_penguins; i++)
				myPenguins[i].StartPenguin(WIDTH, HEIGHT);
			for (int i = 0; i < NUM_penguins; i++)
				myPenguins[i].UpdatePenguin();
			for (int i = 0; i < NUM_snowballs; i++)
				mySnowball[i].CollideSnowball(myPenguins, NUM_penguins, myIceberg);
			for (int i = 0; i < NUM_penguins; i++)
				myPenguins[i].CollidePenguin(myIceberg);

		}
		else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
		{
			done = true;
		}
		else if (ev.type == ALLEGRO_EVENT_KEY_DOWN)
		{
			switch (ev.keyboard.keycode)
			{
			case ALLEGRO_KEY_ESCAPE:
				done = true;
				break;
			case ALLEGRO_KEY_LEFT:
				keys[LEFT] = true;
				break;
			case ALLEGRO_KEY_RIGHT:
				keys[RIGHT] = true;
				break;
			case ALLEGRO_KEY_SPACE:
				keys[SPACE] = true;
				// Fires snowball if space is pressed
				for (int i = 0; i < NUM_snowballs; i++)
					mySnowball[i].FireSnowball(myCannon);
				break;
			}
		}
		else if (ev.type == ALLEGRO_EVENT_KEY_UP)
		{
			switch (ev.keyboard.keycode)
			{
			case ALLEGRO_KEY_ESCAPE:
				done = true;
				break;
			case ALLEGRO_KEY_LEFT:
				keys[LEFT] = false;
				break;
			case ALLEGRO_KEY_RIGHT:
				keys[RIGHT] = false;
				break;
			case ALLEGRO_KEY_SPACE:
				keys[SPACE] = false;
				break;
			}
		}

		if (redraw && al_is_event_queue_empty(event_queue))
		{
			redraw = false;
			// Draws cannon/iceberg
			myIceberg.DrawIceberg();
			myCannon.DrawCannon();
			for (int i = 0; i < NUM_snowballs; i++)
				mySnowball[i].DrawSnowball();
			for (int i = 0; i < NUM_penguins; i++)
				myPenguins[i].DrawPenguin();

			al_flip_display();
			al_clear_to_color(al_map_rgb(0, 0, 0));
		}
		// Game over Screen
		if (myIceberg.getScore() >= 25) {
			al_draw_textf(font, al_map_rgb(255, 255, 255), WIDTH / 2, HEIGHT / 2, ALLEGRO_ALIGN_CENTER, "Congratulations! Reached: %d Points", myIceberg.getScore());
			al_flip_display();
			al_rest(5.0);
			al_destroy_bitmap(background);
			al_destroy_font(font);
			al_destroy_event_queue(event_queue);
			al_destroy_timer(timer);
			al_destroy_display(display);
		}
		if (myIceberg.getLives() <= 0) {
			al_destroy_bitmap(background);
			al_destroy_font(font);
			al_destroy_event_queue(event_queue);
			al_destroy_timer(timer);
			al_destroy_display(display);
			runScroller();
		}
		al_draw_bitmap(background, 0, 0, 0);
		
	}
	
	return 0;
}


int runScroller(void)
{
	const int WIDTH = 900;
	const int HEIGHT = 480;
	bool keys[] = { false, false, false, false, false };
	enum KEYS { UP, DOWN, LEFT, RIGHT, SPACE };
	//variables
	bool done = false;
	bool render = false;
	//Player Variable
	Sprite player;
	const int JUMPIT = 1600;
	int jump = JUMPIT;
	int remainingTime = 90;
	int points = 0;
	bool one = false;
	bool two = false;
	bool three = false;
	bool four = false;
	bool five = false;




	//allegro variable
	ALLEGRO_DISPLAY* display = NULL;
	ALLEGRO_EVENT_QUEUE* event_queue = NULL;
	ALLEGRO_TIMER* timer;
	ALLEGRO_TIMER* timerFinal; // Timer

	ALLEGRO_FONT* font = NULL;

	//program init
	if (!al_init())										//initialize Allegro
		return -1;

	display = al_create_display(WIDTH, HEIGHT);			//create our display object
	timerFinal = al_create_timer(1.0);

	if (!display)										//test display object
		return -1;

	//addon init
	al_install_keyboard();
	al_init_image_addon();
	al_init_primitives_addon();
	al_init_font_addon();
	al_init_ttf_addon();
	font = al_load_ttf_font("aovel.ttf", 32, 0);

	player.InitSprites(WIDTH, HEIGHT);

	int xOff = 0;
	int yOff = 0;
	if (MapLoad("level1.fmp", 1))
		return -5;

	event_queue = al_create_event_queue();
	timer = al_create_timer(1.0 / 60);

	al_register_event_source(event_queue, al_get_timer_event_source(timer));
	al_register_event_source(event_queue, al_get_keyboard_event_source());
	al_register_event_source(event_queue, al_get_timer_event_source(timerFinal));

	al_start_timer(timerFinal);
	al_start_timer(timer);
	//draw the background tiles
	MapDrawBG(xOff, yOff, 0, 0, WIDTH - 1, HEIGHT - 1);

	//draw foreground tiles
	MapDrawFG(xOff, yOff, 0, 0, WIDTH - 1, HEIGHT - 1, 0);
	player.DrawSprites(0, 0);
	al_flip_display();
	al_clear_to_color(al_map_rgb(0, 0, 0));
	// Timer 
	//double start_time = al_get_time();
	int level = 1;

	// Intro
	al_draw_textf(font, al_map_rgb(255, 255, 255), WIDTH / 2, HEIGHT / 2, ALLEGRO_ALIGN_CENTER, "Collect all the Icecubes (From bottom or Top) Arrow Keys to move");
	al_flip_display();
	al_rest(10.0);
	while (!done)
	{
		ALLEGRO_EVENT ev;
		al_wait_for_event(event_queue, &ev);
		if (ev.type == ALLEGRO_EVENT_TIMER)
		{
			render = true;
			if (keys[UP]) {  // Up Direction 3
				player.UpdateSprites(WIDTH, HEIGHT, 3);
			}
			else if (keys[DOWN]) { // Down Direction 0
				player.UpdateSprites(WIDTH, HEIGHT, 0);
			}
			else if (keys[LEFT]) { // Left direction1
				player.UpdateSprites(WIDTH, HEIGHT, 1);
			}

			else if (keys[RIGHT]) { // Right Direction 2
				player.UpdateSprites(WIDTH, HEIGHT, 2);
			}
			else {
				player.UpdateSprites(WIDTH, HEIGHT, 5); // Other directions
			}
			if (ev.timer.source == timerFinal) { // Timer to see if the game should end, runs out game ends
				remainingTime--;
				if (remainingTime <= 0) {
					al_draw_textf(font, al_map_rgb(255, 255, 255), WIDTH / 2, HEIGHT / 2, ALLEGRO_ALIGN_CENTER, "Out of Time!");
					al_draw_textf(font, al_map_rgb(255, 255, 255), WIDTH / 2, HEIGHT / 2 + 40, ALLEGRO_ALIGN_CENTER, "Game Over!");
					al_flip_display();
					al_rest(10.0);
					done = true;
				}
			}
			//if (player.endLevel()) { // When hits end of level swaps to the next one, similar to the kirby files/alien ones using sprintf
			//	level++;
			//	char file[20];
			//	player.DrawSprites(0, 0);
			//	sprintf(file, "level%i.FMP", level);
			//	MapLoad(file, 1);
			//	player.setX(80);
			//	player.setY(-10);
			//	remainingTime = 60;
			//}
			if (player.point1()) {
				one = true;
				std::cout << "one" << std::endl;
				//std::cout << one << std::endl;
				
			}
			if (player.point2()) {
				two = true;

				std::cout << "two" << std::endl;
				//std::cout << two << std::endl;
			}
			if (player.point3()) {
				three = true;
				std::cout << "three" << std::endl;
				//std::cout << three << std::endl;

			}
			if (player.point4()) {
				four = true;
				std::cout << "four" << std::endl;
				//std::cout << four << std::endl;
			}
			if (player.point5()) {
				five = true;

				std::cout << "five" << std::endl;
				//std::cout << five << std::endl;
			}


			// Ends game, Game over screen and timer dispaly
			if (player.endGame()) {
				//std::cout << one << std::endl;
				//std::cout << two << std::endl;
				//std::cout << three << std::endl;
				//std::cout << four << std::endl;
				//std::cout << five << std::endl;


				if(one && two && three && four && five){
					al_draw_textf(font, al_map_rgb(255, 255, 255), WIDTH / 2, HEIGHT / 2, ALLEGRO_ALIGN_CENTER, "Congrats You Gathered All The IceCubes!");
					al_flip_display();
					al_rest(10.0);
					break;
				}
				else {
					al_draw_textf(font, al_map_rgb(255, 255, 255), WIDTH / 2, HEIGHT / 2, ALLEGRO_ALIGN_CENTER, "Did Not Get All IceCubes Game Over!");
					al_flip_display();
					al_rest(10.0);
					break;
				}
			}
			render = true;

		}
		else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
		{
			done = true;
		}
		else if (ev.type == ALLEGRO_EVENT_KEY_DOWN)
		{
			switch (ev.keyboard.keycode)
			{
			case ALLEGRO_KEY_ESCAPE:
				done = true;
				break;
			case ALLEGRO_KEY_UP:
				keys[UP] = true;
				break;
			case ALLEGRO_KEY_DOWN:
				keys[DOWN] = true;
				break;
			case ALLEGRO_KEY_LEFT:
				keys[LEFT] = true;
				break;
			case ALLEGRO_KEY_RIGHT:
				keys[RIGHT] = true;
				break;
			case ALLEGRO_KEY_SPACE:
				keys[SPACE] = true;
				


			}
		}
		else if (ev.type == ALLEGRO_EVENT_KEY_UP)
		{
			switch (ev.keyboard.keycode)
			{
			case ALLEGRO_KEY_ESCAPE:
				done = true;
				break;
			case ALLEGRO_KEY_UP:
				keys[UP] = false;
				break;
			case ALLEGRO_KEY_DOWN:
				keys[DOWN] = false;
				break;
			case ALLEGRO_KEY_LEFT:
				keys[LEFT] = false;
				break;
			case ALLEGRO_KEY_RIGHT:
				keys[RIGHT] = false;
				break;
			case ALLEGRO_KEY_SPACE:
				keys[SPACE] = false;
				break;
			}
		}
		if (render && al_is_event_queue_empty(event_queue))
		{
			render = false;
			MapUpdateAnims();

			//update the map scroll position
			xOff = player.getX() + player.getWidth() - WIDTH / 2;
			yOff = player.getY() + player.getHeight() - HEIGHT / 2;

			//avoid moving beyond the map edge
			if (xOff < 0) xOff = 0;

			if (xOff > (mapwidth * mapblockwidth - WIDTH))
				xOff = mapwidth * mapblockwidth - WIDTH;
			if (yOff < 0)
				yOff = 0;
			if (yOff > (mapheight * mapblockheight - HEIGHT))
				yOff = mapheight * mapblockheight - HEIGHT;

			//draw the background tiles
			MapDrawBG(xOff, yOff, 0, 0, WIDTH, HEIGHT);

			//draw foreground tiles
			MapDrawFG(xOff, yOff, 0, 0, WIDTH, HEIGHT, 0);
			player.DrawSprites(xOff, yOff);
			// Print remaining time to screen
			char timeLeft[30];
			sprintf(timeLeft, "Remaining time: %i", remainingTime);
			al_draw_textf(font, al_map_rgb(255, 255, 255), 5, 5, 0, timeLeft);
			
			al_flip_display();
			al_clear_to_color(al_map_rgb(0, 0, 0));
		}
	}
	MapFreeMem();
	al_destroy_font(font);
	al_destroy_event_queue(event_queue);
	al_destroy_display(display);						//destroy our display object
	al_destroy_timer(timerFinal);
	return 0;
}


// collision with tiles
int collided(int x, int y)
{
	BLKSTR* blockdata;
	blockdata = MapGetBlock(x / mapblockwidth, y / mapblockheight);
	return blockdata->tl;
}

bool endValue(int x, int y) // end game
{

	BLKSTR* data;
	data = MapGetBlock(x / mapblockwidth, y / mapblockheight);

	if (data->user1 == 5)
	{
		return true;
	}
	else
		return false;
}
bool endLevelValue(int x, int y) // end level
{

	BLKSTR* data;
	data = MapGetBlock(x / mapblockwidth, y / mapblockheight);

	if (data->user1 == 7)
	{
		return true;
	}
	else
		return false;
}
bool points1(int x, int y) // Points
{

	BLKSTR* data;
	data = MapGetBlock(x / mapblockwidth, y / mapblockheight);

	if (data->user1 == 9)
	{
		return true;
	}
	else
		return false;
}
bool points2(int x, int y) // Points
{

	BLKSTR* data;
	data = MapGetBlock(x / mapblockwidth, y / mapblockheight);

	if (data->user1 == 10)
	{
		return true;
	}
	else
		return false;
}

bool points3(int x, int y) // Points
{

	BLKSTR* data;
	data = MapGetBlock(x / mapblockwidth, y / mapblockheight);

	if (data->user1 == 11)
	{
		return true;
	}
	else
		return false;
}

bool points4(int x, int y) // Points
{

	BLKSTR* data;
	data = MapGetBlock(x / mapblockwidth, y / mapblockheight);

	if (data->user1 == 12)
	{
		return true;
	}
	else
		return false;
}

bool points5(int x, int y) // Points
{

	BLKSTR* data;
	data = MapGetBlock(x / mapblockwidth, y / mapblockheight);

	if (data->user1 == 13)
	{
		return true;
	}
	else
		return false;
}


