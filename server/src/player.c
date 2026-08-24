/*
** ZAPPY - player.c
** Naissance d'un drone et messages qui le decrivent aux GUI.
**
** Le GUI ne voit jamais le jeu directement : il n'apprend l'existence et
** les mouvements des joueurs que par ces messages.
**   pnw #n X Y O L equipe   un joueur apparait
**   ppo #n X Y O            sa position / son orientation ont change
**   pin #n X Y q0..q6       son inventaire a change
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

/* Envoie un message a tous les GUI connectes (et a eux seuls). */
void gui_broadcast(server_t *srv, const char *msg)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (srv->clients[i].fd != -1 && srv->clients[i].state == STATE_GUI)
            queue_output(&srv->clients[i], msg);
}

void gui_notify_pnw(server_t *srv, client_t *c)
{
    char line[192];

    snprintf(line, sizeof(line), "pnw #%d %d %d %d %d %s\n",
        c->id, c->x, c->y, c->orientation, c->level,
        c->team_name ? c->team_name : "");
    gui_broadcast(srv, line);
}

void gui_notify_ppo(server_t *srv, client_t *c)
{
    char line[64];

    snprintf(line, sizeof(line), "ppo #%d %d %d %d\n",
        c->id, c->x, c->y, c->orientation);
    gui_broadcast(srv, line);
}

void gui_notify_pin(server_t *srv, client_t *c)
{
    char line[128];

    snprintf(line, sizeof(line), "pin #%d %d %d %d %d %d %d %d %d %d\n",
        c->id, c->x, c->y, c->inventory[RES_FOOD], c->inventory[RES_LINEMATE],
        c->inventory[RES_DERAUMERE], c->inventory[RES_SIBUR],
        c->inventory[RES_MENDIANE], c->inventory[RES_PHIRAS],
        c->inventory[RES_THYSTAME]);
    gui_broadcast(srv, line);
}

/*
** Place un nouveau drone : position et orientation au hasard, niveau 1,
** et de quoi tenir un moment. Puis on previent les GUI.
*/
void player_spawn(server_t *srv, client_t *c)
{
    c->id = srv->next_player_id++;
    c->x = rand() % srv->width;
    c->y = rand() % srv->height;
    c->orientation = rand() % 4 + 1;
    c->level = 1;
    memset(c->inventory, 0, sizeof(c->inventory));
    c->inventory[RES_FOOD] = START_FOOD;
    c->food_end_us = now_us() + units_to_us(srv, FOOD_UNITS);
    gui_notify_pnw(srv, c);
    gui_notify_pin(srv, c);
}
