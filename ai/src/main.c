/*
** ZAPPY - zappy_ai (STUB)
** USAGE: ./zappy_ai -p port -n name -h machine
**
** A COMPLETER : ouvrir une socket TCP vers le serveur, faire le handshake
** (lire WELCOME, envoyer le nom d'équipe, lire CLIENT-NUM puis "X Y"), puis
** boucler : décider d'une action, l'envoyer, lire la réponse. Langage libre.
*/

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        printf("USAGE: ./zappy_ai -p port -n name -h machine\n"
            "\t-p port\t\tport number\n"
            "\t-n name\t\tteam name\n"
            "\t-h machine\thostname (localhost par defaut)\n");
        return 0;
    }
    fprintf(stderr, "[zappy_ai] stub - a implementer.\n");
    (void)argc;
    (void)argv;
    return 0;
}
