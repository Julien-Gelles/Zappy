/*
** ZAPPY - gui_query.c
** Les commandes par lesquelles un GUI interroge un joueur precis.
**
**   ppo #n  -> position et orientation
**   plv #n  -> niveau
**   pin #n  -> inventaire
**
** Le numero demande peut ne correspondre a personne (joueur deconnecte,
** faute de frappe) : dans ce cas le protocole impose "sbp".
*/

#include <stdio.h>
#include <string.h>
#include "server.h"

/* Retrouve un joueur par le numero que voient les GUI, NULL si absent. */
static client_t *find_player(server_t *srv, int id)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (srv->clients[i].fd != -1 && srv->clients[i].state == STATE_AI
            && srv->clients[i].id == id)
            return &srv->clients[i];
    return NULL;
}

void gui_query(server_t *srv, client_t *c, const char *line)
{
    client_t *p;
    char out[128];
    int id = -1;

    if (strlen(line) < 3 || sscanf(line + 3, " #%d", &id) != 1) {
        queue_output(c, "sbp\n");
        return;
    }
    p = find_player(srv, id);
    if (p == NULL) {
        queue_output(c, "sbp\n");
        return;
    }
    if (strncmp(line, "ppo", 3) == 0)
        snprintf(out, sizeof(out), "ppo #%d %d %d %d\n",
            p->id, p->x, p->y, p->orientation);
    else if (strncmp(line, "plv", 3) == 0)
        snprintf(out, sizeof(out), "plv #%d %d\n", p->id, p->level);
    else if (strncmp(line, "pin", 3) == 0)
        snprintf(out, sizeof(out), "pin #%d %d %d %d %d %d %d %d %d %d\n",
            p->id, p->x, p->y, p->inventory[0], p->inventory[1],
            p->inventory[2], p->inventory[3], p->inventory[4],
            p->inventory[5], p->inventory[6]);
    else {
        queue_output(c, "suc\n");
        return;
    }
    queue_output(c, out);
}
