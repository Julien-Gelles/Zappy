/*
** ZAPPY - server.h
** Structures et prototypes du serveur.
*/

#ifndef SERVER_H
    #define SERVER_H

    #include <poll.h>
    #include <stddef.h>
    #include <stdint.h>

    #define MAX_CLIENTS   1024
    #define READ_CHUNK    4096
    #define GRAPHIC_TEAM  "GRAPHIC"
    #define NB_RESOURCES  7

/*
** Le temps du jeu se compte en "unites de temps".
** Une unite dure 1/f seconde : plus f est grand, plus le jeu va vite.
** Une action qui coute 7 unites prend donc 7/f seconde.
*/
    #define REFILL_UNITS  20   /* les ressources reapparaissent tous les 20 */

/*
** Les 7 ressources du jeu, dans l'ordre impose par le protocole GUI
** (bct X Y q0 q1 q2 q3 q4 q5 q6). Cet ordre ne doit pas changer.
*/
typedef enum {
    RES_FOOD = 0,
    RES_LINEMATE,
    RES_DERAUMERE,
    RES_SIBUR,
    RES_MENDIANE,
    RES_PHIRAS,
    RES_THYSTAME
} resource_t;

/*
** Une case de la carte : combien d'unites de chaque ressource s'y trouvent.
** Une case peut contenir plusieurs ressources en meme temps.
*/
typedef struct tile_s {
    int qty[NB_RESOURCES];
} tile_t;

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

    /*
    ** La carte : un tableau a plat de width*height cases.
    ** La case (x, y) est a l'indice y * width + x -> voir map_at().
    */
    tile_t     *map;

    /*
    ** Echeance de la prochaine reapparition des ressources, en microsecondes
    ** sur l'horloge monotone. C'est elle qui fixe le timeout du poll().
    */
    uint64_t    next_refill_us;
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

/* map.c -------------------------------------------------------------------- */
int     map_wrap(int value, int max);
tile_t *map_at(server_t *srv, int x, int y);
int     map_init(server_t *srv);
void    map_destroy(server_t *srv);

/* map_resources.c ---------------------------------------------------------- */
int  map_target_qty(server_t *srv, int res);
int  map_spawn_resources(server_t *srv);

/* clock.c ------------------------------------------------------------------ */
uint64_t now_us(void);
uint64_t units_to_us(server_t *srv, int units);
int      server_timeout_ms(server_t *srv);
void     server_tick(server_t *srv);

/* gui.c -------------------------------------------------------------------- */
void gui_send_bct(server_t *srv, client_t *c, int x, int y);
void gui_send_mct(server_t *srv, client_t *c);
void gui_broadcast_mct(server_t *srv);
void gui_command(server_t *srv, client_t *c, const char *line);

#endif /* SERVER_H */
