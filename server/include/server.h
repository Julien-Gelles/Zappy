/*
** ZAPPY - server.h
** Structures et prototypes du serveur.
*/

#ifndef SERVER_H
    #define SERVER_H

    #include <poll.h>
    #include <stddef.h>

    #define MAX_CLIENTS   1024
    #define READ_CHUNK    4096
    #define GRAPHIC_TEAM  "GRAPHIC"

/*
** Etat d'un client dans le handshake / le jeu.
** WAIT_TEAM : connecté, on lui a envoyé WELCOME, on attend son nom d'équipe.
** AI        : c'est une IA authentifiée (un drone).
** GUI       : c'est le client graphique (a envoyé GRAPHIC).
*/
typedef enum {
    STATE_WAIT_TEAM = 0,
    STATE_AI,
    STATE_GUI
} client_state_t;

/*
** Un client connecté (IA ou GUI).
** fd == -1 signifie "slot libre" dans le tableau server.clients[].
**
** read_buf  : accumule les octets reçus jusqu'à trouver un '\n'.
** write_buf : file d'octets à envoyer (rempli par queue_output, vidé par flush).
*/
typedef struct client_s {
    int             fd;
    client_state_t  state;
    char           *team_name;

    char            read_buf[READ_CHUNK];
    size_t          read_len;

    char           *write_buf;
    size_t          write_len;

    /* --- A COMPLETER PLUS TARD (données de jeu) --- */
    /* int x, y;                */
    /* int orientation;         */ /* 1=N 2=E 3=S 4=O */
    /* int level;               */
    /* int inventory[7];        */
    /* long food_timer;         */
} client_t;

/*
** Configuration + état global du serveur.
*/
typedef struct server_s {
    int         listen_fd;

    /* configuration (arguments de la ligne de commande) */
    int         port;
    int         width;
    int         height;
    int         freq;
    int         clients_nb;   /* nb de slots initiaux par équipe (-c) */

    /* équipes */
    char      **team_names;
    int        *team_used;    /* slots occupés par équipe */
    int         nb_teams;

    /* clients connectés */
    client_t    clients[MAX_CLIENTS];

    /* --- A COMPLETER PLUS TARD --- */
    /* tile_t **map;     */ /* carte width*height, ressources par case */
    /* long time_unit;   */
} server_t;

/* args.c ------------------------------------------------------------------- */
int  parse_args(server_t *srv, int argc, char **argv);

/* server.c ----------------------------------------------------------------- */
int  server_init(server_t *srv);
int  server_run(server_t *srv);
void server_cleanup(server_t *srv);

/* client.c ----------------------------------------------------------------- */
int  accept_client(server_t *srv);
void remove_client(server_t *srv, client_t *c);
int  handle_client_read(server_t *srv, client_t *c);
int  flush_output(client_t *c);
int  queue_output(client_t *c, const char *msg);
void process_line(server_t *srv, client_t *c, char *line);

#endif /* SERVER_H */
