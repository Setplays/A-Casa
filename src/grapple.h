#ifndef GRAPPLE_H
#define GRAPPLE_H
#include "zombies.h"
#define QTE_TAPS 12
#define QTE_SECONDS 4.0f
#define GRAB_ENTER_SECONDS .45f
#define GRAB_PUNCH_SECONDS .35f
#define GRAB_PUSH_SECONDS .65f
#define GRAB_BITE_SECONDS .75f
#define GRAB_FALL_SECONDS 1.25f
typedef struct { float x,y,z,yaw,pitch,roll; } PlayerView;
int grapple_busy(const Enemies *e);
int grapple_animating(const Enemies *e);
int grapple_try_start(Enemies *e,const World *w,const Game *g,int index);
void grapple_input(Enemies *e,int down);
void grapple_update(Enemies *e,const World *w,Game *g,float dt);
PlayerView grapple_view(const Enemies *e,const Game *g);
#endif
