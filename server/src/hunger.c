/*
** ZAPPY - hunger.c
** La faim : le compte a rebours qui finit par tuer les drones.
**
** Toutes les FOOD_UNITS (126) unites de temps, chaque drone digere une
** unite de nourriture. Le jour ou il faut digerer et qu'il n'y a plus
** rien dans le sac, le drone meurt. Un drone qui demarre avec N
** nourritures vit donc (N + 1) * 126 unites : N digestions, puis une
** derniere echeance ou le sac est vide.
**
** C'est ce qui rend la nourriture au sol vitale : sans Take reguliers,
** aucune IA ne survit assez longtemps pour monter en niveau.
**
** A la mort, le protocole demande (verifie sur le serveur de reference) :
**   "dead" a l'IA concernee, puis fermeture de sa connexion
**   "pdi #n" a tous les GUI
*/

#include "server.h"

/*
** Le message "dead" doit partir AVANT la fermeture de la socket : on
** force donc l'envoi tout de suite, sans attendre le prochain POLLOUT.
** remove_client() se charge ensuite du pdi et de liberer la place.
*/
static void player_dies(server_t *srv, client_t *c)
{
    queue_output(c, "dead\n");
    flush_output(c);
    remove_client(srv, c);
}

/* Une echeance de digestion est-elle arrivee pour ce drone ? */
static void check_hunger(server_t *srv, client_t *c, uint64_t now)
{
    if (now < c->food_end_us)
        return;
    if (srv->no_food) {
        /* Mode mise au point (/noFood true) : l'horloge continue de
        ** tourner mais on ne consomme rien et personne ne meurt. */
        c->food_end_us = now + units_to_us(srv, FOOD_UNITS);
        return;
    }
    if (c->inventory[RES_FOOD] <= 0) {
        player_dies(srv, c);
        return;
    }
    c->inventory[RES_FOOD]--;
    c->food_end_us = now + units_to_us(srv, FOOD_UNITS);
    gui_notify_pin(srv, c);
}

/*
** Fait avancer tous les drones : leurs commandes en attente, puis leur
** faim. On revalide fd apres les commandes, car un drone peut disparaitre
** en cours de route.
*/
void tick_players(server_t *srv, uint64_t now)
{
    client_t *c;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        c = &srv->clients[i];
        if (c->fd == -1 || c->state != STATE_AI)
            continue;
        incant_tick(srv, c, now);
        client_run_actions(srv, c, now);
        if (c->fd != -1)
            check_hunger(srv, c, now);
    }
}

/* Echeance de la prochaine digestion, 0 si ce client n'est pas un drone. */
uint64_t client_food_deadline(client_t *c)
{
    if (c->fd == -1 || c->state != STATE_AI)
        return 0;
    return c->food_end_us;
}
