/*
** ZAPPY - egg.c
** Les oeufs : ce sont eux, et non un simple compteur, qui donnent les
** places disponibles dans une equipe.
**
** Au demarrage, chaque equipe recoit -c oeufs poses au hasard sur la
** carte. Quand une IA se connecte, elle CONSOMME le plus ancien oeuf de
** son equipe et apparait a l'endroit exact ou il se trouvait. La commande
** Fork en pond un nouveau, ce qui cree une place supplementaire.
**
** Consequence importante, verifiee sur le serveur de reference : la mort
** d'un drone ne rend PAS sa place. L'oeuf a ete consomme une fois pour
** toutes ; sans Fork, une equipe ne peut jamais depasser ses -c drones au
** total sur la duree de la partie.
**
** Connect_nbr repond simplement le nombre d'oeufs restants de l'equipe.
*/

#include <stdio.h>
#include <stdlib.h>
#include "server.h"

/* Agrandit le tableau d'oeufs quand il est plein (on double a chaque fois). */
static int eggs_grow(server_t *srv)
{
    int cap = (srv->cap_eggs == 0) ? 16 : srv->cap_eggs * 2;
    egg_t *tmp = realloc(srv->eggs, cap * sizeof(egg_t));

    if (tmp == NULL)
        return -1;
    srv->eggs = tmp;
    srv->cap_eggs = cap;
    return 0;
}

/*
** Pose un oeuf et previent les GUI.
** parent_id vaut -1 pour les oeufs du debut de partie, qui n'ont ete
** pondus par personne : c'est la convention du protocole.
*/
int egg_add(server_t *srv, int team_idx, int parent_id, int x, int y)
{
    char line[80];
    egg_t *e;

    if (srv->nb_eggs >= srv->cap_eggs && eggs_grow(srv) == -1)
        return -1;
    e = &srv->eggs[srv->nb_eggs];
    e->id = srv->next_egg_id++;
    e->team_idx = team_idx;
    e->parent_id = parent_id;
    e->x = x;
    e->y = y;
    srv->nb_eggs++;
    snprintf(line, sizeof(line), "enw #%d #%d %d %d\n", e->id, parent_id, x, y);
    gui_broadcast(srv, line);
    return e->id;
}

/* Nombre d'oeufs encore disponibles pour une equipe. */
int egg_count(server_t *srv, int team_idx)
{
    int n = 0;

    for (int i = 0; i < srv->nb_eggs; i++)
        if (srv->eggs[i].team_idx == team_idx)
            n++;
    return n;
}

/*
** Consomme le plus ancien oeuf de l'equipe et renvoie son numero, ou -1
** s'il n'y en a plus. La position de l'oeuf est rendue dans x et y : le
** nouveau drone naitra la.
*/
int egg_take(server_t *srv, int team_idx, int *x, int *y)
{
    int id;

    for (int i = 0; i < srv->nb_eggs; i++) {
        if (srv->eggs[i].team_idx != team_idx)
            continue;
        id = srv->eggs[i].id;
        *x = srv->eggs[i].x;
        *y = srv->eggs[i].y;
        for (int k = i + 1; k < srv->nb_eggs; k++)
            srv->eggs[k - 1] = srv->eggs[k];
        srv->nb_eggs--;
        return id;
    }
    return -1;
}

/* Les -c oeufs de depart de chaque equipe, disperses au hasard. */
int eggs_init(server_t *srv)
{
    for (int t = 0; t < srv->nb_teams; t++)
        for (int i = 0; i < srv->clients_nb; i++)
            if (egg_add(srv, t, -1, rand() % srv->width,
                rand() % srv->height) == -1)
                return -1;
    return 0;
}
