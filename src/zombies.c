#include "zombies.h"
#include "grapple.h"
#include <math.h>
#include <string.h>
#include <float.h>
static float clampf(float v,float a,float b) { return fmaxf(a,fminf(b,v)); }
static float rand01(Enemies *e) {
    e->random_state=1664525u*e->random_state+1013904223u;
    return (e->random_state>>8)/16777216.0f;
}
static int cell(float x,float z) {
    int ix=(int)floorf((x+MAP_EDGE)/NAV_STEP+.5f);
    int iz=(int)floorf((z+MAP_EDGE)/NAV_STEP+.5f);
    if(ix<0||iz<0||ix>=NAV_SIZE||iz>=NAV_SIZE) return -1;
    return iz*NAV_SIZE+ix;
}
static float cell_x(int n) { return (n%NAV_SIZE)*NAV_STEP-MAP_EDGE; }
static float cell_z(int n) { return (n/NAV_SIZE)*NAV_STEP-MAP_EDGE; }
static int neighbor(int n,int d) {
    int x=n%NAV_SIZE,z=n/NAV_SIZE;
    if(d==0) x--;
    if(d==1) x++;
    if(d==2) z--;
    if(d==3) z++;
    if(x<0||z<0||x>=NAV_SIZE||z>=NAV_SIZE) return -1;
    return z*NAV_SIZE+x;
}
void enemies_init(Enemies *e,unsigned seed) {
    memset(e,0,sizeof *e); e->random_state=seed?seed:1; e->health=100; e->grab.owner=-1;
    for(int i=0;i<NAV_CELLS;i++) e->distance[i]=-1;
}
void enemies_toggle(Enemies *e) {
    if(e->health<=0) return;
    e->spawning=!e->spawning; if(e->spawning) e->spawn_left=.4f;
    e->nav_left=0;
}
int enemies_alive(const Enemies *e) {
    int n=0; for(int i=0;i<MAX_ZOMBIES;i++)
        if(e->zombies[i].state!=Z_UNUSED && e->zombies[i].state!=Z_DEAD) n++;
    return n;
}
void enemies_rebuild_nav(Enemies *e,const World *w,const Game *g) {
    /* Campo de distancias compartilhado: um BFS para todos os inimigos.
       Reconsulta portas a cada 0.5 s; arestas tambem precisam estar livres. */
    int dirty=!e->nav_valid;
    for(int i=0;i<w->house_count;i++)
        if(fabsf(e->nav_doors[i]-w->houses[i].door)>.001f) dirty=1;
    if(dirty) {
        for(int i=0;i<NAV_CELLS;i++) {
            e->blocked[i]=(unsigned char)world_blocked(w,cell_x(i),cell_z(i),.34f);
            e->edges[i]=0;
        }
        for(int n=0;n<NAV_CELLS;n++) if(!e->blocked[n]) {
            /* Testa cada aresta uma vez e registra os dois sentidos. */
            for(int d=1;d<=3;d+=2) {
                int v=neighbor(n,d); if(v<0||e->blocked[v]) continue;
                float mx=(cell_x(n)+cell_x(v))*.5f,mz=(cell_z(n)+cell_z(v))*.5f;
                if(!world_blocked(w,mx,mz,.34f)) {
                    e->edges[n]|=(unsigned char)(1u<<d);
                    e->edges[v]|=(unsigned char)(1u<<(d-1));
                }
            }
        }
        for(int i=0;i<w->house_count;i++) e->nav_doors[i]=w->houses[i].door;
        e->nav_valid=1;
    }
    for(int i=0;i<NAV_CELLS;i++) e->distance[i]=-1;
    int start=cell(g->x,g->z);
    if(start<0 || e->blocked[start]) {
        float best=4; start=-1;
        for(int i=0;i<NAV_CELLS;i++) if(!e->blocked[i]) {
            float dx=cell_x(i)-g->x,dz=cell_z(i)-g->z,d=dx*dx+dz*dz;
            if(d<best) { best=d; start=i; }
        }
    }
    if(start<0) return;
    int head=0,tail=0; e->queue[tail++]=start; e->distance[start]=0;
    while(head<tail) {
        int n=e->queue[head++];
        for(int d=0;d<4;d++) {
            int v=neighbor(n,d);
            if(v<0 || !(e->edges[n]&(1u<<d)) || e->distance[v]>=0) continue;
            e->distance[v]=e->distance[n]+1; e->queue[tail++]=v;
        }
    }
}
static int outside_houses(const World *w,float x,float z) {
    for(int i=0;i<w->house_count;i++) {
        const House *h=&w->houses[i];
        if(fabsf(x-h->x)<h->w/2+.8f && fabsf(z-h->z)<h->d/2+.8f) return 0;
    }
    return 1;
}
int enemies_spawn_at(Enemies *e,const World *w,const Game *g,float x,float z) {
    float dx=x-g->x,dz=z-g->z;
    int n=cell(x,z);
    if(dx*dx+dz*dz<8*8 || n<0 || e->distance[n]<0 ||
       world_blocked(w,x,z,.65f) || !outside_houses(w,x,z)) return 0;
    for(int i=0;i<MAX_ZOMBIES;i++) {
        Zombie *a=&e->zombies[i];
        if(a->state==Z_UNUSED||a->state==Z_DEAD) continue;
        dx=a->x-x; dz=a->z-z; if(dx*dx+dz*dz<2.25f) return 0;
    }
    for(int i=0;i<MAX_ZOMBIES;i++) if(e->zombies[i].state==Z_UNUSED) {
        e->zombies[i]=(Zombie){0}; Zombie *a=&e->zombies[i];
        a->x=x; a->z=z; a->yaw=atan2f(g->x-x,g->z-z)*180/PI;
        a->state=Z_RISING; a->phase=rand01(e)*6.28f; return 1;
    }
    return 0;
}
ZombiePose zombie_pose(const Zombie *z) {
    ZombiePose p={1,0};
    if(z->state==Z_RISING) {
        float t=clampf(z->age/EMERGE_SECONDS,0,1);
        float s=t*t*(3-2*t); p.y=1-2.05f*(1-s);
    } else if(z->state==Z_FALLING) {
        float t=clampf(z->age/FALL_SECONDS,0,1),s=t*t*(3-2*t);
        p.lean=s*90; p.y=1-.62f*s;
    } else if(z->state==Z_CRAWLING) { p.lean=90; p.y=.38f; }
    else if(z->state==Z_DEAD) {
        float t=clampf(z->age/FALL_SECONDS,0,1);
        p.lean=z->death_lean+(90-z->death_lean)*t;
        p.y=z->death_y+(.38f-z->death_y)*t-fmaxf(0,z->age-4.5f)*.6f;
    }
    if(z->state==Z_STUNNED && z->crippled) { p.lean=90; p.y=.38f; }
    if(z->grab_pose>0) {
        float t=z->grab_pose;
        float target_y=z->crippled?.94f:1.0f;
        float target_lean=z->crippled?25:0;
        p.y=p.y*(1-t)+target_y*t; p.lean=p.lean*(1-t)+target_lean*t;
    }
    p.lean-=z->knock_pose*18;
    return p;
}
int zombie_parts(const Zombie *z,ZombiePart out[10]) {
    float swing=z->state==Z_WALKING?sinf(z->phase)*.11f:0;
    float dig=z->state==Z_RISING?sinf(z->age*12)*.17f:0;
    float crawl=z->state==Z_CRAWLING?sinf(z->phase)*.12f:0;
    /* Pivo da pose na cintura (Y=1). Frente local = +Z. */
    out[0]=(ZombiePart){0,1.78f,.02f,.42f,.42f,.40f,HIT_HEAD,1};
    out[1]=(ZombiePart){0,1.3f,0,.60f,.65f,.34f,HIT_BODY,0};
    out[2]=(ZombiePart){0,.90f,0,.50f,.22f,.33f,HIT_BODY,0};
    out[3]=(ZombiePart){-.16f,.42f,swing,.20f,.76f,.24f,HIT_LEG,0};
    out[4]=(ZombiePart){.16f,.42f,-swing,.20f,.76f,.24f,HIT_LEG,0};
    out[5]=(ZombiePart){-.40f,1.27f+dig,.17f+crawl,.20f,.48f,.23f,HIT_BODY,0};
    out[6]=(ZombiePart){.40f,1.27f-dig,.17f-crawl,.20f,.48f,.23f,HIT_BODY,0};
    out[7]=(ZombiePart){-.40f,1.12f+dig,.42f+crawl,.18f,.18f,.45f,HIT_BODY,1};
    out[8]=(ZombiePart){.40f,1.12f-dig,.42f-crawl,.18f,.18f,.45f,HIT_BODY,1};
    if(z->state==Z_RISING) {
        /* Bracos erguidos: maos rompem a terra antes do corpo terminar de sair. */
        out[5].y=1.60f+dig; out[6].y=1.60f-dig;
        out[7].y=1.90f+dig; out[8].y=1.90f-dig;
        out[7].sy=out[8].sy=.24f;
    }
    if(z->grab_pose>0) {
        float t=z->grab_pose;
        for(int k=5;k<=8;k++) {
            out[k].x=out[k].x*(1-t)+(k%2?-.34f:.34f)*t;
            out[k].y=out[k].y*(1-t)+1.43f*t;
            out[k].z=out[k].z*(1-t)+(k<7?.26f:.61f)*t;
        }
    }
    out[0].z+=z->bite_pose*.22f; out[0].y-=z->bite_pose*.06f;
    return 9;
}
/* Ray/AABB por slabs: direction pode ter componentes nulas. */
static float ray_box(const float o[3],const float d[3],const float lo[3],const float hi[3]) {
    float enter=0,leave=1000;
    for(int k=0;k<3;k++) {
        if(fabsf(d[k])<1e-7f) { if(o[k]<lo[k]||o[k]>hi[k]) return FLT_MAX; }
        else {
            float a=(lo[k]-o[k])/d[k],b=(hi[k]-o[k])/d[k];
            if(a>b) { float t=a; a=b; b=t; }
            enter=fmaxf(enter,a); leave=fminf(leave,b);
            if(enter>leave) return FLT_MAX;
        }
    }
    return enter;
}
static void inverse_yaw(float v[3],float degrees) {
    float a=degrees*PI/180,x=v[0],z=v[2];
    v[0]=cosf(a)*x-sinf(a)*z; v[2]=sinf(a)*x+cosf(a)*z;
}
float zombie_ray(const Zombie *z,const float origin[3],const float direction[3],HitRegion *region) {
    *region=HIT_NONE;
    if(z->state==Z_UNUSED||z->state==Z_DEAD) return FLT_MAX;
    ZombiePose p=zombie_pose(z);
    float o[]={origin[0]-z->x,origin[1]-p.y,origin[2]-z->z};
    float d[]={direction[0],direction[1],direction[2]};
    inverse_yaw(o,z->yaw); inverse_yaw(d,z->yaw);
    float a=p.lean*PI/180,y=o[1],zz=o[2];
    o[1]=cosf(a)*y+sinf(a)*zz+1; o[2]=-sinf(a)*y+cosf(a)*zz;
    y=d[1]; zz=d[2]; d[1]=cosf(a)*y+sinf(a)*zz; d[2]=-sinf(a)*y+cosf(a)*zz;
    ZombiePart parts[10]; int n=zombie_parts(z,parts); float best=FLT_MAX;
    for(int i=0;i<n;i++) {
        ZombiePart *b=&parts[i];
        float lo[]={b->x-b->sx/2,b->y-b->sy/2,b->z-b->sz/2};
        float hi[]={b->x+b->sx/2,b->y+b->sy/2,b->z+b->sz/2};
        float t=ray_box(o,d,lo,hi);
        if(t<best) { best=t; *region=b->region; }
    }
    return best;
}
static float ray_triangle(const float o[3],const float d[3],const float a[3],const float b[3],const float c[3]) {
    float u[3],v[3],p[3],s[3],q[3];
    for(int i=0;i<3;i++) { u[i]=b[i]-a[i]; v[i]=c[i]-a[i]; s[i]=o[i]-a[i]; }
    p[0]=d[1]*v[2]-d[2]*v[1]; p[1]=d[2]*v[0]-d[0]*v[2]; p[2]=d[0]*v[1]-d[1]*v[0];
    float det=u[0]*p[0]+u[1]*p[1]+u[2]*p[2]; if(fabsf(det)<1e-7f) return FLT_MAX;
    float inv=1/det,t1=(s[0]*p[0]+s[1]*p[1]+s[2]*p[2])*inv; if(t1<0||t1>1) return FLT_MAX;
    q[0]=s[1]*u[2]-s[2]*u[1]; q[1]=s[2]*u[0]-s[0]*u[2]; q[2]=s[0]*u[1]-s[1]*u[0];
    float t2=(d[0]*q[0]+d[1]*q[1]+d[2]*q[2])*inv; if(t2<0||t1+t2>1) return FLT_MAX;
    float t=(v[0]*q[0]+v[1]*q[1]+v[2]*q[2])*inv;
    return t>=0?t:FLT_MAX;
}
float world_ray(const World *w,const float origin[3],const float direction[3],float maximum) {
    float best=maximum;
    if(direction[1]<-1e-7f) best=fminf(best,-origin[1]/direction[1]);
    for(int i=0;i<w->box_count;i++) {
        const Box *b=&w->boxes[i];
        float lo[]={b->x-b->sx/2,b->y-b->sy/2,b->z-b->sz/2};
        float hi[]={b->x+b->sx/2,b->y+b->sy/2,b->z+b->sz/2};
        best=fminf(best,ray_box(origin,direction,lo,hi));
    }
    for(int i=0;i<w->house_count;i++) {
        const House *h=&w->houses[i];
        float o[]={origin[0]-(h->x-.7f),origin[1],origin[2]-(h->z+h->d/2+.11f)};
        float d[]={direction[0],direction[1],direction[2]};
        inverse_yaw(o,h->door*90); inverse_yaw(d,h->door*90);
        const float lo[]={0,0,-.06f},hi[]={1.4f,2.4f,.06f};
        best=fminf(best,ray_box(o,d,lo,hi));
        float x=h->w/2+.3f,z=h->d/2+.3f;
        float v[6][3]={{h->x-x,h->h,h->z-z},{h->x+x,h->h,h->z-z},{h->x,h->h+1.4f,h->z-z},
                       {h->x-x,h->h,h->z+z},{h->x+x,h->h,h->z+z},{h->x,h->h+1.4f,h->z+z}};
        const int tri[6][3]={{0,1,2},{3,5,4},{0,3,5},{0,5,2},{1,2,5},{1,5,4}};
        for(int j=0;j<6;j++) best=fminf(best,ray_triangle(origin,direction,v[tri[j][0]],v[tri[j][1]],v[tri[j][2]]));
    }
    return best;
}
int zombie_damage(Enemies *e,int index,HitRegion region) {
    if(index<0||index>=MAX_ZOMBIES||region==HIT_NONE) return 0;
    Zombie *z=&e->zombies[index]; if(z->state==Z_DEAD||z->state==Z_UNUSED) return 0;
    z->flash=.16f; e->feedback=.6f; e->last_headshot=region==HIT_HEAD;
    if(region==HIT_HEAD || ++z->body_hits>=3) {
        ZombiePose pose=zombie_pose(z); z->death_lean=pose.lean; z->death_y=pose.y;
        z->state=Z_DEAD; z->age=0; e->kills++; return 1;
    }
    if(region==HIT_LEG && !z->crippled) {
        z->crippled=1;
        if(z->state!=Z_RISING) { z->state=Z_FALLING; z->age=0; }
    }
    return 0;
}
HitRegion enemies_fire(Enemies *e,Game *g,const World *w) {
    if(e->health<=0||grapple_busy(e)||!game_shoot(g)) return HIT_NONE;
    float a=g->yaw*PI/180,p=g->pitch*PI/180;
    float origin[]={g->x,g->y,g->z},direction[]={cosf(a)*cosf(p),sinf(p),sinf(a)*cosf(p)};
    float best=world_ray(w,origin,direction,100); int index=-1; HitRegion region=HIT_NONE;
    for(int i=0;i<MAX_ZOMBIES;i++) {
        HitRegion part; float t=zombie_ray(&e->zombies[i],origin,direction,&part);
        if(t<best) { best=t; index=i; region=part; }
    }
    if(index>=0) zombie_damage(e,index,region);
    return region;
}
static void dirt_emit(Enemies *e,const Zombie *z) {
    for(int i=0;i<5;i++) {
        Dirt *p=&e->dirt[e->dirt_cursor++%MAX_DIRT];
        float a=rand01(e)*2*PI,r=.18f+rand01(e)*.35f;
        *p=(Dirt){z->x+cosf(a)*r,.04f,z->z+sinf(a)*r,
                  cosf(a)*(.4f+rand01(e)),1.4f+rand01(e)*2,sinf(a)*(.4f+rand01(e)),
                  .5f+rand01(e)*.4f,.04f+rand01(e)*.07f};
    }
}
static int clear_segment(const World *w,float x,float z,float tx,float tz,float radius) {
    float dx=tx-x,dz=tz-z; int steps=(int)(hypotf(dx,dz)/.18f)+1;
    for(int i=1;i<=steps;i++) if(world_blocked(w,x+dx*i/steps,z+dz*i/steps,radius)) return 0;
    return 1;
}
static int fits(const World *w,const Zombie *z,float x,float zz,float yaw) {
    if(world_blocked(w,x,zz,.29f)) return 0;
    if(z->state==Z_CRAWLING) {
        float a=yaw*PI/180;
        if(world_blocked(w,x+sinf(a)*.65f,zz+cosf(a)*.65f,.27f) ||
           world_blocked(w,x-sinf(a)*.6f,zz-cosf(a)*.6f,.27f)) return 0;
    }
    return 1;
}
static void choose_direction(Enemies *e,const World *w,const Game *g,Zombie *z) {
    float dx=g->x-z->x,dz=g->z-z->z;
    if(hypotf(dx,dz)<12 && clear_segment(w,z->x,z->z,g->x,g->z,.34f)) {
        float len=hypotf(dx,dz); z->dir_x=len>.01f?dx/len:0; z->dir_z=len>.01f?dz/len:0; return;
    }
    int n=cell(z->x,z->z),best=-1; float score=FLT_MAX;
    /* Escolhe centro alcancavel com menor distancia global. A pequena busca
       local tambem recupera zumbis deslocados da grade pela separacao. */
    if(n>=0) for(int dzc=-1;dzc<=1;dzc++) for(int dxc=-1;dxc<=1;dxc++) {
        int x=n%NAV_SIZE+dxc,zz=n/NAV_SIZE+dzc;
        if(x<0||zz<0||x>=NAV_SIZE||zz>=NAV_SIZE) continue;
        int v=zz*NAV_SIZE+x;
        if(e->distance[v]<0 || !clear_segment(w,z->x,z->z,cell_x(v),cell_z(v),.30f)) continue;
        float dist=hypotf(cell_x(v)-z->x,cell_z(v)-z->z);
        if(dist<.04f) continue;
        float s=(float)e->distance[v]+dist*.1f;
        if(s<score) { score=s; best=v; }
    }
    z->dir_x=z->dir_z=0;
    if(best>=0) {
        dx=cell_x(best)-z->x; dz=cell_z(best)-z->z; float len=hypotf(dx,dz);
        z->dir_x=dx/len; z->dir_z=dz/len;
    }
}
void enemies_update(Enemies *e,const World *w,const Game *g,float dt) {
    dt=clampf(dt,0,.02f); if(e->health<=0 || grapple_busy(e)) return;
    e->feedback=fmaxf(0,e->feedback-dt); e->hurt=fmaxf(0,e->hurt-dt);
    int alive=enemies_alive(e);
    e->nav_left-=dt;
    if((alive || e->spawning) && e->nav_left<=0) {
        enemies_rebuild_nav(e,w,g); e->nav_left=.5f;
    }
    if(e->spawning) {
        e->spawn_left-=dt;
        if(e->spawn_left<=0) {
            if(alive<MAX_ZOMBIES) for(int attempt=0;attempt<120;attempt++) {
                float x=-34+rand01(e)*68,z=-34+rand01(e)*68;
                float d=hypotf(x-g->x,z-g->z); if(d>23) continue;
                if(enemies_spawn_at(e,w,g,x,z)) break;
            }
            e->spawn_left=3.2f;
        }
    }
    for(int i=0;i<MAX_DIRT;i++) if(e->dirt[i].life>0) {
        Dirt *p=&e->dirt[i]; p->life-=dt; p->vy-=7*dt;
        p->x+=p->vx*dt; p->y+=p->vy*dt; p->z+=p->vz*dt;
        if(p->y<.025f) { p->y=.025f; p->vy=0; p->vx*=.9f; p->vz*=.9f; }
    }
    for(int i=0;i<MAX_ZOMBIES;i++) {
        Zombie *z=&e->zombies[i]; if(z->state==Z_UNUSED) continue;
        float old=z->age; z->age+=dt; z->flash=fmaxf(0,z->flash-dt);
        if(z->state==Z_DEAD) { if(z->age>6) z->state=Z_UNUSED; continue; }
        if(z->state==Z_RISING) {
            if((int)(old/.085f)!=(int)(z->age/.085f)) dirt_emit(e,z);
            if(z->age>=EMERGE_SECONDS) { z->state=z->crippled?Z_FALLING:Z_WALKING; z->age=0; }
            continue;
        }
        if(z->state==Z_FALLING) {
            if(z->age>=FALL_SECONDS) { z->state=Z_CRAWLING; z->age=0; }
            continue;
        }
        if(z->state==Z_STUNNED) {
            z->attack_left=fmaxf(0,z->attack_left-dt);
            z->flash=fmaxf(0,z->flash-dt);
            if(z->age>=1.2f) { z->state=z->crippled?Z_CRAWLING:Z_WALKING; z->age=0; }
            continue;
        }
        float dx=g->x-z->x,dz=g->z-z->z,dist=hypotf(dx,dz);
        z->attack_left=fmaxf(0,z->attack_left-dt);
        if(dist<1.15f && g->y<2.35f && clear_segment(w,z->x,z->z,g->x,g->z,.05f)) {
            z->yaw=atan2f(dx,dz)*180/PI;
            if(grapple_try_start(e,w,g,i)) return;
            continue;
        }
        z->think_left-=dt;
        if(z->think_left<=0) { choose_direction(e,w,g,z); z->think_left=.12f; }
        float vx=z->dir_x,vz=z->dir_z;
        /* Separacao local impede todos os agentes de ocuparem o mesmo ponto. */
        for(int j=0;j<MAX_ZOMBIES;j++) if(j!=i) {
            const Zombie *other=&e->zombies[j];
            if(other->state==Z_UNUSED||other->state==Z_DEAD) continue;
            float sx=z->x-other->x,sz=z->z-other->z,d2=sx*sx+sz*sz;
            if(d2>.0001f && d2<.85f*.85f) { vx+=sx/d2*.25f; vz+=sz/d2*.25f; }
        }
        float len=hypotf(vx,vz); if(len<.001f) continue;
        vx/=len; vz/=len; float yaw=atan2f(vx,vz)*180/PI;
        float speed=z->state==Z_CRAWLING?.62f:1.5f;
        float nx=z->x+vx*speed*dt,nz=z->z+vz*speed*dt;
        if(fits(w,z,nx,nz,yaw)) { z->x=nx; z->z=nz; z->yaw=yaw; }
        else {
            if(fits(w,z,nx,z->z,z->yaw)) z->x=nx;
            if(fits(w,z,z->x,nz,z->yaw)) z->z=nz;
            z->think_left=0;
        }
        z->phase+=dt*(z->state==Z_CRAWLING?5:8);
    }
}
