/*
** ZAPPY - incant_rules.c
** Les conditions d'une elevation.
**
** Pour monter d'un niveau, il faut reunir sur UNE MEME CASE :
**   - un certain nombre de drones du meme niveau,
**   - un certain nombre de pierres.
**
** Deux subtilites du sujet, faciles a manquer :
**   - les participants n'ont PAS besoin d'etre de la meme equipe, seul
**     leur niveau compte : un ennemi de passage monte de niveau avec vous ;
**   - les conditions sont verifiees au DEBUT et a la FIN du rituel. Il
**     suffit qu'elles ne soient plus reunies a la fin pour que tout echoue,
**     et c'est ce qui rend l'Eject si redoutable.
**
** La table ci-dessous est celle du sujet. Note : le serveur de reference
** s'en ecarte -- il accepte le palier 2->3 sans sibur. On suit le sujet.
*/

#include "server.h"

/*
** Une ligne par palier, de 1->2 a 7->8.
** Colonne 0 : nombre de drones. Colonnes 1 a 6 : quantite de chaque
** pierre, dans l'ordre de l'enum resource_t (linemate ... thystame).
*/
static const int NEED[MAX_LEVEL - 1][NB_RESOURCES] = {
    /* joueurs, linemate, deraumere, sibur, mendiane, phiras, thystame */
    {1,         1,        0,         0,     0,        0,      0},
    {2,         1,        1,         1,     0,        0,      0},
    {2,         2,        0,         1,     0,        2,      0},
    {4,         1,        1,         2,     0,        1,      0},
    {4,         1,        2,         1,     3,        0,      0},
    {6,         1,        2,         3,     0,        1,      0},
    {6,         2,        2,         2,     2,        2,      1}
};

/* Nombre de drones requis pour quitter ce niveau. */
int incant_players(int level)
{
    if (level < 1 || level >= MAX_LEVEL)
        return -1;
    return NEED[level - 1][0];
}

/* Quantite requise d'une pierre (res va de RES_LINEMATE a RES_THYSTAME). */
int incant_need(int level, int res)
{
    if (level < 1 || level >= MAX_LEVEL || res < 1 || res >= NB_RESOURCES)
        return 0;
    return NEED[level - 1][res];
}

/* Combien de drones de ce niveau se trouvent sur cette case, toutes equipes. */
int incant_count(server_t *srv, int x, int y, int level)
{
    client_t *p;
    int n = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        p = &srv->clients[i];
        if (p->fd != -1 && p->state == STATE_AI
            && p->x == x && p->y == y && p->level == level)
            n++;
    }
    return n;
}

/* Les pierres necessaires sont-elles bien sur la case ? */
static int stones_ready(server_t *srv, int x, int y, int level)
{
    tile_t *t = map_at(srv, x, y);

    for (int r = RES_LINEMATE; r < NB_RESOURCES; r++)
        if (t->qty[r] < incant_need(level, r))
            return 0;
    return 1;
}

/* Toutes les conditions sont-elles reunies ici et maintenant ? */
int incant_ready(server_t *srv, int x, int y, int level)
{
    if (level < 1 || level >= MAX_LEVEL)
        return 0;
    if (incant_count(srv, x, y, level) < incant_players(level))
        return 0;
    return stones_ready(srv, x, y, level);
}
