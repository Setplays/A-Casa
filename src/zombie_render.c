#include <GL/freeglut.h>
#include <math.h>
#include "zombie_render.h"
static void box(float x,float y,float z,float sx,float sy,float sz,float r,float g,float b) {
    glColor3f(r,g,b); glPushMatrix(); glTranslatef(x,y,z); glScalef(sx,sy,sz);
    glutSolidCube(1); glPopMatrix();
}
static void mound(const Zombie *z) {
    float s=z->state==Z_RISING?(.85f+.06f*sinf(z->age*30)):1;
    glPushMatrix(); glTranslatef(z->x,.025f,z->z); glScalef(s,1,s);
    glColor3f(.20f,.12f,.065f); glBegin(GL_TRIANGLE_FAN); glVertex3f(0,.06f,0);
    for(int j=0;j<=16;j++) { float a=j*2*PI/16; glVertex3f(cosf(a)*.7f,0,sinf(a)*.7f); }
    glEnd(); glColor3f(.10f,.065f,.03f); glBegin(GL_LINES);
    for(int j=0;j<8;j++) { float a=j*2*PI/8;
        glVertex3f(cosf(a)*.22f,.07f,sinf(a)*.22f);
        glVertex3f(cosf(a+.12f)*.58f,.015f,sinf(a+.12f)*.58f); }
    glEnd(); glPopMatrix();
}
void draw_enemies(const Enemies *e) {
    /* Plano em coordenadas de mundo: esconde a parte ainda enterrada. */
    const GLdouble ground[]={0,1,0,-.012}; glClipPlane(GL_CLIP_PLANE0,ground);
    for(int i=0;i<MAX_ZOMBIES;i++) {
        const Zombie *z=&e->zombies[i]; if(z->state==Z_UNUSED) continue;
        if(z->state==Z_RISING) mound(z);
        ZombiePose pose=zombie_pose(z); ZombiePart parts[10]; int n=zombie_parts(z,parts);
        glEnable(GL_CLIP_PLANE0); glPushMatrix();
        glTranslatef(z->x,pose.y,z->z); glRotatef(z->yaw,0,1,0); glRotatef(pose.lean,1,0,0);
        glTranslatef(0,-1,0);
        for(int k=0;k<n;k++) {
            const ZombiePart *p=&parts[k]; float r,g,b;
            if(p->skin) { r=.39f; g=.52f; b=.25f; }
            else if(p->region==HIT_LEG) { r=.18f; g=.19f; b=.20f; }
            else { r=.32f; g=.23f; b=.18f; }
            if(z->flash>0) { r=.85f; g=.53f; b=.24f; }
            box(p->x,p->y,p->z,p->sx,p->sy,p->sz,r,g,b);
        }
        /* Olhos, boca e manchas no tecido permanecem dentro das hitboxes. */
        if(pose.lean>45) {
            /* No rastejo o rosto olha para a frente, em vez de encarar o chao. */
            box(-.10f,1.997f,-.07f,.075f,.012f,.045f,.94f,.74f,.16f);
            box(.10f,1.997f,-.07f,.075f,.012f,.045f,.94f,.74f,.16f);
            box(0,1.998f,.10f,.17f,.01f,.045f,.11f,.10f,.07f);
        } else {
            float bite=z->bite_pose;
            box(-.10f,1.84f-bite*.06f,.226f+bite*.22f,.075f,.045f,.012f,.94f,.74f,.16f);
            box(.10f,1.84f-bite*.06f,.226f+bite*.22f,.075f,.045f,.012f,.94f,.74f,.16f);
            box(0,1.68f-bite*.06f,.227f+bite*.22f,.17f,.045f+bite*.13f,.01f,.11f,.10f,.07f);
            if(bite>0) for(int t=0;t<4;t++)
                box(-.06f+t*.04f,1.71f-bite*.06f,.238f+bite*.22f,.024f,.035f,.012f,.82f,.8f,.61f);
        }
        box(-.12f,1.27f,.176f,.12f,.18f,.01f,.15f,.18f,.09f);
        glPopMatrix(); glDisable(GL_CLIP_PLANE0);
    }
    for(int i=0;i<MAX_DIRT;i++) {
        const Dirt *p=&e->dirt[i]; if(p->life<=0) continue;
        box(p->x,p->y,p->z,p->size,p->size*.7f,p->size,.32f,.20f,.09f);
    }
}
