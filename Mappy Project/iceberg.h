// Brandon Howar Project 3
#include <allegro5\allegro.h>
#ifndef ICEBERGH
#define ICEBERGH

class Iceberg
{
public:
    Iceberg(int HEIGHT, int WIDTH);
    ~Iceberg();
    void DrawIceberg();
    int getBoundX() { return boundx; }
    int getBoundY() { return boundy; }
    int getX() { return x; }
    int getY() { return y; }
    int getLives() const { return lives; }
    int getScore() const { return score; }
    void removeLife() { lives--; }
    void ChangeImage() { icebergIndex = (icebergIndex + 1) % 5; }
    void Score() { score++; }
private:
    int x;
    int y;
    int lives;
    int boundx;
    int boundy;
    int score;
    int icebergIndex;
    ALLEGRO_BITMAP* images[5];
    ALLEGRO_BITMAP* status[5];
};
#endif