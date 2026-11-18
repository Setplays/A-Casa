#include "zombies.h"
#include "grapple.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static Enemies e;
static World w;
static Game g;
static void reset(void) { w=(World){0}; game_init(&g); g.x=0; g.z=6; enemies_init(&e,1234); }
static Zombie *target(void) {
    Zombie *z=&e.zombies[0]; *z=(Zombie){0}; z->state=Z_WALKING; return z;
}
static void advance(float seconds) {
    for(int i=0;i<(int)ceilf(seconds*120);i++) { enemies_update(&e,&w,&g,1.0f/120); grapple_update(&e,&w,&g,1.0f/120); }
}
static void aim(float x,float y,float z) {
    float dx=x-g.x,dy=y-g.y,dz=z-g.z;
    g.yaw=atan2f(dz,dx)*180/PI; g.pitch=atan2f(dy,hypotf(dx,dz))*180/PI;
    g.shot_left=0;
}
static void test_damage(void) {
    reset(); Zombie *z=target();
    assert(zombie_damage(&e,0,HIT_HEAD)); assert(z->state==Z_DEAD && e.kills==1);
    assert(!zombie_damage(&e,0,HIT_HEAD)); assert(e.kills==1);
    reset(); z=target();
    assert(!zombie_damage(&e,0,HIT_BODY)); assert(!zombie_damage(&e,0,HIT_BODY));
    assert(z->state==Z_WALKING && z->body_hits==2);
    assert(zombie_damage(&e,0,HIT_BODY)); assert(e.kills==1);
    reset(); z=target();
    assert(!zombie_damage(&e,0,HIT_LEG)); assert(z->state==Z_FALLING && z->body_hits==1);
    advance(.7f); assert(z->state==Z_CRAWLING);
    assert(!zombie_damage(&e,0,HIT_BODY)); assert(zombie_damage(&e,0,HIT_LEG));
    assert(z->state==Z_DEAD && z->body_hits==3);
    reset(); z=target(); zombie_damage(&e,0,HIT_LEG); advance(.7f);
    assert(zombie_damage(&e,0,HIT_HEAD)); assert(z->state==Z_DEAD);
    reset(); z=target(); z->state=Z_RISING; z->age=2.8f;
    zombie_damage(&e,0,HIT_LEG); advance(1); assert(z->state==Z_CRAWLING);
}
static void test_raycast(void) {
    reset(); Zombie *z=target(); g.y=1.78f; aim(0,1.78f,0);
    assert(enemies_fire(&e,&g,&w)==HIT_HEAD); assert(z->state==Z_DEAD && g.magazine==11);
    reset(); z=target(); g.y=1.3f; aim(0,1.3f,0);
    assert(enemies_fire(&e,&g,&w)==HIT_BODY); assert(z->body_hits==1);
    assert(enemies_fire(&e,&g,&w)==HIT_NONE && z->body_hits==1); /* cadencia */
    g.shot_left=0; g.reloading=1;
    assert(enemies_fire(&e,&g,&w)==HIT_NONE && g.magazine==11);
    reset(); z=target(); g.x=.16f; g.y=.42f; aim(.16f,.42f,0);
    assert(enemies_fire(&e,&g,&w)==HIT_LEG && z->state==Z_FALLING);
    reset(); z=target(); z->state=Z_CRAWLING;
    float origin[]={0,1.78f,6},dir[]={0,0,-1}; HitRegion part;
    assert(zombie_ray(z,origin,dir,&part)==FLT_MAX); /* antiga cabeca nao existe */
    for(int j=0;j<3;j++) {
        z->yaw=j*45.0f;
        float a=z->yaw*PI/180;
        float hx=sinf(a)*.78f,hz=cosf(a)*.78f,hy=.36f;
        float o[]={hx+sinf(a)*4,hy,hz+cosf(a)*4};
        float d[]={-sinf(a),0,-cosf(a)};
        assert(zombie_ray(z,o,d,&part)<FLT_MAX && part==HIT_HEAD);
    }
    /* Um tiro atinge somente o primeiro alvo. */
    reset(); z=target(); e.zombies[1]=*z; e.zombies[1].z=-3;
    g.y=1.3f; aim(0,1.3f,0); assert(enemies_fire(&e,&g,&w)==HIT_BODY);
    assert(z->body_hits==1 && e.zombies[1].body_hits==0);
    /* Bloqueio por parede e por porta, inclusive com folha aberta. */
    reset(); z=target(); w.box_count=1; w.boxes[0]=(Box){0,1,3,2,2,.2f,0,0,0,1};
    g.y=1.3f; aim(0,1.3f,0); assert(enemies_fire(&e,&g,&w)==HIT_NONE && z->body_hits==0);
    reset(); z=target(); w.house_count=1; w.houses[0]=(House){0,0,6,6,3,1,0,0};
    g.y=1.3f; aim(0,1.3f,0); assert(enemies_fire(&e,&g,&w)==HIT_NONE);
    w.houses[0].door=1; g.shot_left=0;
    assert(enemies_fire(&e,&g,&w)==HIT_BODY);
    float ro[]={-.7f,1.2f,6},rd[]={0,0,-1};
    assert(world_ray(&w,ro,rd,100)<3); /* folha aberta continua bloqueando */
    reset(); z=target(); z->state=Z_RISING; z->age=0;
    g.y=1.7f; aim(0,-1,0); assert(enemies_fire(&e,&g,&w)==HIT_NONE); /* solo */
}
static void test_spawn_and_ai(void) {
    reset(); enemies_rebuild_nav(&e,&w,&g);
    assert(!e.spawning && enemies_alive(&e)==0);
    advance(1); assert(enemies_alive(&e)==0);
    assert(!enemies_spawn_at(&e,&w,&g,0,7)); /* distancia minima */
    assert(enemies_spawn_at(&e,&w,&g,0,-4));
    assert(e.zombies[0].state==Z_RISING);
    advance(1.5f); assert(e.zombies[0].state==Z_RISING && g.x==0 && e.health==100);
    int dirt=0; for(int i=0;i<MAX_DIRT;i++) dirt+=e.dirt[i].life>0; assert(dirt>0);
    advance(1.6f); assert(e.zombies[0].state==Z_WALKING);
    reset(); enemies_toggle(&e); advance(.5f); assert(enemies_alive(&e)==1);
    enemies_toggle(&e); advance(4); assert(enemies_alive(&e)==1); /* OFF para novos apenas */
    reset(); g.z=4; Zombie *z=target(); z->z=-4;
    w.box_count=1; w.boxes[0]=(Box){0,1,0,4,2,.25f,0,0,0,1};
    for(int i=0;i<120*12;i++) {
        enemies_update(&e,&w,&g,1.0f/120); grapple_update(&e,&w,&g,1.0f/120);
        assert(!world_blocked(&w,z->x,z->z,.28f));
    }
    assert(e.health<100); /* contornou o obstaculo e alcancou jogador */
    reset(); g.z=4; z=target(); z->state=Z_CRAWLING; z->z=-4;
    w.box_count=1; w.boxes[0]=(Box){0,1,0,4,2,.25f,0,0,0,1};
    advance(25); assert(e.health<100); /* rastejando tambem contorna a parede */
    reset(); z=target(); z->z=5.4f;
    w.box_count=1; w.boxes[0]=(Box){0,1,5.7f,4,2,.1f,0,0,0,1};
    advance(.5f); assert(e.health==100); /* nao ataca atraves da parede */
    reset(); z=target(); z->z=5.0f; advance(.1f); assert(e.grab.state==GRAB_ENTER && e.health==100);
    advance(.5f); assert(e.grab.state==GRAB_MASH && e.health==100);
    advance(4.5f); assert(e.health==0);
    reset(); z=target(); zombie_damage(&e,0,HIT_HEAD); advance(6.1f);
    assert(z->state==Z_UNUSED); /* slot reutilizavel */
    /* Com o jogador numa casa fechada, spawn externo sem caminho e rejeitado.
       Abrir a porta invalida o cache e libera o caminho. */
    reset(); world_init(&w); g.z=1.5f; enemies_rebuild_nav(&e,&w,&g);
    assert(!enemies_spawn_at(&e,&w,&g,0,12));
    w.houses[0].door=1; enemies_rebuild_nav(&e,&w,&g);
    assert(enemies_spawn_at(&e,&w,&g,0,12));
    reset(); enemies_toggle(&e);
    for(int i=0;i<120*20;i++) { enemies_update(&e,&w,&g,1.0f/120); assert(enemies_alive(&e)<=MAX_ZOMBIES); }
}
int main(void) {
    test_damage(); test_raycast(); test_spawn_and_ai();
    puts("OK: headshot, 3 acertos, pernas/queda/rastejo, hitboxes por pose, parede/porta/solo, cadencia, spawn, terra, BFS, agarramento e reciclagem.");
    return 0;
}
