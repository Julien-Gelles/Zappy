/*
** ZAPPY - cmd_inventory.c
** Inventory, Take et Set : ce que le drone porte et ce qu'il ramasse.
**
** Format de l'inventaire, releve sur le serveur de reference :
**   [ food 9, linemate 0, deraumere 0, sibur 0, mendiane 0, phiras 0,
**     thystame 0 ]
** soit "[ " puis les couples separes par ", " et " ]" a la fin.
**
** Take et Set repondent "ok" ou "ko" et previennent les GUI :
**   pgt #n i  quand le joueur ramasse la ressource i
**   pdr #n i  quand il la pose
** suivis dans les deux cas de pin (inventaire) et bct (etat de la case).
*/

#include <stdio.h>
#include <string.h>
#include "server.h"

/* Numero de la ressource portant ce nom, ou -1 si le nom est inconnu. */
static int resource_index(const char *name)
{
    for (int r = 0; r < NB_RESOURCES; r++)
        if (strcmp(name, resource_name(r)) == 0)
            return r;
    return -1;
}

/* Previent tous les GUI que cette case a change. */
static void gui_notify_tile(server_t *srv, int x, int y)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (srv->clients[i].fd != -1 && srv->clients[i].state == STATE_GUI)
            gui_send_bct(srv, &srv->clients[i], x, y);
}

void cmd_inventory(server_t *srv, client_t *c)
{
    char line[256];

    (void)srv;
    snprintf(line, sizeof(line),
        "[ food %d, linemate %d, deraumere %d, sibur %d, mendiane %d, "
        "phiras %d, thystame %d ]\n",
        c->inventory[RES_FOOD], c->inventory[RES_LINEMATE],
        c->inventory[RES_DERAUMERE], c->inventory[RES_SIBUR],
        c->inventory[RES_MENDIANE], c->inventory[RES_PHIRAS],
        c->inventory[RES_THYSTAME]);
    queue_output(c, line);
}

/* Ramasse une unite sur la case courante, si elle s'y trouve. */
void cmd_take(server_t *srv, client_t *c, const char *name)
{
    int r = resource_index(name);
    tile_t *t = map_at(srv, c->x, c->y);
    char line[32];

    if (r == -1 || t->qty[r] <= 0) {
        queue_output(c, "ko\n");
        return;
    }
    t->qty[r]--;
    c->inventory[r]++;
    queue_output(c, "ok\n");
    snprintf(line, sizeof(line), "pgt #%d %d\n", c->id, r);
    gui_broadcast(srv, line);
    gui_notify_pin(srv, c);
    gui_notify_tile(srv, c->x, c->y);
}

/* Depose une unite sur la case courante, si le joueur en porte. */
void cmd_set(server_t *srv, client_t *c, const char *name)
{
    int r = resource_index(name);
    tile_t *t = map_at(srv, c->x, c->y);
    char line[32];

    if (r == -1 || c->inventory[r] <= 0) {
        queue_output(c, "ko\n");
        return;
    }
    c->inventory[r]--;
    t->qty[r]++;
    queue_output(c, "ok\n");
    snprintf(line, sizeof(line), "pdr #%d %d\n", c->id, r);
    gui_broadcast(srv, line);
    gui_notify_pin(srv, c);
    gui_notify_tile(srv, c->x, c->y);
}
