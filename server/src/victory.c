/*
** ZAPPY - victory.c
** Monter d'un niveau, et la condition de fin de partie.
**
** Le sujet : "l'equipe gagnante est la premiere dont au moins 6 joueurs
** atteignent l'elevation maximale". On verifie donc apres chaque montee
** de niveau, et on previent les GUI par "seg <equipe>".
*/

#include <stdio.h>
#include "server.h"

/* Combien de drones de cette equipe ont atteint le niveau maximum. */
static int count_at_max(server_t *srv, int team_idx)
{
    client_t *p;
    int n = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        p = &srv->clients[i];
        if (p->fd != -1 && p->state == STATE_AI
            && p->team_idx == team_idx && p->level >= MAX_LEVEL)
            n++;
    }
    return n;
}

/*
** Le drone monte d'un niveau : il l'apprend, les GUI aussi, et on regarde
** si son equipe vient de remporter la partie.
*/
void player_level_up(server_t *srv, client_t *c)
{
    char line[128];

    c->level++;
    snprintf(line, sizeof(line), "Current level: %d\n", c->level);
    queue_output(c, line);
    snprintf(line, sizeof(line), "plv #%d %d\n", c->id, c->level);
    gui_broadcast(srv, line);
    if (srv->game_over || c->level < MAX_LEVEL)
        return;
    if (count_at_max(srv, c->team_idx) < WIN_PLAYERS)
        return;
    srv->game_over = 1;
    snprintf(line, sizeof(line), "seg %s\n", srv->team_names[c->team_idx]);
    gui_broadcast(srv, line);
    printf("[zappy] l'equipe %s remporte la partie.\n",
        srv->team_names[c->team_idx]);
}
