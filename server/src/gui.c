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

/*
** Renvoie l'etat de la carte a TOUS les GUI connectes.
** Utilise apres une reapparition de ressources, pour qu'ils se remettent
** a jour sans avoir a redemander.
*/
void gui_broadcast_mct(server_t *srv)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (srv->clients[i].fd != -1 && srv->clients[i].state == STATE_GUI)
            gui_send_mct(srv, &srv->clients[i]);
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
    char buf[64];

    if (strcmp(line, "msz") == 0) {
        snprintf(buf, sizeof(buf), "msz %d %d\n", srv->width, srv->height);
        queue_output(c, buf);
    } else if (strcmp(line, "sgt") == 0) {
        snprintf(buf, sizeof(buf), "sgt %d\n", srv->freq);
        queue_output(c, buf);
    } else if (strcmp(line, "mct") == 0)
        gui_send_mct(srv, c);
    else if (strncmp(line, "bct", 3) == 0)
        gui_bct(srv, c, line);
    else if (line[0] == 'p')
        gui_query(srv, c, line);
    else
        queue_output(c, "suc\n");
    /* TODO: sst (changer l'unite de temps), tna (equipes). */
}
