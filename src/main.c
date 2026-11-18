/* A Casa: demonstracao academica em C11 / OpenGL classico / FreeGLUT. */
#include <GL/freeglut.h>
#include <GL/glu.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "zombies.h"
#include "zombie_render.h"
#include "grapple.h"
#include "grapple_render.h"
#include <time.h>
static World world;
static Game game;
static Enemies enemies;
static int enemy_render_test=0;
static const char *qte_preview=NULL;
static void capture(int enabled);
static int width=1100,height=720,locked=1,keys[256],last_ms,help=1;
static int smoke_test=0,frame_count=0;
static float accumulator=0,smoke_clock=0;
typedef struct { float x,y,z,life,size; } Smoke;
static Smoke smoke[48];
static float clampf(float v,float a,float b) { return fmaxf(a,fminf(v,b)); }
static void cube(float x,float y,float z,float sx,float sy,float sz,float r,float g,float b) {
    glColor3f(r,g,b); glPushMatrix(); glTranslatef(x,y,z); glScalef(sx,sy,sz);
    glutSolidCube(1); glPopMatrix();
}
static void roof(const House *h) {
    float x=h->w/2+.3f,z=h->d/2+.3f;
    glPushMatrix(); glTranslatef(h->x,h->h,h->z);
    glColor3f(.48f,.13f,.07f);
    glBegin(GL_TRIANGLES);
    glVertex3f(-x,0,z); glVertex3f(x,0,z); glVertex3f(0,1.4f,z);
    glVertex3f(-x,0,-z); glVertex3f(0,1.4f,-z); glVertex3f(x,0,-z);
    glEnd(); glBegin(GL_QUADS);
    glColor3f(.58f,.18f,.08f);
    glVertex3f(-x,0,-z); glVertex3f(-x,0,z); glVertex3f(0,1.4f,z); glVertex3f(0,1.4f,-z);
    glColor3f(.40f,.10f,.05f);
    glVertex3f(x,0,z); glVertex3f(x,0,-z); glVertex3f(0,1.4f,-z); glVertex3f(0,1.4f,z);
    glEnd(); glPopMatrix();
}
static void draw_world(void) {
    cube(0,-.12f,0,78,.24f,78,.19f,.34f,.14f);
    for(int i=0;i<world.box_count;i++) {
        Box *b=&world.boxes[i]; cube(b->x,b->y,b->z,b->sx,b->sy,b->sz,b->r,b->g,b->b);
    }
    for(int i=0;i<world.tree_count;i++) {
        Tree *t=&world.trees[i];
        glPushMatrix(); glTranslatef(t->x,2*t->scale,t->z);
        glRotatef(-90,1,0,0); glColor3f(.08f,.23f,.10f);
        glutSolidCone(1.5*t->scale,3.5*t->scale,8,1);
        glTranslatef(0,0,1.1f*t->scale); glColor3f(.12f,.31f,.13f);
        glutSolidCone(1.25*t->scale,3*t->scale,8,1); glPopMatrix();
    }
    for(int i=0;i<world.house_count;i++) {
        House *h=&world.houses[i]; roof(h);
        glPushMatrix(); glTranslatef(h->x-.7f,0,h->z+h->d/2+.11f);
        glRotatef(h->door*90,0,1,0);
        cube(.7f,1.2f,0,1.4f,2.4f,.12f,h->main_house?.12f:.35f,.22f,.29f);
        cube(1.22f,1.15f,.09f,.1f,.1f,.1f,.82f,.65f,.24f); glPopMatrix();
        /* Janelas decorativas na fachada: nao atravessaveis. */
        cube(h->x+h->w*.32f,1.7f,h->z+h->d/2+.12f,.9f,.95f,.04f,.16f,.39f,.43f);
        cube(h->x+h->w*.32f,1.7f,h->z+h->d/2+.15f,.06f,1,.05f,.2f,.13f,.08f);
        cube(h->x+h->w*.32f,1.7f,h->z+h->d/2+.15f,.95f,.06f,.05f,.2f,.13f,.08f);
        if(!game.ammo_taken[i]) {
            cube(h->x,.22f,h->z-1.1f,.5f,.44f,.4f,.2f,.35f,.12f);
            cube(h->x,.23f,h->z-.89f,.34f,.10f,.01f,.9f,.76f,.21f);
        }
    }
    cube(1.8f,3.8f,0,.8f,1.6f,.9f,.4f,.4f,.4f);
    cube(1.8f-2.5f*(1-game.chimney),4.9f-4.5f*(1-game.chimney),0,
         1.3f,.5f,1.3f,.28f,.28f,.3f);
    /* Limite fisico do mapa assinalado por cerca. */
    for(int i=-36;i<=36;i+=4) {
        cube((float)i,.8f,37,.16f,1.6f,.16f,.26f,.21f,.13f);
        cube((float)i,.8f,-37,.16f,1.6f,.16f,.26f,.21f,.13f);
        cube(37,.8f,(float)i,.16f,1.6f,.16f,.26f,.21f,.13f);
        cube(-37,.8f,(float)i,.16f,1.6f,.16f,.26f,.21f,.13f);
    }
    for(int i=0;i<2;i++) {
        float y=.5f+i*.65f;
        cube(0,y,37,74,.09f,.09f,.34f,.28f,.17f); cube(0,y,-37,74,.09f,.09f,.34f,.28f,.17f);
        cube(37,y,0,.09f,.09f,74,.34f,.28f,.17f); cube(-37,y,0,.09f,.09f,74,.34f,.28f,.17f);
    }
}
static void puff(void) {
    for(int i=0;i<48;i++) if(smoke[i].life<=0) {
        smoke[i]=(Smoke){1.8f,5.2f,0,1,.28f}; break;
    }
}
static void draw_smoke(void) {
    /* Ordenacao de tras para frente pela distancia da camera. */
    int order[48],n=0;
    for(int i=0;i<48;i++) if(smoke[i].life>0) order[n++]=i;
    for(int i=0;i<n;i++) for(int j=i+1;j<n;j++) {
        Smoke *a=&smoke[order[i]],*b=&smoke[order[j]];
        float da=(a->x-game.x)*(a->x-game.x)+(a->y-game.y)*(a->y-game.y)+(a->z-game.z)*(a->z-game.z);
        float db=(b->x-game.x)*(b->x-game.x)+(b->y-game.y)*(b->y-game.y)+(b->z-game.z)*(b->z-game.z);
        if(db>da) { int k=order[i]; order[i]=order[j]; order[j]=k; }
    }
    glDepthMask(GL_FALSE);
    for(int j=0;j<n;j++) {
        Smoke *p=&smoke[order[j]]; glColor4f(.75f,.77f,.75f,p->life*.45f);
        glPushMatrix(); glTranslatef(p->x,p->y,p->z); glutSolidCube(p->size); glPopMatrix();
    }
    glDepthMask(GL_TRUE);
}
static void rect(float x,float y,float w,float h,float r,float g,float b) {
    glColor3f(r,g,b); glBegin(GL_QUADS);
    glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h); glEnd();
}
static void text(float x,float y,const char *s) {
    glRasterPos2f(x,y); while(*s) glutBitmapCharacter(GLUT_BITMAP_8_BY_13,*s++);
}
static void hud(void) {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0,width,0,height);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_FOG);
    if(!grapple_busy(&enemies)) {
    float scale=height/720.0f,bx=width/2+sinf(game.bob)*10*scale;
    float drop=game.reloading?sinf((1-game.reload_left/RELOAD_SECONDS)*PI)*110:0;
    float by=40+(fabsf(cosf(game.bob))*6+(game.flash_left>0?12:0)-drop)*scale;
    glPushMatrix(); glTranslatef(bx,by,0); glScalef(scale,scale,1);
    rect(-27,0,54,110,.16f,.15f,.14f); rect(-60,70,120,112,.25f,.26f,.28f);
    rect(-50,80,100,99,.46f,.48f,.49f); rect(-32,153,64,70,.24f,.25f,.27f);
    rect(-23,160,46,60,.38f,.4f,.42f); rect(-9,220,18,16,.1f,.1f,.1f);
    for(int i=0;i<4;i++) rect(-48,95+i*13,14,4,.2f,.21f,.22f);
    if(game.flash_left>0) {
        glBegin(GL_TRIANGLES); glColor3f(1,.8f,.12f);
        glVertex2f(0,294); glVertex2f(-40,230); glVertex2f(40,230); glEnd();
    }
    glPopMatrix();
    rect(width/2-9,height/2-1,6,2,.9f,.9f,.75f); rect(width/2+3,height/2-1,6,2,.9f,.9f,.75f);
    rect(width/2-1,height/2+3,2,6,.9f,.9f,.75f); rect(width/2-1,height/2-9,2,6,.9f,.9f,.75f);
    }
    rect(0,0,width,38,.07f,.08f,.07f);
    char line[160]; snprintf(line,sizeof line,"MUNICAO %02d / %02d    RESERVA %03d    %s",game.magazine,
        MAGAZINE_SIZE,game.reserve,game.reloading?"RECARREGANDO...":game.magazine==0?"APERTE R PARA RECARREGAR":"PISTOLA");
    glColor3f(.92f,.78f,.41f); text(18,15,line);
    glColor3f(.9f,.9f,.8f); text(18,height-24,"A Casa");
    if(!grapple_busy(&enemies) && game_near_door(&game,&world)>=0) text(width/2-110,height/2+30,"[E] Abrir / fechar porta");
    if(game.reloading && !grapple_busy(&enemies)) {
        rect(width/2-80,height/2-36,160,6,.16f,.16f,.16f);
        rect(width/2-80,height/2-36,160*(1-game.reload_left/RELOAD_SECONDS),6,.88f,.65f,.23f);
    }
    int button_x=width-245,button_y=height-44;
    rect((float)button_x,(float)button_y,227,32,enemies.spawning?.26f:.12f,.19f,.10f);
    glColor3f(.96f,.85f,.54f);
    text((float)button_x+12,(float)button_y+12,enemies.spawning?"[Z] SPAWN: LIGADO":"[Z] SPAWN: DESLIGADO");
    snprintf(line,sizeof line,"VIDA %03d   ZUMBIS %02d   ABATES %03d",enemies.health,enemies_alive(&enemies),enemies.kills);
    text(18,51,line);
    if(enemies.feedback>0) {
        glColor3f(enemies.last_headshot?1:.8f,enemies.last_headshot?.35f:.85f,.15f);
        text(width/2-72,height/2+57,enemies.last_headshot?"HEADSHOT - MORTO":"ACERTO NO CORPO");
        glBegin(GL_LINES);
        glVertex2f(width/2-12,height/2-12); glVertex2f(width/2-6,height/2-6);
        glVertex2f(width/2+12,height/2-12); glVertex2f(width/2+6,height/2-6);
        glVertex2f(width/2-12,height/2+12); glVertex2f(width/2-6,height/2+6);
        glVertex2f(width/2+12,height/2+12); glVertex2f(width/2+6,height/2+6); glEnd();
    }
    if(enemies.hurt>0) {
        rect(0,38,7,height-38,.65f,.05f,.04f); rect(width-7,38,7,height-38,.65f,.05f,.04f);
    }
    if(help) {
        glColor3f(.92f,.92f,.82f);
        text(18,height-49,"WASD: mover | Mouse: olhar | Clique: atirar | R: recarregar");
        text(18,height-67,"Espaco: pular | E: portas / escapar | C: chamine | F: fumaca | H: ajuda");
        text(18,height-85,"ESC: pausa / botao de spawn | Z: spawn | T: reiniciar | Q: sair");
    }
    draw_grapple_overlay(&enemies,width,height);
    if(!locked && (enemies.health>0 || grapple_animating(&enemies))) {
        rect(width/2-190,height/2-45,380,90,.08f,.1f,.08f);
        glColor3f(.95f,.85f,.55f); text(width/2-145,height/2+10,"PAUSADO - ESC OU CLIQUE PARA VOLTAR");
        text(width/2-145,height/2-17,"Clique em SPAWN para ligar/desligar");
    }
    if(enemies.health<=0 && !grapple_animating(&enemies)) {
        rect(width/2-210,height/2-42,420,84,.12f,.045f,.035f);
        glColor3f(1,.68f,.4f); text(width/2-105,height/2+8,"VOCE MORREU");
        text(width/2-148,height/2-17,"[T] Reiniciar   |   [Q] Sair");
    }
    glEnable(GL_FOG); glEnable(GL_DEPTH_TEST);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}
static void display(void) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); glLoadIdentity();
    PlayerView view=grapple_view(&enemies,&game);
    float a=view.yaw*PI/180,p=view.pitch*PI/180,r=view.roll*PI/180;
    float fx=cosf(a)*cosf(p),fy=sinf(p),fz=sinf(a)*cosf(p);
    float ux=-cosf(a)*sinf(p)*cosf(r)-sinf(a)*sinf(r);
    float uy=cosf(p)*cosf(r),uz=-sinf(a)*sinf(p)*cosf(r)+cosf(a)*sinf(r);
    gluLookAt(view.x,view.y,view.z,view.x+fx,view.y+fy,view.z+fz,ux,uy,uz);
    draw_world(); draw_player_blood(&enemies,&game); draw_enemies(&enemies); draw_smoke();
    draw_grapple_hands(&enemies,(float)width/height); hud(); glutSwapBuffers();
    if(smoke_test && ++frame_count>=5) {
        GLenum error=glGetError();
        printf("Renderizacao: 5 frames, erro GL = %u\n",(unsigned)error);
        if(error!=GL_NO_ERROR) exit(EXIT_FAILURE);
        glutLeaveMainLoop();
    }
}
static void reshape(int w,int h) {
    width=w>0?w:1; height=h>0?h:1; glViewport(0,0,width,height);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(65,(double)width/height,.08,130);
    glMatrixMode(GL_MODELVIEW);
    if(locked) glutWarpPointer(width/2,height/2);
}
static void simulation_step(float step) {
    float old_doors[MAX_HOUSES];
    for(int j=0;j<world.house_count;j++) old_doors[j]=world.houses[j].door;
    int held=grapple_busy(&enemies);
    game_update(&game,&world,step,held?0:(float)(keys['w']-keys['s']),
                held?0:(float)(keys['d']-keys['a']),held?0:keys[' ']);
    keys[' ']=0;
    for(int j=0;j<world.house_count;j++) for(int k=0;k<MAX_ZOMBIES;k++) {
        Zombie *z=&enemies.zombies[k];
        if(z->state!=Z_UNUSED && z->state!=Z_DEAD &&
           door_blocked(&world.houses[j],world.houses[j].door,z->x,z->z,.35f))
            world.houses[j].door=old_doors[j];
    }
    enemies_update(&enemies,&world,&game,step);
    if(!held && grapple_busy(&enemies)) grapple_input(&enemies,keys['e']);
    grapple_update(&enemies,&world,&game,step);
    for(int i=0;i<48;i++) if(smoke[i].life>0) {
        smoke[i].life-=step*.45f; smoke[i].y+=step*.7f;
        smoke[i].x+=step*.14f; smoke[i].size+=step*.16f;
    }
    smoke_clock+=step;
    if(smoke_clock>.15f && game.chimney>=1) { puff(); smoke_clock=0; }
}
static void timer(int value) {
    (void)value; int now=glutGet(GLUT_ELAPSED_TIME);
    float dt=clampf((now-last_ms)/1000.0f,0,.1f); last_ms=now;
    if(locked && (enemies.health>0 || grapple_animating(&enemies)) && !enemy_render_test && !qte_preview) {
        accumulator+=dt;
        while(accumulator>=1.0f/120) {
            float step=1.0f/120;
            simulation_step(step); accumulator-=step;
            if(enemies.health<=0 && !grapple_animating(&enemies)) { capture(0); accumulator=0; break; }
        }
    } else accumulator=0;
    glutPostRedisplay(); glutTimerFunc(8,timer,0);
}
static void capture(int enabled) {
    locked=enabled; memset(keys,0,sizeof keys); grapple_input(&enemies,0);
    glutSetCursor(locked?GLUT_CURSOR_NONE:GLUT_CURSOR_INHERIT);
    if(locked) glutWarpPointer(width/2,height/2);
}
static void key_down(unsigned char key,int x,int y) {
    (void)x;(void)y; key=(unsigned char)tolower(key);
    if(key==27) { if(enemies.health>0 || grapple_animating(&enemies)) capture(!locked); return; }
    if(key=='q') { glutLeaveMainLoop(); return; }
    if(key=='h') { help=!help; return; }
    if(key=='z') { enemies_toggle(&enemies); return; }
    if(key=='t') {
        world_init(&world); game_init(&game); enemies_init(&enemies,(unsigned)time(NULL));
        memset(smoke,0,sizeof smoke); smoke_clock=0; accumulator=0;
        qte_preview=NULL; enemy_render_test=0; capture(1); return;
    }
    if(enemies.health<=0) return;
    if(!locked || keys[key]) return;
    keys[key]=1;
    if(grapple_busy(&enemies)) { if(key=='e') grapple_input(&enemies,1); return; }
    if(key=='r') game_reload(&game);
    if(key=='e') game_interact(&game,&world);
    if(key=='c') game.chimney=0;
    if(key=='f') puff();
}
static void key_up(unsigned char key,int x,int y) {
    (void)x;(void)y; key=(unsigned char)tolower(key); keys[key]=0;
    if(key=='e') grapple_input(&enemies,0);
}
static void mouse_move(int x,int y) {
    if(!locked || (x==width/2 && y==height/2)) return;
    if(!grapple_busy(&enemies)) {
        game.yaw=fmodf(game.yaw+(x-width/2)*.15f,360);
        game.pitch=clampf(game.pitch+(height/2-y)*.15f,-85,85);
    }
    glutWarpPointer(width/2,height/2);
}
static void mouse_click(int button,int state,int x,int y) {
    (void)x;(void)y; if(button!=GLUT_LEFT_BUTTON || state!=GLUT_DOWN) return;
    if(!locked && x>=width-245 && x<=width-18 && y>=12 && y<=44) {
        enemies_toggle(&enemies); return;
    }
    if(enemies.health<=0) return;
    if(!locked) { capture(1); return; }
    if(!grapple_busy(&enemies)) enemies_fire(&enemies,&game,&world);
}
static void entry(int state) { if(state==GLUT_LEFT && locked) capture(0); }
static void setup_qte_preview(const char *phase) {
    game.z=18; game.y=EYE_HEIGHT; game.yaw=-90; game.pitch=0;
    Zombie *z=&enemies.zombies[0]; *z=(Zombie){0}; z->state=Z_WALKING; z->z=17;
    grapple_try_start(&enemies,&world,&game,0);
    for(int i=0;i<60;i++) grapple_update(&enemies,&world,&game,1.0f/120);
    for(int i=0;i<6;i++) { grapple_input(&enemies,1); grapple_input(&enemies,0); }
    if(strcmp(phase,"grab")==0) return;
    if(strcmp(phase,"punch")==0||strcmp(phase,"push")==0) {
        for(int i=0;i<6;i++) { grapple_input(&enemies,1); grapple_input(&enemies,0); }
        int n=strcmp(phase,"punch")==0?20:76;
        for(int i=0;i<n;i++) grapple_update(&enemies,&world,&game,1.0f/120);
    } else {
        int n=strcmp(phase,"bite")==0?530:strcmp(phase,"fall")==0?640:850;
        for(int i=0;i<n;i++) grapple_update(&enemies,&world,&game,1.0f/120);
    }
}
int main(int argc,char **argv) {
    for(int i=1;i<argc;i++) {
        if(strcmp(argv[i],"--smoke-test")==0) smoke_test=1;
        if(strcmp(argv[i],"--enemy-render-test")==0) enemy_render_test=1;
        if(strncmp(argv[i],"--qte-preview=",14)==0) qte_preview=argv[i]+14;
    }
    world_init(&world); game_init(&game);
    enemies_init(&enemies,(unsigned)time(NULL));
    if(enemy_render_test) {
        /* Cena reproduzivel para verificar poses e particulas, sem IA. */
        game.z=18; game.y=2.1f; game.pitch=-8;
        for(int i=0;i<4;i++) {
            Zombie *z=&enemies.zombies[i]; z->x=-3.3f+i*2.2f; z->z=12;
            z->state=i==0?Z_RISING:i==1?Z_WALKING:i==2?Z_FALLING:Z_CRAWLING;
            z->age=i==0?1.6f:.35f; z->phase=1; z->yaw=0;
        }
        for(int i=0;i<30;i++) enemies.dirt[i]=(Dirt){-3.3f+(i%5-2)*.16f,.12f+(i/5)*.12f,
            12+(i%3-1)*.23f,0,0,0,1,.07f};
    }
    if(qte_preview) setup_qte_preview(qte_preview);
    glutInit(&argc,argv); glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
    glutInitWindowSize(width,height); glutCreateWindow("A Casa");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutIgnoreKeyRepeat(1);
    glEnable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(.49f,.65f,.68f,1);
    GLfloat fog[]={.49f,.65f,.68f,1}; glEnable(GL_FOG); glFogfv(GL_FOG_COLOR,fog);
    glFogi(GL_FOG_MODE,GL_LINEAR); glFogf(GL_FOG_START,25); glFogf(GL_FOG_END,85);
    glutDisplayFunc(display); glutReshapeFunc(reshape);
    glutKeyboardFunc(key_down); glutKeyboardUpFunc(key_up);
    glutPassiveMotionFunc(mouse_move); glutMotionFunc(mouse_move); glutMouseFunc(mouse_click);
    glutEntryFunc(entry); capture(1); last_ms=glutGet(GLUT_ELAPSED_TIME); glutTimerFunc(8,timer,0);
    glutMainLoop(); return 0;
}
