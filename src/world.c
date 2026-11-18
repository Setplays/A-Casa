#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static void box(World *w,float x,float y,float z,float sx,float sy,float sz,
                float r,float g,float b,int solid) {
    if(w->box_count>=MAX_BOXES) { fputs("Limite de objetos excedido\n",stderr); abort(); }
    w->boxes[w->box_count++]=(Box){x,y,z,sx,sy,sz,r,g,b,solid};
}
static void house(World *w,float x,float z,float width,float depth,int main_house) {
    House h={x,z,width,depth,3.0f,main_house,0,0};
    float a=width/2,b=depth/2;
    float r=main_house?.99f:.44f,g=main_house?.69f:.26f,bl=main_house?0:.13f;
    w->houses[w->house_count++]=h;
    /* Fachada dividida: vao central de 1.4 m e verga a 2.4 m. */
    box(w,x-a,1.5f,z,.2f,3,depth,r,g,bl,1);
    box(w,x+a,1.5f,z,.2f,3,depth,r,g,bl,1);
    box(w,x,1.5f,z-b,width,3,.2f,r,g,bl,1);
    box(w,x-(a+.7f)/2,1.5f,z+b,a-.7f,3,.2f,r,g,bl,1);
    box(w,x+(a+.7f)/2,1.5f,z+b,a-.7f,3,.2f,r,g,bl,1);
    box(w,x,2.7f,z+b,1.4f,.6f,.2f,r,g,bl,0);
    box(w,x,.015f,z,width,.03f,depth,.36f,.25f,.17f,0);
    box(w,x,3.0f,z,width,.08f,depth,.32f,.20f,.13f,0);
    /* Mesa, cama e estante: mesma geometria para desenho e colisao. */
    box(w,x+.45f,.68f,z+.1f,1.1f,.12f,.8f,.48f,.25f,.10f,1);
    for(int i=0;i<4;i++) box(w,x+.45f+(i%2? .44f:-.44f),.33f,
        z+.1f+(i/2?.3f:-.3f),.1f,.66f,.1f,.30f,.16f,.06f,0);
    box(w,x-a+.9f,.30f,z-b+1.4f,1.25f,.6f,2.0f,.38f,.20f,.1f,1);
    box(w,x-a+.9f,.66f,z-b+1.4f,1.2f,.12f,1.95f,.8f,.83f,.8f,0);
    box(w,x-a+.9f,.75f,z-b+1.7f,1.22f,.12f,1.2f,.16f,.32f,.62f,0);
    box(w,x-a+.9f,.79f,z-b+.68f,.85f,.14f,.35f,.96f,.95f,.88f,0);
    box(w,x+a-.4f,1.0f,z-b+1.1f,.5f,2,1.3f,.3f,.16f,.08f,1);
    for(int i=0;i<3;i++) {
        box(w,x+a-.68f,.4f+i*.6f,z-b+1.1f,.06f,.07f,1.3f,.6f,.39f,.2f,0);
        box(w,x+a-.72f,.56f+i*.6f,z-b+.9f,.08f,.28f,.3f,.2f+i*.2f,.25f,.6f-i*.15f,0);
    }
}
/* PRNG local: floresta reproduzivel, sem afetar os futuros spawns. */
static unsigned seed=314159u;
static float random01(void) { seed=1664525u*seed+1013904223u; return (seed>>8)/16777216.0f; }
void world_init(World *w) {
    *w=(World){0}; seed=314159u;
    house(w,0,0,6,6,1);
    house(w,-13,-10,5,5,0); house(w,13,-9,5,5,0); house(w,12,12,5,5,0);
    /* Caminhos sem collider, inclusive ate as portas das cabanas. */
    box(w,0,.008f,10,2,.01f,14,.57f,.48f,.32f,0);
    box(w,0,.009f,7,36,.01f,1.8f,.57f,.48f,.32f,0);
    box(w,-13,.009f,-.75f,1.8f,.01f,15.5f,.57f,.48f,.32f,0);
    box(w,13,.009f,1,1.8f,.01f,15,.57f,.48f,.32f,0);
    /* Acesso pela frente da cabana sudeste, contornando a parede de fundo. */
    box(w,17,.009f,12,1.8f,.01f,10,.57f,.48f,.32f,0);
    box(w,14.5f,.009f,17,6.8f,.01f,1.8f,.57f,.48f,.32f,0);
    box(w,12,.009f,16,1.8f,.01f,3,.57f,.48f,.32f,0);
    /* Jardim a esquerda da casa, canteiros elevados e flores. */
    for(int row=0;row<3;row++) {
        float z=1.0f+row*2.1f;
        box(w,-7,.16f,z,4,.32f,1.15f,.32f,.20f,.10f,1);
        box(w,-7,.33f,z,3.8f,.03f,.95f,.17f,.10f,.05f,0);
        for(int j=0;j<7;j++) {
            float x=-8.6f+j*.53f;
            box(w,x,.56f,z,.06f,.45f,.06f,.15f,.42f,.08f,0);
            box(w,x,.82f,z,.23f,.18f,.23f,row==1?1:.8f,row==2?.7f:.14f,.36f,0);
        }
    }
    /* Banco ao lado do jardim. */
    box(w,-5,.52f,7.3f,2,.18f,.65f,.42f,.24f,.10f,1);
    box(w,-5,.9f,7.6f,2,.8f,.12f,.42f,.24f,.10f,1);
    /* Arvores ao redor da clareira central. Tronco com collider; copa decorativa. */
    for(int i=0;i<180;i++) {
        float x=-35+random01()*70,z=-35+random01()*70;
        if(fabsf(x)<19 && fabsf(z)<19) continue;
        float s=.8f+random01()*.6f;
        w->trees[w->tree_count++]=(Tree){x,z,s};
        box(w,x,1.5f*s,z,.48f*s,3*s,.48f*s,.27f,.16f,.08f,1);
    }
}
