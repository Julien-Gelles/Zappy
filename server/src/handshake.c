/*
** ZAPPY - handshake.c
** Le premier message d'un client est son nom d'equipe : c'est lui qui
** decide de ce que le client devient.
**
**   "GRAPHIC"        -> un spectateur : il recoit tout l'etat du monde
**   un nom d'equipe  -> un drone, s'il reste un oeuf disponible
**   autre chose      -> "ko", la connexion ne sert a rien
**
** Rejoindre une equipe consomme un oeuf : le drone nait exactement la ou
** cet oeuf se trouvait. Sans oeuf disponible, la reponse est "ko" meme si
** l'equipe existe.
*/

#include <stdio.h>
#include <string.h>
#include "server.h"

/*
** Tente d'inscrire le client dans l'equipe demandee.
** Retourne 0 si l'equipe n'existe pas ou n'a plus d'oeuf.
*/
static int join_team(server_t *srv, client_t *c, const char *name)
{
    char line[64];
    int x = 0;
    int y = 0;
    int egg;

    for (int t = 0; t < srv->nb_teams; t++) {
        if (strcmp(name, srv->team_names[t]) != 0)
            continue;
        egg = egg_take(srv, t, &x, &y);
        if (egg == -1)
            return 0;
        c->state = STATE_AI;
        c->team_idx = t;
        c->team_name = strdup(name);
        snprintf(line, sizeof(line), "%d\n", egg_count(srv, t));
        queue_output(c, line);
        snprintf(line, sizeof(line), "%d %d\n", srv->width, srv->height);
        queue_output(c, line);
        player_spawn(srv, c, x, y);
        snprintf(line, sizeof(line), "ebo #%d\n", egg);
        gui_broadcast(srv, line);
        return 1;
    }
    return 0;
}

void handle_team_name(server_t *srv, client_t *c, const char *name)
{
    if (strcmp(name, GRAPHIC_TEAM) == 0) {
        c->state = STATE_GUI;
        return gui_send_state(srv, c);
    }
    if (join_team(srv, c, name) == 0)
        queue_output(c, "ko\n");
}
