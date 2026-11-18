#ifndef GRAPPLE_RENDER_H
#define GRAPPLE_RENDER_H
#include "grapple.h"
void draw_grapple_hands(const Enemies *e,float aspect);
void draw_player_blood(const Enemies *e,const Game *g);
/* Chamado com a projecao ortografica do HUD ja ativa. */
void draw_grapple_overlay(const Enemies *e,int width,int height);
#endif
