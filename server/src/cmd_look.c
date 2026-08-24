/*
** ZAPPY - cmd_look.c
** La commande Look : ce que le drone voit devant lui.
**
** Le champ de vision est un triangle qui s'elargit avec le niveau :
**   niveau 1 : la case du joueur + 3 cases devant  = 4 cases
**   niveau 2 : + 5 cases sur la rangee suivante    = 9 cases
**   ...soit (niveau + 1)^2 cases au total.
**
** Les cases sont listees rangee par rangee, et dans chaque rangee DE
** GAUCHE A DROITE du point de vue du joueur (verifie en comparant la
** sortie du serveur de reference avec les bct de la carte).
**
** Format exact : "[" puis les cases separees par des virgules, chaque
** objet precede d'une espace, et " ]" a la fin. Une case vide ne produit
** donc rien du tout entre deux virgules :
**   [ player food, linemate,, deraumere ]
**
** On ecrit directement dans la file de sortie plutot que dans un tampon :
** au niveau 8 il y a 81 cases dont le contenu n'a pas de taille maximale,
** et cela evite d'avoir a deviner une taille suffisante.
*/

#include "server.h"

/* Ecrit le contenu d'une case : d'abord les joueurs, puis les ressources. */
static void look_tile(server_t *srv, client_t *c, int x, int y)
{
    tile_t *t = map_at(srv, x, y);
    int wx = map_wrap(x, srv->width);
    int wy = map_wrap(y, srv->height);

    for (int i = 0; i < MAX_CLIENTS; i++)
        if (srv->clients[i].fd != -1 && srv->clients[i].state == STATE_AI
            && srv->clients[i].x == wx && srv->clients[i].y == wy)
            queue_output(c, " player");
    for (int r = 0; r < NB_RESOURCES; r++)
        for (int k = 0; k < t->qty[r]; k++) {
            queue_output(c, " ");
            queue_output(c, resource_name(r));
        }
}

/*
** Parcourt le cone de vision.
** rangee r : les cases (joueur + r*devant + k*droite) pour k de -r a +r.
** Le vecteur "droite" est simplement le vecteur "devant" de l'orientation
** suivante, d'ou l'indice o % 4 + 1.
*/
void cmd_look(server_t *srv, client_t *c)
{
    static const int dx[5] = {0, 0, 1, 0, -1};
    static const int dy[5] = {0, -1, 0, 1, 0};
    int right = c->orientation % 4 + 1;
    int first = 1;

    queue_output(c, "[");
    for (int r = 0; r <= c->level; r++)
        for (int k = -r; k <= r; k++) {
            if (!first)
                queue_output(c, ",");
            first = 0;
            look_tile(srv, c, c->x + r * dx[c->orientation] + k * dx[right],
                c->y + r * dy[c->orientation] + k * dy[right]);
        }
    queue_output(c, " ]\n");
}
