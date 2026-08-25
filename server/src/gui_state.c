/*
** ZAPPY - gui_state.c
** L'etat du monde pousse a un GUI qui vient de se connecter.
**
** Un GUI peut arriver longtemps apres le debut de la partie : il faut
** donc lui decrire tout ce qui existe deja, sans qu'il ait rien a
** demander. Le serveur de reference envoie, dans cet ordre : la taille de
** la carte, l'unite de temps, le contenu des cases, les noms d'equipes,
** puis chaque joueur et chaque oeuf encore en attente.
*/

#include <stdio.h>
#include "server.h"

/* Previent tous les GUI que le contenu d'une case a change. */
void gui_send_tile(server_t *srv, int x, int y)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (srv->clients[i].fd != -1 && srv->clients[i].state == STATE_GUI)
            gui_send_bct(srv, &srv->clients[i], x, y);
}

/* tna : un message par equipe. */
void gui_send_tna(server_t *srv, client_t *c)
{
    char line[128];

    for (int t = 0; t < srv->nb_teams; t++) {
        snprintf(line, sizeof(line), "tna %s\n", srv->team_names[t]);
        queue_output(c, line);
    }
}

/* Pour chaque drone deja en jeu : son existence, son niveau, son sac. */
static void gui_send_players(server_t *srv, client_t *c)
{
    char line[192];
    client_t *p;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        p = &srv->clients[i];
        if (p->fd == -1 || p->state != STATE_AI)
            continue;
        snprintf(line, sizeof(line), "pnw #%d %d %d %d %d %s\n",
            p->id, p->x, p->y, p->orientation, p->level,
            p->team_name ? p->team_name : "");
        queue_output(c, line);
        snprintf(line, sizeof(line), "plv #%d %d\n", p->id, p->level);
        queue_output(c, line);
    }
}

/* Les oeufs encore en attente d'un client. */
static void gui_send_eggs(server_t *srv, client_t *c)
{
    char line[80];

    for (int i = 0; i < srv->nb_eggs; i++) {
        snprintf(line, sizeof(line), "enw #%d #%d %d %d\n",
            srv->eggs[i].id, srv->eggs[i].parent_id,
            srv->eggs[i].x, srv->eggs[i].y);
        queue_output(c, line);
    }
}

void gui_send_state(server_t *srv, client_t *c)
{
    char line[64];

    snprintf(line, sizeof(line), "msz %d %d\n", srv->width, srv->height);
    queue_output(c, line);
    snprintf(line, sizeof(line), "sgt %d\n", srv->freq);
    queue_output(c, line);
    gui_send_mct(srv, c);
    gui_send_tna(srv, c);
    gui_send_players(srv, c);
    gui_send_eggs(srv, c);
}
