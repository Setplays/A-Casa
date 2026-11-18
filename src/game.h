#ifndef GAME_H
#define GAME_H
/* Logica e mapa independentes do OpenGL: reutilizaveis pela IA e pelo QTE. */
#define MAX_BOXES 2048
#define MAX_HOUSES 4
#define MAP_EDGE 37.0f
#define PLAYER_RADIUS 0.30f
#define EYE_HEIGHT 1.7f
#define MAGAZINE_SIZE 12
#define RELOAD_SECONDS 1.6f
#define PI 3.14159265358979323846f
typedef struct { float x,y,z,sx,sy,sz,r,g,b; int solid; } Box;
typedef struct { float x,z,w,d,h; int main_house,open; float door; } House;
typedef struct { float x,z,scale; } Tree;
typedef struct {
    Box boxes[MAX_BOXES]; int box_count;
    House houses[MAX_HOUSES]; int house_count;
    Tree trees[200]; int tree_count;
} World;
typedef struct {
    float x,y,z,yaw,pitch,vy,bob;
    int grounded,magazine,reserve,reloading;
    float reload_left,shot_left,flash_left,chimney;
    int ammo_taken[4];
} Game;
void world_init(World *w);
void game_init(Game *g);
int world_blocked(const World *w,float x,float z,float radius);
int door_blocked(const House *h,float progress,float x,float z,float radius);
int game_shoot(Game *g);
int game_reload(Game *g);
int game_near_door(const Game *g,const World *w);
void game_interact(Game *g,World *w);
void game_update(Game *g,World *w,float dt,float forward,float side,int jump);
#endif
