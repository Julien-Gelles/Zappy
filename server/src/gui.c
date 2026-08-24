/*
** ZAPPY - gui.c
** Le petit bout de protocole GUI qui concerne la carte.
**
** Trois commandes seulement pour l'instant :
**   msz          -> taille de la carte
**   bct X Y      -> contenu d'une case
**   mct          -> contenu de TOUTES les cases
** Les autres (tna, ppo, pin, sgt...) viendront avec les joueurs.
**
** Reponses d'erreur normalisees par le protocole :
**   sbp = mauvais parametres, suc = commande inconnue.
*/

#include <stdio.h>
#include <string.h>
#include "server.h"

/* Envoie "bct X Y food linemate deraumere sibur mendiane phiras thystame". */
void gui_send_bct(server_t *srv, client_t *c, int x, int y)
{
    tile_t *t = map_at(srv, x, y);
    char line[128];

    snprintf(line, sizeof(line), "bct %d %d %d %d %d %d %d %d %d\n",
        x, y, t->qty[RES_FOOD], t->qty[RES_LINEMATE], t->qty[RES_DERAUMERE],
        t->qty[RES_SIBUR], t->qty[RES_MENDIANE], t->qty[RES_PHIRAS],
        t->qty[RES_THYSTAME]);
    queue_output(c, line);
}

/* mct : un bct pour chaque case, ligne par ligne. */
void gui_send_mct(server_t *srv, client_t *c)
{
    for (int y = 0; y < srv->height; y++)
        for (int x = 0; x < srv->width; x++)
            gui_send_bct(srv, c, x, y);
}

/* bct X Y : on refuse les coordonnees hors carte (le GUI ne doit pas deviner). */
static void gui_bct(server_t *srv, client_t *c, const char *line)
{
    int x = 0;
    int y = 0;

    if (sscanf(line, "bct %d %d", &x, &y) != 2
        || x < 0 || x >= srv->width || y < 0 || y >= srv->height) {
        queue_output(c, "sbp\n");
        return;
    }
    gui_send_bct(srv, c, x, y);
}

void gui_command(server_t *srv, client_t *c, const char *line)
{
    char msz[64];

    if (strcmp(line, "msz") == 0) {
        snprintf(msz, sizeof(msz), "msz %d %d\n", srv->width, srv->height);
        queue_output(c, msz);
    } else if (strcmp(line, "mct") == 0)
        gui_send_mct(srv, c);
    else if (strncmp(line, "bct", 3) == 0)
        gui_bct(srv, c, line);
    else
        queue_output(c, "suc\n");
    /* TODO: tna (equipes), sgt/sst (time unit), ppo/plv/pin (joueurs). */
}
