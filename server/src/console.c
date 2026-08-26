/*
** ZAPPY - console.c
** Une petite console d'administration, lue sur l'entree standard.
**
** Elle ne fait pas partie du sujet : c'est un outil de mise au point. Une
** fois le serveur lance, on tape simplement une commande dans le terminal :
**
**   /noFood true    les drones ne digerent plus et ne meurent plus de faim
**   /noFood false   retour au fonctionnement normal
**   /help           rappel des commandes
**
** Utile pour developper une IA : on peut la laisser tourner des heures
** sans que ses drones meurent, et rebrancher la faim quand on veut
** verifier qu'elle sait se nourrir.
**
** L'entree standard rejoint le poll() comme n'importe quelle socket : le
** serveur continue donc de dormir tant que personne ne tape rien. Si elle
** est fermee ou redirigee depuis /dev/null (serveur lance par un script),
** on la retire de la surveillance des le premier EOF -- sans quoi poll()
** la signalerait sans arret et la boucle tournerait a vide.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "server.h"

void console_init(server_t *srv)
{
    srv->console_fd = STDIN_FILENO;
    srv->console_len = 0;
}

/* Applique une commande tapee au clavier. */
static void console_exec(server_t *srv, char *line)
{
    char *arg = line;

    while (*arg && *arg != ' ')
        arg++;
    while (*arg == ' ')
        *arg++ = '\0';
    if (strcmp(line, "/noFood") == 0 && strcmp(arg, "true") == 0)
        srv->no_food = 1;
    else if (strcmp(line, "/noFood") == 0 && strcmp(arg, "false") == 0)
        srv->no_food = 0;
    else if (line[0] != '\0') {
        printf("[zappy] commandes : /noFood true|false, /help\n");
        fflush(stdout);
        return;
    } else
        return;
    printf("[zappy] faim %s.\n",
        srv->no_food ? "desactivee : les drones sont immortels"
        : "reactivee : les drones digerent de nouveau");
    fflush(stdout);
}

/*
** Lit ce qui a ete tape et execute chaque ligne complete.
** Retourne -1 quand l'entree standard est fermee : l'appelant cesse alors
** de la surveiller.
*/
int console_read(server_t *srv)
{
    ssize_t n = read(srv->console_fd, srv->console_buf + srv->console_len,
        sizeof(srv->console_buf) - srv->console_len - 1);
    char *nl;
    size_t used;

    if (n <= 0)
        return -1;
    srv->console_len += n;
    srv->console_buf[srv->console_len] = '\0';
    while ((nl = memchr(srv->console_buf, '\n', srv->console_len)) != NULL) {
        *nl = '\0';
        if (nl > srv->console_buf && *(nl - 1) == '\r')
            *(nl - 1) = '\0';
        console_exec(srv, srv->console_buf);
        used = (nl - srv->console_buf) + 1;
        memmove(srv->console_buf, srv->console_buf + used,
            srv->console_len - used);
        srv->console_len -= used;
    }
    if (srv->console_len >= sizeof(srv->console_buf) - 1)
        srv->console_len = 0;          /* ligne trop longue : on l'oublie */
    return 0;
}
