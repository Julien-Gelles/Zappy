/*
** ZAPPY - handshake.c
** Le premier message d'un client est son nom d'equipe : c'est lui qui
** decide de ce que le client devient.
**
**   "GRAPHIC"        -> un spectateur : il recoit l'etat du monde
**   un nom d'equipe  -> un drone : il recoit ses places restantes et la
**                       taille de la carte, puis apparait sur la carte
**   autre chose      -> "ko", la connexion ne sert a rien
*/

#include <stdio.h>
#include <string.h>
#include "server.h"

/* Un GUI vient de se connecter : on lui pousse l'etat du monde. */
static void welcome_gui(server_t *srv, client_t *c)
{
    char line[64];

    c->state = STATE_GUI;
    snprintf(line, sizeof(line), "msz %d %d\n", srv->width, srv->height);
    queue_output(c, line);
    snprintf(line, sizeof(line), "sgt %d\n", srv->freq);
    queue_output(c, line);
    gui_send_mct(srv, c);
    /* TODO: envoyer aussi tna (equipes) et la liste des joueurs deja la. */
}

/*
** Tente d'inscrire le client dans l'equipe demandee.
** Retourne 0 si l'equipe n'existe pas ou n'a plus de place.
*/
static int join_team(server_t *srv, client_t *c, const char *name)
{
    char line[64];

    for (int t = 0; t < srv->nb_teams; t++) {
        if (strcmp(name, srv->team_names[t]) != 0
            || srv->team_used[t] >= srv->clients_nb)
            continue;
        srv->team_used[t]++;
        c->state = STATE_AI;
        c->team_idx = t;
        c->team_name = strdup(name);
        snprintf(line, sizeof(line), "%d\n",
            srv->clients_nb - srv->team_used[t]);
        queue_output(c, line);
        snprintf(line, sizeof(line), "%d %d\n", srv->width, srv->height);
        queue_output(c, line);
        player_spawn(srv, c);
        return 1;
    }
    return 0;
}

void handle_team_name(server_t *srv, client_t *c, const char *name)
{
    if (strcmp(name, GRAPHIC_TEAM) == 0)
        return welcome_gui(srv, c);
    if (join_team(srv, c, name) == 0)
        queue_output(c, "ko\n");
}
