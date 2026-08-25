/*
** ZAPPY - cmd_incant.c
** Le rituel d'elevation, du debut a la fin.
**
** Deroulement :
**   1. Incantation : si les conditions ne sont pas reunies, "ko" tout de
**      suite. Sinon tous les drones du meme niveau presents sur la case
**      recoivent "Elevation underway" et sont FIGES.
**   2. Pendant le rituel (INCANT_UNITS(niveau) unites), les participants
**      n'executent plus rien. Leurs commandes s'empilent.
**   3. A l'echeance, on revalide les conditions sur la case du rituel.
**      Reussite : les pierres disparaissent, tout le monde monte d'un
**      niveau. Echec : "ko" pour tous, les pierres restent.
**
** Un Eject pendant le rituel chasse un participant de la case : il en
** manque un a la revalidation, et l'elevation echoue. C'est la seule
** facon d'interrompre une incantation.
*/

#include <stdio.h>
#include <string.h>
#include "server.h"

/*
** Fige tous les participants et annonce le debut aux GUI.
** Le message pic liste la case, le niveau, puis chaque participant.
*/
static void start_ritual(server_t *srv, client_t *c, uint64_t end)
{
    char line[512];
    char one[24];
    client_t *p;

    snprintf(line, sizeof(line), "pic %d %d %d", c->x, c->y, c->level);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        p = &srv->clients[i];
        if (p->fd == -1 || p->state != STATE_AI || p->level != c->level
            || p->x != c->x || p->y != c->y)
            continue;
        p->incant_end_us = end;
        p->incant_id = srv->next_incant_id;
        p->incant_leader = (p == c);
        p->incant_x = c->x;
        p->incant_y = c->y;
        queue_output(p, "Elevation underway\n");
        snprintf(one, sizeof(one), " #%d", p->id);
        if (strlen(line) + strlen(one) + 2 < sizeof(line))
            strcat(line, one);
    }
    strcat(line, "\n");
    gui_broadcast(srv, line);
}

void cmd_incantation(server_t *srv, client_t *c)
{
    if (!incant_ready(srv, c->x, c->y, c->level)) {
        queue_output(c, "ko\n");
        return;
    }
    srv->next_incant_id++;
    start_ritual(srv, c, now_us() + units_to_us(srv, INCANT_UNITS(c->level)));
}

/*
** Le drone reprend la main. Les commandes envoyees pendant le rituel ont
** des echeances desormais depassees : on les decale pour qu'elles
** reprennent a partir de maintenant, au lieu de partir toutes d'un coup.
*/
static void unfreeze(client_t *p, uint64_t now)
{
    uint64_t shift;

    p->incant_end_us = 0;
    p->incant_id = 0;
    p->incant_leader = 0;
    if (p->nb_actions == 0 || p->actions[0].end_us >= now)
        return;
    shift = now - p->actions[0].end_us;
    for (int i = 0; i < p->nb_actions; i++)
        p->actions[i].end_us += shift;
}

/* L'echeance est arrivee : on revalide, puis on recompense ou on sanctionne. */
static void incant_finish(server_t *srv, client_t *lead, uint64_t now)
{
    /* On releve tout AVANT la boucle : unfreeze() remet ces champs a zero
    ** des le premier participant traite, et comparer a une valeur remise a
    ** zero embarquerait tous les clients qui n'ont pas d'incantation. */
    int id = lead->incant_id;
    int level = lead->level;
    int x = lead->incant_x;
    int y = lead->incant_y;
    int ok = incant_ready(srv, x, y, level);
    tile_t *t = map_at(srv, x, y);
    char line[64];
    client_t *p;

    for (int r = RES_LINEMATE; ok && r < NB_RESOURCES; r++)
        t->qty[r] -= incant_need(level, r);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        p = &srv->clients[i];
        if (p->fd == -1 || p->incant_id != id)
            continue;
        if (ok)
            player_level_up(srv, p);
        else
            queue_output(p, "ko\n");
        unfreeze(p, now);
    }
    snprintf(line, sizeof(line), "pie %d %d %d\n", x, y, ok);
    gui_broadcast(srv, line);
    if (ok)
        gui_send_tile(srv, x, y);
}

/* Appele a chaque tour d'horloge, pour chaque drone. */
void incant_tick(server_t *srv, client_t *c, uint64_t now)
{
    if (c->incant_end_us == 0 || c->incant_leader == 0)
        return;
    if (now < c->incant_end_us)
        return;
    incant_finish(srv, c, now);
}
