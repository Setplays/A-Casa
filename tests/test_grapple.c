#include "grapple.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static Enemies e; static World w; static Game g;
static void reset(void) {
    w=(World){0}; game_init(&g); g.z=0; enemies_init(&e,17);
    e.zombies[0]=(Zombie){0}; e.zombies[0].state=Z_WALKING; e.zombies[0].z=-1;
}
static void advance(float time) {
    for(int i=0;i<(int)ceilf(time*120);i++) {
        enemies_update(&e,&w,&g,1.0f/120); grapple_update(&e,&w,&g,1.0f/120);
    }
}
static void start(void) {
    assert(grapple_try_start(&e,&w,&g,0)); advance(.5f); assert(e.grab.state==GRAB_MASH);
}
static void tap(void) { grapple_input(&e,1); grapple_input(&e,0); }
int main(void) {
    reset(); e.zombies[0].z=-4; assert(!grapple_try_start(&e,&w,&g,0));
    reset(); g.grounded=0; assert(!grapple_try_start(&e,&w,&g,0));
    reset(); w.box_count=1; w.boxes[0]=(Box){0,1,-.5f,3,2,.1f,0,0,0,1};
    assert(!grapple_try_start(&e,&w,&g,0)); /* sem agarrar atraves da parede */
    reset(); start(); assert(e.health==100 && grapple_busy(&e));
    for(int i=0;i<50;i++) grapple_input(&e,1);
    assert(e.grab.taps==1); grapple_input(&e,0);
    for(int i=0;i<5;i++) tap();
    assert(e.grab.taps==6);
    assert(enemies_fire(&e,&g,&w)==HIT_NONE && g.magazine==12);
    e.zombies[1]=e.zombies[0]; e.zombies[1].x=.5f;
    assert(!grapple_try_start(&e,&w,&g,1) && e.grab.owner==0);
    e.zombies[1].state=Z_UNUSED;
    for(int i=0;i<6;i++) tap();
    assert(e.grab.state==GRAB_PUNCH);
    advance(.38f); assert(e.grab.state==GRAB_PUSH);
    advance(.7f); assert(!grapple_busy(&e) && e.health==100);
    assert(e.zombies[0].z<-2.9f && e.zombies[0].state==Z_STUNNED);
    assert(!world_blocked(&w,e.zombies[0].x,e.zombies[0].z,.34f));
    assert(!grapple_try_start(&e,&w,&g,0)); /* protecao depois de escapar */
    /* Falha percorre mordida e queda antes de liberar a tela de derrota. */
    reset(); start(); advance(4.0f); assert(e.grab.state==GRAB_BITE);
    advance(.45f); assert(e.health==0 && e.grab.blood>0 && grapple_animating(&e));
    advance(.4f); assert(e.grab.state==GRAB_FALL);
    PlayerView mid=grapple_view(&e,&g); assert(mid.y<EYE_HEIGHT && mid.roll>0);
    advance(1.4f); assert(e.grab.state==GRAB_DEAD && !grapple_animating(&e));
    PlayerView floor=grapple_view(&e,&g);
    assert(fabsf(floor.y-.24f)<.001f && floor.roll>60 && e.grab.blood>=.99f);
    /* Empurrao encontra uma parede e nao atravessa o collider. */
    reset(); w.box_count=1; w.boxes[0]=(Box){0,1,-1.9f,10,2,.2f,0,0,0,1}; start();
    for(int i=0;i<QTE_TAPS;i++) tap();
    advance(1.1f);
    assert(!grapple_busy(&e) && !world_blocked(&w,e.zombies[0].x,e.zombies[0].z,.34f));
    assert(e.zombies[0].z>-1.47f);
    /* Rastejante ergue o tronco durante a luta e volta a rastejar apos ser empurrado. */
    reset(); e.zombies[0].state=Z_CRAWLING; e.zombies[0].crippled=1; start();
    ZombiePose pose=zombie_pose(&e.zombies[0]); assert(pose.lean<40 && pose.y>.8f);
    for(int i=0;i<QTE_TAPS;i++) tap();
    advance(2.5f);
    assert(e.zombies[0].state==Z_CRAWLING && e.zombies[0].crippled);
    /* Cancelamento seguro se o captor for removido por outra regra do jogo. */
    reset(); start(); zombie_damage(&e,0,HIT_HEAD); advance(.05f);
    assert(!grapple_busy(&e) && e.grab.owner==-1 && e.health==100);
    /* Reiniciar limpa progresso, captura e sangue. */
    enemies_init(&e,1); assert(e.grab.state==GRAB_NONE && e.grab.blood==0 && e.grab.owner==-1);
    puts("OK: QTE 12 toques, tecla segurada, agarramento unico/LOS, bloqueio de tiro, sucesso/soco/empurrao, parede, falha/mordida/queda/sangue, rastejante e reset.");
    return 0;
}
