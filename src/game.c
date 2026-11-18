#include "game.h"
#include <math.h>
static float clampf(float v,float lo,float hi) { return fmaxf(lo,fminf(v,hi)); }
static int circle_box(float x,float z,float radius,float x0,float x1,float z0,float z1) {
    float dx=x-clampf(x,x0,x1),dz=z-clampf(z,z0,z1);
    return dx*dx+dz*dz < radius*radius;
}
/* Porta articulada: transformar jogador para o espaco local da folha. */
int door_blocked(const House *h,float progress,float x,float z,float radius) {
    float a=progress*PI/2,dx=x-(h->x-.7f),dz=z-(h->z+h->d/2+.11f);
    float lx=cosf(a)*dx-sinf(a)*dz,lz=sinf(a)*dx+cosf(a)*dz;
    return circle_box(lx,lz,radius,0,1.4f,-.06f,.06f);
}
int world_blocked(const World *w,float x,float z,float radius) {
    if(fabsf(x)+radius>MAP_EDGE || fabsf(z)+radius>MAP_EDGE) return 1;
    for(int i=0;i<w->box_count;i++) {
        const Box *b=&w->boxes[i];
        if(b->solid && circle_box(x,z,radius,b->x-b->sx/2,b->x+b->sx/2,
                                  b->z-b->sz/2,b->z+b->sz/2)) return 1;
    }
    for(int i=0;i<w->house_count;i++)
        if(door_blocked(&w->houses[i],w->houses[i].door,x,z,radius)) return 1;
    return 0;
}
void game_init(Game *g) {
    *g=(Game){0}; g->z=8; g->y=EYE_HEIGHT; g->yaw=-90;
    g->grounded=1; g->magazine=MAGAZINE_SIZE; g->reserve=48;
}
int game_shoot(Game *g) {
    if(g->reloading || g->shot_left>0 || g->magazine<=0) return 0;
    --g->magazine; g->shot_left=.23f; g->flash_left=.09f; return 1;
}
int game_reload(Game *g) {
    if(g->reloading || g->magazine==MAGAZINE_SIZE || g->reserve<=0) return 0;
    g->reloading=1; g->reload_left=RELOAD_SECONDS; return 1;
}
int game_near_door(const Game *g,const World *w) {
    int selected=-1; float best=2.4f*2.4f;
    for(int i=0;i<w->house_count;i++) {
        const House *h=&w->houses[i];
        float dx=g->x-h->x,dz=g->z-(h->z+h->d/2),d=dx*dx+dz*dz;
        if(d<best) { best=d; selected=i; }
    }
    return selected;
}
void game_interact(Game *g,World *w) {
    int i=game_near_door(g,w);
    if(i>=0) w->houses[i].open=!w->houses[i].open;
}
void game_update(Game *g,World *w,float dt,float forward,float side,int jump) {
    /* Quem chama usa passos fixos de 1/120s; limitacao protege chamadas externas. */
    dt=clampf(dt,0,.02f);
    g->shot_left=fmaxf(0,g->shot_left-dt); g->flash_left=fmaxf(0,g->flash_left-dt);
    if(g->reloading) {
        g->reload_left-=dt;
        if(g->reload_left<=0) {
            int n=MAGAZINE_SIZE-g->magazine; if(n>g->reserve) n=g->reserve;
            g->magazine+=n; g->reserve-=n; g->reloading=0; g->reload_left=0;
        }
    }
    for(int i=0;i<w->house_count;i++) {
        House *h=&w->houses[i]; float next=clampf(h->door+(h->open?1:-1)*1.6f*dt,0,1);
        /* Bloqueia a animacao quando a folha encontraria o jogador. */
        if(!door_blocked(h,next,g->x,g->z,PLAYER_RADIUS+.02f)) h->door=next;
    }
    float yaw=g->yaw*PI/180,len=sqrtf(forward*forward+side*side);
    if(len>1) { forward/=len; side/=len; }
    float dx=(cosf(yaw)*forward-sinf(yaw)*side)*5.0f*dt;
    float dz=(sinf(yaw)*forward+cosf(yaw)*side)*5.0f*dt;
    /* Eixos independentes permitem deslizar junto as paredes. */
    if(!world_blocked(w,g->x+dx,g->z,PLAYER_RADIUS)) g->x+=dx;
    if(!world_blocked(w,g->x,g->z+dz,PLAYER_RADIUS)) g->z+=dz;
    if(len>0 && g->grounded) g->bob+=9*dt;
    if(jump && g->grounded) { g->vy=5; g->grounded=0; }
    g->vy-=15*dt; g->y+=g->vy*dt;
    /* Teto e verga: salto nao atravessa cobertura nem o topo da porta. */
    for(int i=0;i<w->house_count;i++) {
        House *h=&w->houses[i]; float ceiling=100;
        if(fabsf(g->x-h->x)<h->w/2+.3f && fabsf(g->z-h->z)<h->d/2+.3f) ceiling=2.85f;
        if(fabsf(g->x-h->x)<1.0f && fabsf(g->z-(h->z+h->d/2))<.45f) ceiling=2.3f;
        if(g->y>ceiling) { g->y=ceiling; if(g->vy>0) g->vy=0; }
    }
    if(g->y<=EYE_HEIGHT) { g->y=EYE_HEIGHT; g->vy=0; g->grounded=1; }
    g->chimney=fminf(1,g->chimney+dt*.65f);
    /* Uma caixa de 24 balas em cada casa, coletada por proximidade. */
    for(int i=0;i<w->house_count;i++) {
        House *h=&w->houses[i]; float dx2=g->x-h->x,dz2=g->z-(h->z-1.1f);
        if(!g->ammo_taken[i] && dx2*dx2+dz2*dz2<.75f*.75f) {
            g->ammo_taken[i]=1; g->reserve+=24;
        }
    }
}
