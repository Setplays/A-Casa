#include <GL/freeglut.h>
#include <GL/glu.h>
#include <math.h>
#include <stdio.h>
#include "grapple_render.h"
static float clampf(float v,float lo,float hi) { return fmaxf(lo,fminf(v,hi)); }
static void box(float x,float y,float z,float sx,float sy,float sz,float r,float g,float b) {
    glColor3f(r,g,b); glPushMatrix(); glTranslatef(x,y,z); glScalef(sx,sy,sz);
    glutSolidCube(1); glPopMatrix();
}
static void limb(const float a[3],const float b[3],float width,float r,float g,float blue) {
    float dx=b[0]-a[0],dy=b[1]-a[1],dz=b[2]-a[2],len=sqrtf(dx*dx+dy*dy+dz*dz);
    if(len<.001f) return;
    glPushMatrix(); glTranslatef((a[0]+b[0])*.5f,(a[1]+b[1])*.5f,(a[2]+b[2])*.5f);
    if(fabsf(dx)+fabsf(dy)>.00001f)
        glRotatef(acosf(clampf(dz/len,-1,1))*180/PI,-dy,dx,0);
    else if(dz<0) glRotatef(180,1,0,0);
    box(0,0,0,width,width,len,r,g,blue); glPopMatrix();
}
void draw_grapple_hands(const Enemies *e,float aspect) {
    const Grapple *q=&e->grab; if(!grapple_busy(e)||q->state==GRAB_DEAD) return;
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluPerspective(65,aspect,.025,5);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    /* Viewmodel: maos desenhadas depois do mundo, sem sumirem dentro do captor. */
    glClear(GL_DEPTH_BUFFER_BIT); glEnable(GL_DEPTH_TEST); glDisable(GL_FOG);
    float enter=q->state==GRAB_ENTER?clampf(q->age/GRAB_ENTER_SECONDS,0,1):1;
    for(int side=0;side<2;side++) {
        float sign=side?1:-1;
        float hand[]={sign*.27f,-.09f-(1-enter)*.65f,-.78f};
        float shake=q->state==GRAB_MASH?sinf(q->age*26+side*2)*.016f:0;
        hand[0]+=shake; hand[1]+=q->pulse*.18f;
        int fist=0;
        if(q->state==GRAB_PUNCH) {
            float t=clampf(q->age/GRAB_PUNCH_SECONDS,0,1),thrust=sinf(t*PI);
            if(side) { hand[0]=.27f-.23f*thrust; hand[1]=-.18f+.17f*thrust;
                hand[2]=-.50f-.57f*thrust; fist=1; }
            else { hand[1]-=.14f*thrust; hand[2]+=.12f*thrust; }
        } else if(q->state==GRAB_PUSH) {
            float t=clampf(q->age/GRAB_PUSH_SECONDS,0,1);
            hand[0]=sign*(.27f+.12f*t); hand[1]=-.09f-.48f*t*t;
            hand[2]=-.78f-.15f*sinf(t*PI);
        } else if(q->state==GRAB_BITE) {
            hand[0]+=sign*sinf(q->age*18)*.025f; hand[1]-=.06f;
        } else if(q->state==GRAB_FALL) {
            float t=clampf(q->age/GRAB_FALL_SECONDS,0,1);
            hand[0]+=sign*.4f*t; hand[1]-=t*.9f; hand[2]+=.25f*t;
        }
        float shoulder[]={sign*.61f,-.73f,-.20f};
        float elbow[]={sign*.39f,hand[1]-.23f,-.45f};
        limb(shoulder,elbow,.19f,.13f,.19f,.14f);
        limb(elbow,hand,.135f,.66f,.45f,.30f);
        box(hand[0],hand[1],hand[2],.15f,.11f,.17f,.72f,.51f,.35f);
        for(int finger=0;finger<4;finger++) {
            float fx=hand[0]-.056f+finger*.037f;
            box(fx,hand[1]+(fist?.045f:.075f),hand[2]-(fist?.02f:.065f),
                .031f,fist?.06f:.12f,.065f,.78f,.56f,.39f);
        }
        box(hand[0]-sign*.092f,hand[1]-.005f,hand[2]-.04f,.065f,.075f,.10f,.72f,.5f,.34f);
        box(hand[0],hand[1]-.075f,hand[2]+.03f,.16f,.045f,.13f,.18f,.22f,.17f);
    }
    glEnable(GL_FOG); glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}
void draw_player_blood(const Enemies *e,const Game *g) {
    float b=e->grab.blood; if(b<=0) return;
    float a=g->yaw*PI/180,r=.25f+b*.65f;
    glColor4f(.28f,.015f,.012f,.92f);
    glBegin(GL_TRIANGLE_FAN);
    float x=g->x+cosf(a)*.65f,z=g->z+sinf(a)*.65f;
    glVertex3f(x,.035f,z);
    for(int i=0;i<=32;i++) {
        float angle=i*2*PI/32,edge=r*(1+.08f*sinf(angle*7));
        glVertex3f(x+cosf(angle)*edge,.035f,z+sinf(angle)*edge*.7f);
    }
    glEnd();
}
static void rect(float x,float y,float w,float h,float r,float g,float b,float alpha) {
    glColor4f(r,g,b,alpha); glBegin(GL_QUADS);
    glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h); glEnd();
}
static void label(float x,float y,const char *s) {
    glColor3f(1,.90f,.70f); glRasterPos2f(x,y);
    while(*s) glutBitmapCharacter(GLUT_BITMAP_9_BY_15,*s++);
}
void draw_grapple_overlay(const Enemies *e,int width,int height) {
    const Grapple *q=&e->grab;
    if(q->blood>0) {
        float b=q->blood;
        rect(0,0,width,height,.35f,.005f,0,.12f*b);
        rect(0,0,width,height*.10f,.30f,.005f,0,.6f*b);
        rect(0,0,width*.06f,height,.36f,.005f,0,.5f*b);
        rect(width*.94f,0,width*.06f,height,.36f,.005f,0,.5f*b);
        /* Respingo estilizado e escorrimentos, sem ferida explicita. */
        glColor4f(.46f,.01f,.008f,.66f*b);
        for(int i=0;i<11;i++) {
            float x=width*(.045f+.091f*i),y=height*(.12f+.10f*(i%3));
            float radius=(10+(i%4)*6)*b;
            glBegin(GL_TRIANGLE_FAN); glVertex2f(x,y);
            for(int j=0;j<=12;j++) { float a=j*2*PI/12;
                glVertex2f(x+cosf(a)*radius,y+sinf(a)*radius*.8f); }
            glEnd();
            rect(x-radius*.18f,y-40*b,radius*.36f,40*b,.46f,.01f,.008f,.6f*b);
        }
    }
    if(!grapple_animating(e)) return;
    float x=width*.5f-220,y=height*.22f;
    if(q->state==GRAB_ENTER||q->state==GRAB_MASH) {
        rect(x-12,y-10,464,113,.045f,.04f,.03f,.92f);
        label(x+15,y+73,q->state==GRAB_ENTER?"O ZUMBI TE AGARROU!":"APERTE [E] REPETIDAMENTE PARA ESCAPAR");
        float progress=(float)q->taps/QTE_TAPS;
        rect(x,y+42,440,15,.19f,.17f,.12f,1);
        rect(x,y+42,440*progress,15,.76f,.64f,.22f,1);
        float time=q->state==GRAB_ENTER?1:clampf(1-q->age/QTE_SECONDS,0,1);
        rect(x,y+19,440,8,.18f,.11f,.08f,1);
        rect(x,y+19,440*time,8,.70f,.19f,.08f,1);
        char s[100]; snprintf(s,sizeof s,"ESFORCO %02d/%02d     TEMPO %.1f s",q->taps,QTE_TAPS,
            q->state==GRAB_ENTER?QTE_SECONDS:fmaxf(0,QTE_SECONDS-q->age));
        label(x+30,y+1,s);
    } else {
        const char *s=q->state==GRAB_PUNCH?"VOCE ESCAPOU!":q->state==GRAB_PUSH?"AFASTE-SE DO ZUMBI!":
                      q->state==GRAB_BITE?"ELE TE MORDEU!":"";
        if(*s) { rect(width*.5f-150,y,300,39,.06f,.035f,.025f,.8f);
            label(width*.5f-(float)18*(q->state==GRAB_PUSH?5.0f:4.0f),y+13,s); }
    }
}
