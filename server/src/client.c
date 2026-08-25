/*
** ZAPPY - client.c
** Gestion d'un client : acceptation, buffering entrée/sortie, handshake,
** et aiguillage des commandes.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "server.h"

/* Trouve un slot client libre. Renvoie NULL si le serveur est plein. */
static client_t *find_free_slot(server_t *srv)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (srv->clients[i].fd == -1)
            return &srv->clients[i];
    return NULL;
}

int accept_client(server_t *srv)
{
    int fd = accept(srv->listen_fd, NULL, NULL);
    client_t *c;

    if (fd == -1)
        return -1;
    fcntl(fd, F_SETFL, O_NONBLOCK);
    c = find_free_slot(srv);
    if (!c) {
        close(fd);
        return -1;
    }
    memset(c, 0, sizeof(*c));
    c->fd = fd;
    c->state = STATE_WAIT_TEAM;
    /* Handshake : le serveur parle en premier. */
    queue_output(c, "WELCOME\n");
    return 0;
}

void remove_client(server_t *srv, client_t *c)
{
    char line[32];

    if (c->state == STATE_AI) {
        /* La place ne revient PAS : l'œuf a été consommé pour de bon.
        ** Seul un Fork peut en recréer une. */
        snprintf(line, sizeof(line), "pdi #%d\n", c->id);
        gui_broadcast(srv, line);
    }
    if (c->fd != -1)
        close(c->fd);
    free(c->write_buf);
    free(c->team_name);
    memset(c, 0, sizeof(*c));
    c->fd = -1;
}

/* Ajoute msg à la file de sortie (agrandie dynamiquement). */
int queue_output(client_t *c, const char *msg)
{
    size_t len = strlen(msg);
    char *tmp = realloc(c->write_buf, c->write_len + len);

    if (!tmp)
        return -1;
    c->write_buf = tmp;
    memcpy(c->write_buf + c->write_len, msg, len);
    c->write_len += len;
    return 0;
}

/* Envoie ce qui est en attente (write non bloquant, on gère les envois partiels). */
int flush_output(client_t *c)
{
    ssize_t n;

    if (c->write_len == 0)
        return 0;
    n = write(c->fd, c->write_buf, c->write_len);
    if (n <= 0)
        return -1;
    memmove(c->write_buf, c->write_buf + n, c->write_len - n);
    c->write_len -= n;
    return 0;
}

/*
** Traite UNE ligne complète (sans le '\n').
** Selon l'état du client, c'est soit le nom d'équipe, soit une commande.
*/
void process_line(server_t *srv, client_t *c, char *line)
{
    if (c->state == STATE_WAIT_TEAM) {
        handle_team_name(srv, c, line);
        return;
    }
    if (c->state == STATE_GUI) {
        gui_command(srv, c, line);
        return;
    }
    /* STATE_AI : une commande de drone. On ne l'exécute pas maintenant :
    ** elle coûte du temps, donc elle part en file d'attente. */
    action_enqueue(srv, c, line);
}

/*
** Lit les octets disponibles, en extrait les lignes complètes (séparées par \n)
** et les fait traiter une par une. Gère la déconnexion.
*/
int handle_client_read(server_t *srv, client_t *c)
{
    ssize_t n;
    char *nl;
    size_t used;

    n = read(c->fd, c->read_buf + c->read_len,
        sizeof(c->read_buf) - c->read_len - 1);
    if (n <= 0) {
        remove_client(srv, c);
        return -1;
    }
    c->read_len += n;
    c->read_buf[c->read_len] = '\0';
    while ((nl = memchr(c->read_buf, '\n', c->read_len)) != NULL) {
        *nl = '\0';
        if (nl > c->read_buf && *(nl - 1) == '\r') /* tolère les fins \r\n */
            *(nl - 1) = '\0';
        process_line(srv, c, c->read_buf);
        used = (nl - c->read_buf) + 1;
        memmove(c->read_buf, c->read_buf + used, c->read_len - used);
        c->read_len -= used;
    }
    return 0;
}
