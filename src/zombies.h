#ifndef ZOMBIES_H
#define ZOMBIES_H
#include "game.h"
#define MAX_ZOMBIES 32
#define MAX_DIRT 384
#define NAV_SIZE 149
#define NAV_CELLS (NAV_SIZE*NAV_SIZE)
#define NAV_STEP .5f
#define EMERGE_SECONDS 3.0f
#define FALL_SECONDS .65f
typedef enum { Z_UNUSED, Z_RISING, Z_WALKING, Z_FALLING, Z_CRAWLING, Z_STUNNED, Z_DEAD } ZombieState;
typedef enum { HIT_NONE, HIT_HEAD, HIT_BODY, HIT_LEG } HitRegion;
typedef struct {
    float x,z,yaw,age,phase,attack_left,think_left,dir_x,dir_z,flash;
    float death_lean,death_y;
    float grab_pose,bite_pose,knock_pose;
    int body_hits,crippled;
    ZombieState state;
} Zombie;
typedef struct { float x,y,z,vx,vy,vz,life,size; } Dirt;
/* Mesmas pecas e transformacoes para desenho e raycast (sem caixas invisiveis maiores). */
typedef struct { float x,y,z,sx,sy,sz; HitRegion region; int skin; } ZombiePart;
typedef struct { float y,lean; } ZombiePose;
typedef enum { GRAB_NONE, GRAB_ENTER, GRAB_MASH, GRAB_PUNCH, GRAB_PUSH,
               GRAB_BITE, GRAB_FALL, GRAB_DEAD } GrabState;
typedef struct {
    GrabState state;
    int owner,taps,key_held;
    float age,cooldown,pulse,blood;
    float start_yaw,start_pitch,target_yaw,target_pitch,away_x,away_z;
} Grapple;
typedef struct {
    Grapple grab;
    Zombie zombies[MAX_ZOMBIES]; Dirt dirt[MAX_DIRT]; int dirt_cursor;
    unsigned random_state;
    int spawning,kills,health,last_headshot;
    float spawn_left,nav_left,feedback,hurt;
    unsigned char blocked[NAV_CELLS],edges[NAV_CELLS];
    int distance[NAV_CELLS],queue[NAV_CELLS],nav_valid;
    float nav_doors[MAX_HOUSES];
} Enemies;
void enemies_init(Enemies *e,unsigned seed);
void enemies_toggle(Enemies *e);
int enemies_alive(const Enemies *e);
int enemies_spawn_at(Enemies *e,const World *w,const Game *g,float x,float z);
void enemies_rebuild_nav(Enemies *e,const World *w,const Game *g);
void enemies_update(Enemies *e,const World *w,const Game *g,float dt);
HitRegion enemies_fire(Enemies *e,Game *g,const World *w);
int zombie_damage(Enemies *e,int index,HitRegion region);
ZombiePose zombie_pose(const Zombie *z);
int zombie_parts(const Zombie *z,ZombiePart out[10]);
float zombie_ray(const Zombie *z,const float origin[3],const float direction[3],HitRegion *region);
float world_ray(const World *w,const float origin[3],const float direction[3],float maximum);
#endif
