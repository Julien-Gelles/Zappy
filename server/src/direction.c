/*
** ZAPPY - direction.c
** "D'ou vient ce qui m'arrive ?" : le calcul partage par Broadcast et Eject.
**
** Le resultat est un numero de 1 a 8, exprime DANS LE REPERE DE CELUI QUI
** RECOIT (0 = sur ma propre case) :
**
** Vu du recepteur, le haut du schema est la direction qu'il regarde.
** Les numeros tournent dans le sens antihoraire en partant de l'avant :
**
**            2   1   8          1 devant       5 derriere
**              \ | /            3 a gauche     7 a droite
**            3 - o - 7          0 = sur ma propre case
**              / | \
**            4   5   6
**
** COMMENT ON TROUVE CE NUMERO
**
** On place d'abord la source dans le repere du recepteur : f cases devant
** lui, r cases a sa droite (le monde etant un tore, on prend le chemin le
** plus court). Puis on raisonne par ANNEAUX CARRES : la source est sur
** l'anneau n = max(|r|, |f|), qui compte 8n cases. On numerote sa position
** le long de cet anneau en partant de la case droit devant.
**
** Les 8 secteurs ne sont pas de meme taille : les 4 directions "droites"
** (devant, derriere, gauche, droite) occupent chacune un TIERS d'anneau
** (2n/3 cases), les 4 diagonales DEUX tiers (4n/3 cases). Un decoupage en
** angles egaux de 45 degres donnerait un resultat different.
**
** Regles relevees sur le serveur de reference en amenant un emetteur
** exactement sur chaque position voulue autour d'un auditeur immobile,
** sur les anneaux 1 a 6.
*/

#include "server.h"

/*
** Ecart le plus court entre deux coordonnees sur un axe qui boucle.
** Sur une carte de 10, aller de 1 a 9 fait -2 (par le bord) et non +8.
*/
static int shortest_delta(int from, int to, int size)
{
    int d = map_wrap(to - from, size);

    if (d > size / 2)
        d -= size;
    return d;
}

/*
** Position de la case (r, f) le long de son anneau, en tournant dans le
** sens des aiguilles d'une montre depuis la case droit devant (0, n).
** Le resultat est ramene dans ]-4n, 4n] : negatif = du cote gauche.
*/
static int ring_index(int r, int f, int n)
{
    int p;

    if (f == n)
        p = (r >= 0) ? r : 8 * n + r;
    else if (r == n)
        p = 2 * n - f;
    else if (f == -n)
        p = 4 * n - r;
    else
        p = 6 * n + f;
    return (p > 4 * n) ? p - 8 * n : p;
}

/*
** Secteur correspondant a une position sur l'anneau.
** On compare en tiers d'anneau (d'ou le x3) : les bornes tombent a 1, 5,
** 7 et 11 tiers, ce qui donne bien 2n/3 pour les axes et 4n/3 pour les
** diagonales. Le cote gauche utilise des comparaisons strictes : c'est ce
** demi-decalage qui reproduit l'asymetrie de la reference.
*/
static int ring_sector(int p, int n)
{
    static const int bound[4] = {1, 5, 7, 11};
    static const int side[2][4] = {{1, 8, 7, 6}, {1, 2, 3, 4}};
    int neg = (p < 0);
    int v = 3 * (neg ? -p : p);

    for (int i = 0; i < 4; i++)
        if (neg ? (v < bound[i] * n) : (v <= bound[i] * n))
            return side[neg][i];
    return 5;
}

/*
** Numero de la direction sous laquelle "to" percoit la case (sx, sy).
** Sert au son d'un Broadcast comme a la poussee d'un Eject.
*/
int direction_from(server_t *srv, client_t *to, int sx, int sy)
{
    static const int dx[5] = {0, 0, 1, 0, -1};
    static const int dy[5] = {0, -1, 0, 1, 0};
    int ex = shortest_delta(to->x, sx, srv->width);
    int ey = shortest_delta(to->y, sy, srv->height);
    int right = to->orientation % 4 + 1;
    int f = ex * dx[to->orientation] + ey * dy[to->orientation];
    int r = ex * dx[right] + ey * dy[right];
    int n = (r < 0 ? -r : r) > (f < 0 ? -f : f)
        ? (r < 0 ? -r : r) : (f < 0 ? -f : f);

    if (n == 0)
        return 0;
    return ring_sector(ring_index(r, f, n), n);
}
