/*
** ZAPPY - main.c
** Point d'entrée du serveur : parse les arguments, initialise, lance la boucle.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "server.h"

static void usage(const char *bin)
{
    fprintf(stderr,
        "USAGE: %s -p port -x width -y height -n name1 name2 ... "
        "-c clientsNb -f freq\n"
        "\t-p port\t\tport number\n"
        "\t-x width\tworld width (defaut 20)\n"
        "\t-y height\tworld height (defaut 20)\n"
        "\t-n names\tteam names (au moins une)\n"
        "\t-c clientsNb\tnb of clients per team\n"
        "\t-f freq\t\treciprocal of time unit (defaut 10)\n",
        bin);
}

int main(int argc, char **argv)
{
    server_t srv;

    memset(&srv, 0, sizeof(srv));
    srand(time(NULL));
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        usage(argv[0]);
        return 0;
    }
    if (parse_args(&srv, argc, argv) != 0) {
        usage(argv[0]);
        return 84;
    }
    if (server_init(&srv) != 0) {
        server_cleanup(&srv);
        return 84;
    }
    printf("[zappy] serveur en ecoute sur le port %d (%dx%d, freq=%d)\n",
        srv.port, srv.width, srv.height, srv.freq);
    server_run(&srv);
    server_cleanup(&srv);
    return 0;
}
