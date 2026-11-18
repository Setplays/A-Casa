/* Exercita os callbacks reais com um contexto grafico.
   Precisa de display/GLX; nao faz parte dos testes sem janela. */
#define main casa_application_main
#include "../src/main.c"
#undef main
#include <assert.h>
int main(int argc,char **argv) {
    glutInit(&argc,argv); glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
    glutInitWindowSize(width,height); int window=glutCreateWindow("A Casa");
    world_init(&world); game_init(&game); enemies_init(&enemies,7);
    reshape(width,height); glEnable(GL_DEPTH_TEST);
    capture(0);
    mouse_click(GLUT_LEFT_BUTTON,GLUT_DOWN,width-100,25);
    assert(enemies.spawning && !locked); /* botao clicavel sem retomar partida */
    mouse_click(GLUT_LEFT_BUTTON,GLUT_DOWN,width-100,25);
    assert(!enemies.spawning && !locked);
    key_down('z',0,0); assert(enemies.spawning);
    capture(1); game.z=18; game.y=1.78f;
    enemies.zombies[0]=(Zombie){0}; enemies.zombies[0].state=Z_WALKING;
    enemies.zombies[0].z=12;
    mouse_click(GLUT_LEFT_BUTTON,GLUT_DOWN,width/2,height/2);
    assert(enemies.kills==1 && enemies.zombies[0].state==Z_DEAD && game.magazine==11);
    key_down('t',0,0);
    assert(enemies.health==100 && !enemies.spawning && enemies_alive(&enemies)==0 && game.magazine==12);
    /* Verifica renderizacao de todas as posturas, inclusive corpo enterrado. */
    game.z=18;
    for(int i=0;i<5;i++) {
        Zombie *z=&enemies.zombies[i]; z->x=-4+i*2.0f; z->z=12;
        z->state=i==0?Z_RISING:i==1?Z_WALKING:i==2?Z_FALLING:i==3?Z_CRAWLING:Z_DEAD;
        z->age=i==0?1.5f:.35f; z->death_y=1; z->yaw=0;
    }
    display(); assert(glGetError()==GL_NO_ERROR);
    enemies.health=0; capture(0); int ammo=game.magazine;
    mouse_click(GLUT_LEFT_BUTTON,GLUT_DOWN,width/2,height/2); assert(!locked && game.magazine==ammo);
    key_down('t',0,0); assert(locked && enemies.health==100);
    /* Agarramento real: bloqueia movimento, tiro, recarga e a acao da porta. */
    game.z=4.55f; enemies.zombies[0]=(Zombie){0};
    enemies.zombies[0].state=Z_WALKING; enemies.zombies[0].z=3.6f;
    simulation_step(1.0f/120); assert(enemies.grab.state==GRAB_ENTER);
    float anchor_x=game.x,anchor_z=game.z;
    keys['w']=1;
    for(int i=0;i<65;i++) simulation_step(1.0f/120);
    assert(enemies.grab.state==GRAB_MASH && game.x==anchor_x && game.z==anchor_z);
    int door=world.houses[0].open;
    key_down('e',0,0);
    for(int i=0;i<30;i++) key_down('e',0,0);
    assert(enemies.grab.taps==1 && world.houses[0].open==door);
    key_up('e',0,0);
    int bullets=game.magazine;
    mouse_click(GLUT_LEFT_BUTTON,GLUT_DOWN,width/2,height/2);
    key_down('r',0,0); assert(game.magazine==bullets && !game.reloading);
    float yaw=game.yaw; mouse_move(width/2+20,height/2+10); assert(game.yaw==yaw);
    capture(0); float age=enemies.grab.age; timer(0);
    assert(enemies.grab.age==age); key_down('e',0,0); assert(enemies.grab.taps==1);
    capture(1);
    for(int i=1;i<QTE_TAPS;i++) { key_down('e',0,0); key_up('e',0,0); }
    assert(enemies.grab.state==GRAB_PUNCH);
    for(int i=0;i<135;i++) simulation_step(1.0f/120);
    assert(!grapple_busy(&enemies) && enemies.health==100);
    /* Renderiza cada fase de forma reproduzivel, incluindo a camera no chao. */
    const char *phases[]={"grab","punch","push","bite","fall","dead"};
    const GrabState states[]={GRAB_MASH,GRAB_PUNCH,GRAB_PUSH,GRAB_BITE,GRAB_FALL,GRAB_DEAD};
    for(int i=0;i<6;i++) {
        game_init(&game); enemies_init(&enemies,7); setup_qte_preview(phases[i]);
        assert(enemies.grab.state==states[i]);
        display(); assert(glGetError()==GL_NO_ERROR);
    }
    key_down('t',0,0);
    assert(enemies.grab.state==GRAB_NONE && enemies.grab.blood==0 && enemies.health==100);
    glutDestroyWindow(window);
    puts("OK: botao, Z, headshot, reset, QTE/tecla E, pausa, bloqueios, 6 fases GL e derrota.");
    return 0;
}
