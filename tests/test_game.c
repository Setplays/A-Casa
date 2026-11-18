#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static World world;
static void advance(Game *g,float seconds) {
    int n=(int)ceilf(seconds*120);
    for(int i=0;i<n;i++) game_update(g,&world,1.0f/120,0,0,0);
}
int main(void) {
    Game g; world_init(&world); game_init(&g);
    assert(world.house_count==4 && world.tree_count>50);
    assert(!world_blocked(&world,g.x,g.z,PLAYER_RADIUS));
    assert(!game_reload(&g));
    assert(game_shoot(&g) && g.magazine==11);
    assert(!game_shoot(&g));
    advance(&g,.3f); assert(game_reload(&g));
    assert(!game_shoot(&g) && !game_reload(&g));
    advance(&g,1); assert(g.magazine==11 && g.reserve==48);
    advance(&g,.7f); assert(g.magazine==12 && g.reserve==47 && !g.reloading);
    g.magazine=4; g.reserve=3; assert(game_reload(&g)); advance(&g,1.7f);
    assert(g.magazine==7 && g.reserve==0 && !game_reload(&g));
    g.magazine=0; assert(!game_shoot(&g));
    assert(world_blocked(&world,3,0,PLAYER_RADIUS));
    assert(world_blocked(&world,-7,1,PLAYER_RADIUS));
    assert(world_blocked(&world,37,0,PLAYER_RADIUS));
    /* Todas as portas: fechada bloqueia, aberta deixa passagem central. */
    for(int i=0;i<4;i++) {
        House *h=&world.houses[i]; float z=h->z+h->d/2+.11f;
        assert(world_blocked(&world,h->x,z,PLAYER_RADIUS)); h->door=1;
        assert(!world_blocked(&world,h->x,z,PLAYER_RADIUS));
        assert(door_blocked(h,1,h->x-.7f,z-.7f,PLAYER_RADIUS));
        /* Trajeto de entrada pelo vao ate o centro do interior. */
        game_init(&g); g.x=h->x; g.z=z+1.1f;
        h->open=1;
        for(int j=0;j<55;j++) game_update(&g,&world,1.0f/120,1,0,0);
        assert(g.z<z-.7f);
    }
    /* A porta nao fecha atraves do jogador. */
    game_init(&g); House *h=&world.houses[0]; h->door=1; h->open=0;
    g.x=0; g.z=3.11f; advance(&g,2); assert(h->door>.1f);
    /* Velocidade diagonal normalizada. */
    Game a,b; game_init(&a); game_init(&b); a.x=b.x=-3; a.z=b.z=15;
    for(int i=0;i<60;i++) {
        game_update(&a,&world,1.0f/120,1,0,0); game_update(&b,&world,1.0f/120,1,1,0);
    }
    float da=hypotf(a.x+3,a.z-15),db=hypotf(b.x+3,b.z-15); assert(fabsf(da-db)<.002f);
    game_init(&g); game_update(&g,&world,1.0f/120,0,0,1);
    assert(g.y>EYE_HEIGHT && !g.grounded); advance(&g,2); assert(g.grounded);
    /* Coleta unica e conservacao da reserva. */
    game_init(&g); g.x=0; g.z=-1.1f; advance(&g,.1f);
    assert(g.reserve==72 && g.ammo_taken[0]); advance(&g,1); assert(g.reserve==72);
    puts("OK: municao, cadencia, recarga parcial, portas, 4 interiores, jardim, limites, diagonal, pulo e coleta unica.");
    return 0;
}
