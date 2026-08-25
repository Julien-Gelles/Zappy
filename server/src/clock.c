/*
** ZAPPY - clock.c
** L'horloge du jeu : convertir les "unites de temps" en vrai temps, et
** faire dormir le serveur exactement jusqu'au prochain evenement.
**
** Le sujet interdit l'attente active : on ne boucle jamais en verifiant
** l'heure. A la place, on calcule COMBIEN DE TEMPS il reste avant le
** prochain evenement et on donne ce delai a poll(), qui dort. Le serveur
** se reveille soit parce qu'un client a parle, soit parce que le delai
** est ecoule -- jamais pour rien.
**
**   server_timeout_ms() : combien de temps dormir  (avant le poll)
**   server_tick()       : qu'est-ce qui est du     (apres le poll)
*/

#include <time.h>
#include "server.h"

/*
** L'heure courante en microsecondes, sur l'horloge MONOTONE.
** Monotone = elle avance toujours, meme si l'heure systeme est changee
** (fuseau horaire, NTP). Indispensable pour mesurer des durees.
*/
uint64_t now_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

/*
** Convertit une duree du jeu (en unites de temps) en microsecondes reelles.
** Une unite vaut 1/f seconde, donc n unites valent n * 1000000 / f us.
** Le minimum de 1 evite qu'une frequence enorme donne une duree nulle,
** ce qui ferait tourner la boucle sans jamais dormir.
*/
uint64_t units_to_us(server_t *srv, int units)
{
    uint64_t us = (uint64_t)units * 1000000ULL / (uint64_t)srv->freq;

    return (us == 0) ? 1 : us;
}

/*
** Delai a passer a poll(), en millisecondes :
**   0  -> il y a deja quelque chose a faire, ne pas dormir
**   n  -> dormir au plus n ms
** On arrondit vers le HAUT pour ne pas se reveiller juste avant l'echeance
** et repartir aussitot pour rien.
*/
/*
** L'echeance la plus proche parmi tous les evenements a venir : la
** reapparition des ressources et la fin de la commande en cours de
** chaque joueur.
*/
static uint64_t earliest_deadline(server_t *srv)
{
    uint64_t next = srv->next_refill_us;
    uint64_t d;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        d = client_next_deadline(&srv->clients[i]);
        if (d != 0 && d < next)
            next = d;
        d = client_food_deadline(&srv->clients[i]);
        if (d != 0 && d < next)
            next = d;
        d = srv->clients[i].incant_end_us;
        if (d != 0 && d < next)
            next = d;
    }
    return next;
}

int server_timeout_ms(server_t *srv)
{
    uint64_t now = now_us();
    uint64_t next = earliest_deadline(srv);

    if (next <= now)
        return 0;
    return (int)((next - now + 999) / 1000);
}

/*
** Execute les evenements arrives a echeance. Appele apres chaque poll().
**
** On repart de "maintenant" plutot que d'enchainer les periodes ratees :
** map_spawn_resources() remet de toute facon la carte a son niveau cible
** d'un seul coup, rattraper les periodes manquees ne servirait a rien.
*/
void server_tick(server_t *srv)
{
    uint64_t now = now_us();

    tick_players(srv, now);
    if (now < srv->next_refill_us)
        return;
    if (map_spawn_resources(srv) > 0)
        gui_broadcast_mct(srv);
    srv->next_refill_us = now + units_to_us(srv, REFILL_UNITS);
}
