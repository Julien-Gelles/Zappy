/*
** ZAPPY - gui_server.c
** Les deux messages qui parlent du serveur lui-meme, pas du monde :
**   sst T : le GUI change l'unite de temps en cours de partie
**   smg M : le serveur adresse un message libre aux GUI
**
** Changer l'unite de temps demande une precaution. Tout le jeu se compte
** en UNITES : "il reste 200 unites avant la prochaine digestion". Mais les
** echeances sont stockees en microsecondes reelles, calculees avec
** l'ancienne valeur de f. Passer de f=2 a f=1000 sans rien faire laisserait
** une reapparition de ressources prevue dans 10 secondes alors qu'elle
** devrait arriver dans 20 millisecondes.
**
** On remet donc toutes les echeances a l'echelle : ce qu'il restait a
** attendre est reexprime avec la nouvelle unite. Le temps de jeu restant
** est preserve, seule sa duree reelle change -- ce qui est exactement ce
** qu'attend quelqu'un qui pousse le curseur de vitesse.
*/

#include <stdio.h>
#include "server.h"

/* Reexprime une echeance avec la nouvelle unite de temps. */
static uint64_t rescale(uint64_t deadline, uint64_t now, int old_f, int new_f)
{
    uint64_t left;

    if (deadline == 0 || deadline <= now)
        return deadline;
    left = deadline - now;
    return now + left * (uint64_t)old_f / (uint64_t)new_f;
}

/*
** Toutes les echeances du jeu : reapparition des ressources, digestion,
** rituels en cours et commandes en attente de chaque drone.
*/
static void rescale_all(server_t *srv, int old_f, uint64_t now)
{
    client_t *c;

    srv->next_refill_us = rescale(srv->next_refill_us, now, old_f, srv->freq);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        c = &srv->clients[i];
        if (c->fd == -1 || c->state != STATE_AI)
            continue;
        c->food_end_us = rescale(c->food_end_us, now, old_f, srv->freq);
        c->incant_end_us = rescale(c->incant_end_us, now, old_f, srv->freq);
        for (int k = 0; k < c->nb_actions; k++)
            c->actions[k].end_us =
                rescale(c->actions[k].end_us, now, old_f, srv->freq);
    }
}

void gui_set_time(server_t *srv, client_t *c, const char *line)
{
    char out[64];
    int old_f = srv->freq;
    int t = 0;

    if (sscanf(line + 3, "%d", &t) != 1 || t <= 0) {
        queue_output(c, "sbp\n");
        return;
    }
    srv->freq = t;
    rescale_all(srv, old_f, now_us());
    /*
    ** Le tableau du protocole annonce "sst T", mais le serveur de
    ** reference repond "sgt T". On envoie les deux : un GUI ignore les
    ** lignes qu'il ne connait pas, donc aucun des deux ne se perd.
    */
    snprintf(out, sizeof(out), "sst %d\n", t);
    gui_broadcast(srv, out);
    snprintf(out, sizeof(out), "sgt %d\n", t);
    gui_broadcast(srv, out);
}

/* Message libre du serveur vers tous les GUI. */
void gui_send_smg(server_t *srv, const char *msg)
{
    char line[256];

    snprintf(line, sizeof(line), "smg %s\n", msg);
    gui_broadcast(srv, line);
}
