#include "grapple.h"
#include <math.h>
static float clampf(float x,float a,float b) { return fmaxf(a,fminf(x,b)); }
static float smooth(float x) { x=clampf(x,0,1); return x*x*(3-2*x); }
static float lerp_angle(float from,float to,float t) {
    float d=fmodf(to-from+540,360)-180; return from+d*t;
}
int grapple_busy(const Enemies *e) { return e->grab.state!=GRAB_NONE; }
int grapple_animating(const Enemies *e) {
    return e->grab.state!=GRAB_NONE && e->grab.state!=GRAB_DEAD;
}
static int free_path(const World *w,float x,float z,float nx,float nz,float radius) {
    float dx=nx-x,dz=nz-z; int n=(int)(hypotf(dx,dz)/.07f)+1;
    for(int i=0;i<=n;i++) if(world_blocked(w,x+dx*i/n,z+dz*i/n,radius)) return 0;
    return 1;
}
int grapple_try_start(Enemies *e,const World *w,const Game *g,int index) {
    if(grapple_busy(e)||e->grab.cooldown>0||e->health<=0||!g->grounded ||
       index<0||index>=MAX_ZOMBIES) return 0;
    Zombie *z=&e->zombies[index];
    if((z->state!=Z_WALKING&&z->state!=Z_CRAWLING)||z->attack_left>0) return 0;
    float dx=z->x-g->x,dz=z->z-g->z,dist=hypotf(dx,dz);
    if(dist>1.15f || g->y>EYE_HEIGHT+.12f ||
       !free_path(w,z->x,z->z,g->x,g->z,.06f)) return 0;
    /* Nunca inicia uma luta atraves da folha de uma porta ou outro objeto. */
    float o[]={g->x,g->y,g->z};
    float target_y=z->state==Z_CRAWLING?.55f:1.65f;
    float d[]={dx,target_y-g->y,dz},len=sqrtf(dx*dx+d[1]*d[1]+dz*dz);
    if(len>.001f) { for(int i=0;i<3;i++) d[i]/=len;
        if(world_ray(w,o,d,len+.01f)<len-.12f) return 0;
    }
    if(dist<.8f) {
        /* Se o jogador entrou no collider do inimigo, separa-os sem atravessar paredes. */
        float a=g->yaw*PI/180;
        if(dist<.01f) { dx=cosf(a); dz=sinf(a); dist=1; }
        float nx=g->x+dx/dist*.95f,nz=g->z+dz/dist*.95f;
        if(!free_path(w,z->x,z->z,nx,nz,.30f)) return 0;
        z->x=nx; z->z=nz; dx=nx-g->x; dz=nz-g->z; dist=hypotf(dx,dz);
    }
    if(z->state==Z_CRAWLING) z->crippled=1;
    Grapple *q=&e->grab; *q=(Grapple){0}; q->state=GRAB_ENTER; q->owner=index;
    q->start_yaw=g->yaw; q->start_pitch=g->pitch;
    q->target_yaw=atan2f(dz,dx)*180/PI;
    /* Rastejantes erguem o tronco durante o agarre, preservando pernas feridas. */
    q->target_pitch=z->crippled||z->state==Z_CRAWLING?-8:3;
    q->away_x=dx/dist; q->away_z=dz/dist;
    z->yaw=atan2f(-dx,-dz)*180/PI; z->dir_x=z->dir_z=0;
    return 1;
}
void grapple_input(Enemies *e,int down) {
    Grapple *q=&e->grab;
    if(down && !q->key_held && q->state==GRAB_MASH) {
        if(q->taps<QTE_TAPS) q->taps++;
        q->pulse=.15f;
        if(q->taps>=QTE_TAPS) { q->state=GRAB_PUNCH; q->age=0; }
    }
    q->key_held=down!=0;
}
static void push_zombie(Enemies *e,const World *w,Zombie *z,float dx,float dz) {
    float nx=z->x+dx,nz=z->z+dz;
    if(!free_path(w,z->x,z->z,nx,nz,.34f)) return;
    for(int i=0;i<MAX_ZOMBIES;i++) {
        const Zombie *a=&e->zombies[i];
        if(a==z||a->state==Z_UNUSED||a->state==Z_DEAD) continue;
        if(hypotf(a->x-nx,a->z-nz)<.65f) return;
    }
    z->x=nx; z->z=nz;
}
void grapple_update(Enemies *e,const World *w,Game *g,float dt) {
    dt=clampf(dt,0,.02f); Grapple *q=&e->grab;
    q->cooldown=fmaxf(0,q->cooldown-dt); q->pulse=fmaxf(0,q->pulse-dt);
    if(!grapple_animating(e)) return;
    if(q->owner<0||q->owner>=MAX_ZOMBIES) { q->state=GRAB_NONE; return; }
    Zombie *z=&e->zombies[q->owner];
    if((q->state==GRAB_ENTER||q->state==GRAB_MASH) &&
       (z->state==Z_UNUSED||z->state==Z_DEAD)) {
        z->grab_pose=z->bite_pose=z->knock_pose=0; q->state=GRAB_NONE; q->owner=-1; q->key_held=0; q->cooldown=1; return;
    }
    z->flash=fmaxf(0,z->flash-dt);
    q->age+=dt;
    if(q->state==GRAB_ENTER) {
        float t=smooth(q->age/GRAB_ENTER_SECONDS);
        z->grab_pose=t;
        g->yaw=lerp_angle(q->start_yaw,q->target_yaw,t);
        g->pitch=q->start_pitch+(q->target_pitch-q->start_pitch)*t;
        if(q->age>=GRAB_ENTER_SECONDS) { q->state=GRAB_MASH; q->age=0; }
    } else if(q->state==GRAB_MASH) {
        z->grab_pose=1;
        if(q->age>=QTE_SECONDS) { q->state=GRAB_BITE; q->age=0; }
    } else if(q->state==GRAB_PUNCH) {
        z->grab_pose=1;
        if(q->age>=GRAB_PUNCH_SECONDS*.5f) z->flash=.12f;
        if(q->age>=GRAB_PUNCH_SECONDS) {
            q->state=GRAB_PUSH; q->age=0; z->state=Z_STUNNED; z->age=0;
        }
    } else if(q->state==GRAB_PUSH) {
        float t=clampf(q->age/GRAB_PUSH_SECONDS,0,1);
        z->grab_pose=1-smooth(t); z->knock_pose=sinf(t*PI);
        push_zombie(e,w,z,q->away_x*3.2f*dt,q->away_z*3.2f*dt);
        if(q->age>=GRAB_PUSH_SECONDS) {
            z->grab_pose=z->bite_pose=z->knock_pose=0;
            z->state=Z_STUNNED; z->age=0; z->attack_left=4;
            q->state=GRAB_NONE; q->owner=-1; q->cooldown=4;
            q->key_held=0; g->yaw=q->target_yaw; g->pitch=q->target_pitch;
        }
    } else if(q->state==GRAB_BITE) {
        z->grab_pose=1; z->bite_pose=smooth(q->age/(GRAB_BITE_SECONDS*.6f));
        q->blood=clampf((q->age-.28f)*1.7f,0,.7f);
        if(q->age>.35f) { e->health=0; e->hurt=.25f; }
        if(q->age>=GRAB_BITE_SECONDS) { q->state=GRAB_FALL; q->age=0; }
    } else if(q->state==GRAB_FALL) {
        z->grab_pose=1-smooth(q->age/.6f); z->bite_pose=z->grab_pose;
        q->blood=fminf(1,.7f+q->age*.3f);
        if(q->age>=GRAB_FALL_SECONDS) {
            q->state=GRAB_DEAD; q->age=0; e->health=0; z->grab_pose=z->bite_pose=0;
        }
    }
}
PlayerView grapple_view(const Enemies *e,const Game *g) {
    const Grapple *q=&e->grab;
    PlayerView v={g->x,g->y,g->z,g->yaw,g->pitch,0};
    if(q->state==GRAB_MASH||q->state==GRAB_ENTER) {
        float strength=q->state==GRAB_MASH?1:clampf(q->age/GRAB_ENTER_SECONDS,0,1);
        v.yaw+=sinf(q->age*23)*.65f*strength;
        v.pitch+=sinf(q->age*31)*.6f*strength; v.roll=sinf(q->age*19)*1.2f*strength;
    } else if(q->state==GRAB_PUNCH) {
        float t=q->age/GRAB_PUNCH_SECONDS; v.pitch-=sinf(t*PI)*3; v.roll=-sinf(t*PI)*4;
    } else if(q->state==GRAB_BITE) {
        v.pitch-=sinf(q->age*12)*3; v.roll=sinf(q->age*15)*5;
    } else if(q->state==GRAB_FALL||q->state==GRAB_DEAD) {
        float t=q->state==GRAB_DEAD?1:smooth(q->age/GRAB_FALL_SECONDS);
        v.y=g->y+(.24f-g->y)*t; v.roll=t*68;
        v.pitch=g->pitch+(-24-g->pitch)*t;
    }
    return v;
}
