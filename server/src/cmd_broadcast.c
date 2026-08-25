/*
** ZAPPY - cmd_broadcast.c
** Broadcast : crier un message que tout le monde entend.
**
** L'interet n'est pas le texte mais la DIRECTION : chaque auditeur recoit
** "message K, texte" ou K dit d'ou vient le son, de son propre point de
** vue. Deux drones places differemment recoivent donc des K differents
** pour le meme cri, et c'est ce qui permet a une IA de retrouver un allie.
** Le calcul de K est dans direction.c.
*/

#include <stdio.h>
#include "server.h"

void cmd_broadcast(server_t *srv, client_t *c, const char *text)
{
    char line[ACTION_MAX + 64];
    client_t *l;

    queue_output(c, "ok\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        l = &srv->clients[i];
        if (l->fd == -1 || l == c || l->state != STATE_AI)
            continue;
        snprintf(line, sizeof(line), "message %d, %s\n",
            direction_from(srv, l, c->x, c->y), text);
        queue_output(l, line);
    }
    snprintf(line, sizeof(line), "pbc #%d %s\n", c->id, text);
    gui_broadcast(srv, line);
}
