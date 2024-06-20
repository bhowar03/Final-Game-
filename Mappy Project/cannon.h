// Brandon Howar Project 5
#ifndef CANNONH
#define CANNONH

#include <allegro5\allegro.h>

class Cannon
{
public:
    Cannon(int HEIGHT, int WIDTH);
    ~Cannon();
    void MoveLeft();
    void MoveRight();
    void DrawCannon();
    // getter for the angle
    float getAngle() { return angle; }
    // method to update angle
    void updateAngle(float angleChange);
    int getX() { return x; }
    int getY() { return y; }
private:
    int x;
    int y;
    // store current angle of cannon
    float angle;
    ALLEGRO_BITMAP* cannon;
    ALLEGRO_BITMAP* captain;
};
#endif