/*
** ZAPPY - args.c
** Parsing minimal des arguments de la ligne de commande.
** -p port  -x width  -y height  -n name1 name2 ...  -c clientsNb  -f freq
**
** NB: parsing volontairement simple pour démarrer. A durcir plus tard
** (valeurs négatives, doublons, options manquantes, etc.).
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "server.h"

static int parse_teams(server_t *srv, int argc, char **argv, int *i)
{
    int start = *i + 1;
    int count = 0;

    while (start + count < argc && argv[start + count][0] != '-')
        count++;
    if (count == 0)
        return -1;
    srv->team_names = calloc(count, sizeof(char *));
    srv->team_used = calloc(count, sizeof(int));
    if (!srv->team_names || !srv->team_used)
        return -1;
    for (int k = 0; k < count; k++)
        srv->team_names[k] = argv[start + k];
    srv->nb_teams = count;
    *i = start + count - 1;
    return 0;
}

int parse_args(server_t *srv, int argc, char **argv)
{
    /* Valeurs par défaut : utilisées si l'option n'est pas passée. */
    srv->freq = 100;
    srv->width = 20;
    srv->height = 20;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc)
            srv->port = atoi(argv[++i]);
        else if (strcmp(argv[i], "-x") == 0 && i + 1 < argc)
            srv->width = atoi(argv[++i]);
        else if (strcmp(argv[i], "-y") == 0 && i + 1 < argc)
            srv->height = atoi(argv[++i]);
        else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc)
            srv->clients_nb = atoi(argv[++i]);
        else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
            srv->freq = atoi(argv[++i]);
        else if (strcmp(argv[i], "-n") == 0) {
            if (parse_teams(srv, argc, argv, &i) != 0)
                return -1;
        } else
            return -1;
    }
    if (srv->port <= 0 || srv->width <= 0 || srv->height <= 0
        || srv->clients_nb <= 0 || srv->nb_teams <= 0 || srv->freq <= 0) {
        fprintf(stderr, "[zappy] arguments invalides ou manquants.\n");
        return -1;
    }
    return 0;
}
