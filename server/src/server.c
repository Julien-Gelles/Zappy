/*
** ZAPPY - server.c
** Création de la socket d'écoute + boucle principale basée sur poll().
**
** Principe demandé par le sujet :
**   - un seul process, un seul thread
**   - poll() ne se réveille QUE s'il se passe quelque chose sur une socket
**     (ou, plus tard, si un événement de jeu est prêt à être exécuté -> timeout)
**   - PAS d'attente active
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "server.h"

/* Rend une socket non bloquante (obligatoire avec poll pour ne jamais bloquer). */
static int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int server_init(server_t *srv)
{
    struct sockaddr_in addr;
    int opt = 1;

    for (int i = 0; i < MAX_CLIENTS; i++)
        srv->clients[i].fd = -1;
    srv->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (srv->listen_fd == -1)
        return (perror("socket"), -1);
    setsockopt(srv->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(srv->port);
    if (bind(srv->listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        return (perror("bind"), -1);
    if (listen(srv->listen_fd, 128) == -1)
        return (perror("listen"), -1);
    if (set_nonblocking(srv->listen_fd) == -1)
        return (perror("fcntl"), -1);
    /* TODO: allouer la carte (width*height) et générer les ressources ici. */
    return 0;
}

/*
** Construit le tableau de pollfd à partir de la socket d'écoute + des clients.
** On demande POLLIN toujours, et POLLOUT seulement si on a des octets à envoyer.
** Retourne le nombre de pollfd remplis, et remplit map[] pour retrouver le client.
*/
static int build_pollfds(server_t *srv, struct pollfd *pfds, client_t **map)
{
    int n = 0;

    pfds[n].fd = srv->listen_fd;
    pfds[n].events = POLLIN;
    map[n] = NULL;
    n++;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (srv->clients[i].fd == -1)
            continue;
        pfds[n].fd = srv->clients[i].fd;
        pfds[n].events = POLLIN;
        if (srv->clients[i].write_len > 0)
            pfds[n].events |= POLLOUT;
        map[n] = &srv->clients[i];
        n++;
    }
    return n;
}

int server_run(server_t *srv)
{
    struct pollfd pfds[MAX_CLIENTS + 1];
    client_t *map[MAX_CLIENTS + 1];
    int nfds;
    int ready;

    while (1) {
        nfds = build_pollfds(srv, pfds, map);
        /* TODO: calculer le timeout = temps avant le prochain événement de jeu.
        ** Pour l'instant -1 = on dort jusqu'à activité réseau. */
        ready = poll(pfds, nfds, -1);
        if (ready == -1) {
            if (errno == EINTR)
                continue;
            return (perror("poll"), -1);
        }
        /* TODO: ici, exécuter les événements de jeu arrivés à échéance
        ** (fin d'un Forward, d'une Incantation, respawn ressources, faim...). */
        for (int i = 0; i < nfds; i++) {
            if (pfds[i].fd == srv->listen_fd && (pfds[i].revents & POLLIN)) {
                accept_client(srv);
                continue;
            }
            if (map[i] == NULL)
                continue;
            if (pfds[i].revents & POLLOUT)
                flush_output(map[i]);
            if (pfds[i].revents & (POLLIN | POLLHUP))
                handle_client_read(srv, map[i]);
        }
    }
    return 0;
}

void server_cleanup(server_t *srv)
{
    if (srv->listen_fd > 0)
        close(srv->listen_fd);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (srv->clients[i].fd != -1)
            close(srv->clients[i].fd);
        free(srv->clients[i].write_buf);
        free(srv->clients[i].team_name);
    }
    free(srv->team_names);
    free(srv->team_used);
}
