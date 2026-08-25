/*
** ZAPPY - commands.c
** Execution des commandes d'un drone : deplacements et aiguillage.
**
** Orientation (releve sur le serveur de reference en observant ppo) :
**     1 = Nord  -> y - 1        3 = Sud   -> y + 1
**     2 = Est   -> x + 1        4 = Ouest -> x - 1
** Tourner a droite fait 1 -> 2 -> 3 -> 4 -> 1, d'ou la formule
** o % 4 + 1 ; tourner a gauche est la rotation inverse.
*/

#include <string.h>
#include <stdio.h>
#include "server.h"

/* Vecteur d'avancee pour chaque orientation (indices 1 a 4). */
static const int DX[5] = {0, 0, 1, 0, -1};
static const int DY[5] = {0, -1, 0, 1, 0};

/*
** Avance d'une case. map_wrap() se charge du tore : sortir a droite fait
** reapparaitre a gauche, sans qu'on ait de cas particulier a ecrire.
*/
static void cmd_forward(server_t *srv, client_t *c)
{
    c->x = map_wrap(c->x + DX[c->orientation], srv->width);
    c->y = map_wrap(c->y + DY[c->orientation], srv->height);
    queue_output(c, "ok\n");
    gui_notify_ppo(srv, c);
}

static void cmd_right(server_t *srv, client_t *c)
{
    c->orientation = c->orientation % 4 + 1;
    queue_output(c, "ok\n");
    gui_notify_ppo(srv, c);
}

static void cmd_left(server_t *srv, client_t *c)
{
    c->orientation = (c->orientation + 2) % 4 + 1;
    queue_output(c, "ok\n");
    gui_notify_ppo(srv, c);
}

/* Nombre de places encore libres dans l'equipe du joueur. */
static void cmd_connect_nbr(server_t *srv, client_t *c)
{
    char line[32];

    snprintf(line, sizeof(line), "%d\n",
        srv->clients_nb - srv->team_used[c->team_idx]);
    queue_output(c, line);
}

/* Aiguillage : appele quand l'echeance de la commande est arrivee. */
void ai_execute(server_t *srv, client_t *c, const char *line)
{
    if (strcmp(line, "Forward") == 0)
        return cmd_forward(srv, c);
    if (strcmp(line, "Right") == 0)
        return cmd_right(srv, c);
    if (strcmp(line, "Left") == 0)
        return cmd_left(srv, c);
    if (strcmp(line, "Look") == 0)
        return cmd_look(srv, c);
    if (strcmp(line, "Inventory") == 0)
        return cmd_inventory(srv, c);
    if (strcmp(line, "Connect_nbr") == 0)
        return cmd_connect_nbr(srv, c);
    if (strncmp(line, "Take ", 5) == 0)
        return cmd_take(srv, c, line + 5);
    if (strncmp(line, "Set ", 4) == 0)
        return cmd_set(srv, c, line + 4);
    if (strncmp(line, "Broadcast ", 10) == 0)
        return cmd_broadcast(srv, c, line + 10);
    /* TODO: Fork, Eject, Incantation. */
    queue_output(c, "ko\n");
}
