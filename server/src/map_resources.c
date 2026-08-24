/*
** ZAPPY - map_resources.c
** Repartition des ressources sur la carte.
**
** Regle du sujet : pour chaque ressource, la carte doit contenir
**   quantite = width * height * densite unites,
**   dispersees au hasard. Une case peut en accumuler plusieurs.
**
** map_spawn_resources() ne "remet pas a zero" : elle complete ce qui
** manque par rapport a la quantite visee. Elle sert donc aussi bien a la
** generation initiale (la carte est vide, tout est a poser) qu'a la
** reapparition periodique, quand l'horloge de jeu l'appellera.
*/

#include <stdlib.h>
#include "server.h"

/*
** Densite de chaque ressource, exprimee en POUR-CENT afin de rester en
** arithmetique entiere : 0.05 s'ecrit 5. Cela evite les surprises des
** nombres flottants.
** L'ordre suit celui de l'enum resource_t.
*/
static const int DENSITY_PCT[NB_RESOURCES] = {
    50,   /* food      0.50 */
    30,   /* linemate  0.30 */
    15,   /* deraumere 0.15 */
    10,   /* sibur     0.10 */
    10,   /* mendiane  0.10 */
    8,    /* phiras    0.08 */
    5     /* thystame  0.05 */
};

/*
** Quantite visee sur l'ensemble de la carte pour une ressource,
** arrondie a l'entier le plus proche.
*/
int map_target_qty(server_t *srv, int res)
{
    return (srv->width * srv->height * DENSITY_PCT[res] + 50) / 100;
}

/*
** Nom d'une ressource, tel qu'il apparait dans le protocole
** (Look, Take, Set). L'ordre suit celui de l'enum resource_t.
*/
const char *resource_name(int res)
{
    static const char *names[NB_RESOURCES] = {
        "food", "linemate", "deraumere", "sibur",
        "mendiane", "phiras", "thystame"
    };

    return names[res];
}

/* Compte les unites d'une ressource actuellement presentes sur la carte. */
static int count_resource(server_t *srv, int res)
{
    int tiles = srv->width * srv->height;
    int total = 0;

    for (int i = 0; i < tiles; i++)
        total += srv->map[i].qty[res];
    return total;
}

/* Depose une unite d'une ressource sur une case tiree au hasard. */
static void place_one(server_t *srv, int res)
{
    int x = rand() % srv->width;
    int y = rand() % srv->height;

    map_at(srv, x, y)->qty[res]++;
}

/*
** Complete la carte jusqu'a la quantite visee, ressource par ressource.
** Retourne le nombre total d'unites ajoutees : 0 signifie que rien n'a
** bouge, ce qui permet a l'appelant de ne prevenir les GUI que si besoin.
*/
int map_spawn_resources(server_t *srv)
{
    int missing;
    int added = 0;

    for (int res = 0; res < NB_RESOURCES; res++) {
        missing = map_target_qty(srv, res) - count_resource(srv, res);
        for (int i = 0; i < missing; i++)
            place_one(srv, res);
        if (missing > 0)
            added += missing;
    }
    return added;
}
