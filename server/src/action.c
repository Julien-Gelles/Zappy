/*
** ZAPPY - action.c
** La file des commandes en attente.
**
** Une commande ne s'execute pas a la reception : elle COUTE DU TEMPS.
** "Forward" vaut 7 unites, soit 7/f seconde. On range donc la commande
** dans une file avec l'heure a laquelle elle devra prendre effet, et le
** serveur l'execute plus tard, quand cette heure arrive.
**
** Les commandes s'enchainent : la deuxieme demarre a la fin de la
** premiere, pas a l'heure ou elle est arrivee. Un joueur qui envoie
** trois Forward d'un coup mettra bien 21 unites a parcourir 3 cases.
**
** Le sujet limite la file a 10 commandes ; au-dela, on ignore (verifie
** sur le serveur de reference : il repond 10 fois sur 15 demandes).
*/

#include <string.h>
#include "server.h"

/* Cout d'une commande, en unites de temps. 0 = reponse immediate. */
static int action_cost(const char *cmd)
{
    if (strcmp(cmd, "Inventory") == 0)
        return COST_INVENTORY;
    if (strcmp(cmd, "Look") == 0)
        return COST_LOOK;
    if (strcmp(cmd, "Forward") == 0 || strcmp(cmd, "Right") == 0
        || strcmp(cmd, "Left") == 0)
        return COST_MOVE;
    if (strncmp(cmd, "Take ", 5) == 0 || strncmp(cmd, "Set ", 4) == 0)
        return COST_OBJECT;
    if (strncmp(cmd, "Broadcast ", 10) == 0)
        return COST_BROADCAST;
    if (strcmp(cmd, "Fork") == 0)
        return COST_FORK;
    if (strcmp(cmd, "Eject") == 0)
        return COST_EJECT;
    return 0;
}

/*
** Ajoute une commande a la file.
** Le depart est la fin de la commande precedente s'il y en a une en
** cours, sinon maintenant.
*/
void action_enqueue(server_t *srv, client_t *c, const char *line)
{
    action_t *a;
    uint64_t start;

    if (c->nb_actions >= MAX_PENDING)
        return;
    a = &c->actions[c->nb_actions];
    start = (c->nb_actions > 0)
        ? c->actions[c->nb_actions - 1].end_us : now_us();
    strncpy(a->cmd, line, ACTION_MAX - 1);
    a->cmd[ACTION_MAX - 1] = '\0';
    a->end_us = start + units_to_us(srv, action_cost(a->cmd));
    c->nb_actions++;
}

/* Retire la commande de tete et decale les suivantes. */
static void action_pop(client_t *c)
{
    for (int i = 1; i < c->nb_actions; i++)
        c->actions[i - 1] = c->actions[i];
    c->nb_actions--;
}

/*
** Execute toutes les commandes dont l'echeance est passee.
** La boucle traite plusieurs commandes d'affilee si le serveur a pris du
** retard, ou si plusieurs commandes a cout nul se suivent.
*/
void client_run_actions(server_t *srv, client_t *c, uint64_t now)
{
    char cmd[ACTION_MAX];

    while (c->nb_actions > 0 && c->actions[0].end_us <= now) {
        memcpy(cmd, c->actions[0].cmd, ACTION_MAX);
        action_pop(c);
        ai_execute(srv, c, cmd);
    }
}

/*
** Echeance de la prochaine commande de ce client, ou 0 s'il n'attend rien.
** Sert a calculer le temps de sommeil du poll().
*/
uint64_t client_next_deadline(client_t *c)
{
    if (c->fd == -1 || c->state != STATE_AI || c->nb_actions == 0)
        return 0;
    return c->actions[0].end_us;
}
