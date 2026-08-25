/*
** ZAPPY - cmd_fork.c
** Fork et Eject : pondre un oeuf, et pousser les autres hors de sa case.
**
** Fork (42 unites, soit 0.42 s a f=100 : la commande la plus lente apres
** l'incantation) pose un oeuf sur la case du drone. Cela n'ajoute pas un
** joueur : cela ajoute une PLACE, qu'un nouveau client viendra occuper.
**
** Eject (7 unites, 0.07 s a f=100) balaie la case :
**   - tous les AUTRES drones qui s'y trouvent sont pousses d'une case dans
**     la direction ou regarde l'ejecteur, allies comme ennemis ;
**   - tous les oeufs qui s'y trouvent sont ecrases.
** Chaque victime recoit "eject: K", ou K designe le cote d'ou vient la
** poussee dans son propre repere -- meme numerotation que Broadcast.
**
** Le rapport de couts fait tout l'interet de la commande : ecraser un oeuf
** coute 7 unites la ou en pondre un en coute 42, et disperser des drones
** reunis pour une incantation ruine 300 unites de preparation.
**
** Note : le serveur de reference ne detruit PAS les oeufs, contrairement
** au sujet. On suit ici le sujet. Il repond aussi "ok" meme quand il n'y
** avait rien a ejecter, et on s'aligne sur lui sur ce point.
*/

#include <stdio.h>
#include "server.h"

void cmd_fork(server_t *srv, client_t *c)
{
    char line[64];

    snprintf(line, sizeof(line), "pfk #%d\n", c->id);
    gui_broadcast(srv, line);
    egg_add(srv, c->team_idx, c->id, c->x, c->y);
    queue_output(c, "ok\n");
}

/* Pousse une victime d'une case et lui dit d'ou vient le coup. */
static void push_away(server_t *srv, client_t *c, client_t *v)
{
    static const int dx[5] = {0, 0, 1, 0, -1};
    static const int dy[5] = {0, -1, 0, 1, 0};
    char line[32];

    v->x = map_wrap(v->x + dx[c->orientation], srv->width);
    v->y = map_wrap(v->y + dy[c->orientation], srv->height);
    /* L'ejecteur n'a pas bouge : vu de la victime deplacee, il est
    ** desormais du cote oppose a la poussee. */
    snprintf(line, sizeof(line), "eject: %d\n",
        direction_from(srv, v, c->x, c->y));
    queue_output(v, line);
    gui_notify_ppo(srv, v);
}

/*
** Ecrase tous les oeufs de la case, sans regarder l'equipe : un drone
** maladroit detruit donc aussi ceux des siens. La place est perdue pour
** de bon, exactement comme si le drone qui en serait sorti etait mort.
*/
static void crush_eggs(server_t *srv, int x, int y)
{
    char line[32];
    int i = 0;

    while (i < srv->nb_eggs) {
        if (srv->eggs[i].x != x || srv->eggs[i].y != y) {
            i++;
            continue;
        }
        snprintf(line, sizeof(line), "edi #%d\n", srv->eggs[i].id);
        gui_broadcast(srv, line);
        for (int k = i + 1; k < srv->nb_eggs; k++)
            srv->eggs[k - 1] = srv->eggs[k];
        srv->nb_eggs--;
        /* i ne bouge pas : l'oeuf suivant a pris la place de celui-ci. */
    }
}

void cmd_eject(server_t *srv, client_t *c)
{
    char line[32];
    client_t *v;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        v = &srv->clients[i];
        if (v->fd == -1 || v == c || v->state != STATE_AI)
            continue;
        if (v->x == c->x && v->y == c->y)
            push_away(srv, c, v);
    }
    crush_eggs(srv, c->x, c->y);
    queue_output(c, "ok\n");
    snprintf(line, sizeof(line), "pex #%d\n", c->id);
    gui_broadcast(srv, line);
}
