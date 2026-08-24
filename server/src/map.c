/*
** ZAPPY - map.c
** Allocation de la carte et acces aux cases en geometrie TORIQUE.
**
** Le monde n'a pas de bord : sortir par la droite fait revenir a gauche,
** sortir par le haut fait revenir en bas. Toute la logique du tore tient
** dans map_wrap() : les autres fichiers n'ont jamais a s'en soucier, il
** leur suffit d'appeler map_at() avec des coordonnees eventuellement
** hors limites (negatives ou trop grandes).
*/

#include <stdlib.h>
#include "server.h"

/*
** Ramene une coordonnee dans [0, max[ en enroulant le monde sur lui-meme.
** En C, l'operateur % garde le signe du dividende : -1 % 10 vaut -1 et non 9.
** On rattrape donc le cas negatif en ajoutant max une fois.
*/
int map_wrap(int value, int max)
{
    int m = value % max;

    if (m < 0)
        m += max;
    return m;
}

/*
** Retourne la case (x, y), avec enroulement automatique.
** La carte est stockee a plat : la case (x, y) est a l'indice y * width + x.
*/
tile_t *map_at(server_t *srv, int x, int y)
{
    int wx = map_wrap(x, srv->width);
    int wy = map_wrap(y, srv->height);

    return &srv->map[wy * srv->width + wx];
}

/*
** Alloue la carte (calloc -> toutes les cases demarrent vides) puis y
** repartit les ressources initiales.
*/
int map_init(server_t *srv)
{
    size_t tiles = (size_t)srv->width * (size_t)srv->height;

    srv->map = calloc(tiles, sizeof(tile_t));
    if (srv->map == NULL)
        return -1;
    map_spawn_resources(srv);
    return 0;
}

void map_destroy(server_t *srv)
{
    free(srv->map);
    srv->map = NULL;
}
