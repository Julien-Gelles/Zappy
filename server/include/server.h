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
** Cout en unites de temps de chaque commande (releve sur le serveur de
** reference). Connect_nbr et les commandes inconnues repondent tout de suite.
*/
    #define COST_MOVE       7  /* Forward, Right, Left */
    #define COST_LOOK       7
    #define COST_INVENTORY  1
    #define COST_OBJECT     7  /* Take, Set */
    #define COST_BROADCAST  7
    #define COST_FORK      42
    #define COST_EJECT      7

    #define MAX_PENDING   10   /* commandes en attente par joueur, au maximum */
    #define ACTION_MAX    256  /* longueur maximale d'une commande stockee */
    #define START_FOOD    50   /* unites de nourriture au depart */

/*
** Toutes les FOOD_UNITS unites de temps, un drone digere une nourriture.
** S'il n'en a plus au moment de la digestion, il meurt : sa duree de vie
** est donc de (nourriture + 1) * FOOD_UNITS unites.
** Valeur verifiee sur le serveur de reference : un drone avec 9 nourritures
** affichees vit exactement 1260 unites, soit 10 * 126.
*/
    #define FOOD_UNITS    126

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
** Une commande en attente d'execution.
** Les commandes coutent du temps : elles ne sont pas jouees a la reception
** mais mises en file, et executees quand leur echeance arrive.
*/
typedef struct action_s {
    char     cmd[ACTION_MAX];
    uint64_t end_us;
} action_t;

/*
** Un oeuf en attente : c'est une PLACE libre dans une equipe.
** Le prochain client de cette equipe naitra a l'endroit de l'oeuf.
*/
typedef struct egg_s {
    int id;
    int team_idx;
    int parent_id;   /* le drone qui l'a pondu, -1 pour les œufs du départ */
    int x;
    int y;
} egg_t;

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

    /* --- données de jeu (uniquement pour un client STATE_AI) --- */
    int             id;          /* numéro montré aux GUI : #0, #1, ... */
    int             team_idx;    /* indice dans srv->team_names */
    int             x;
    int             y;
    int             orientation; /* 1=N 2=E 3=S 4=O */
    int             level;
    int             inventory[NB_RESOURCES];

    /* file des commandes en attente (la plus proche en premier) */
    action_t        actions[MAX_PENDING];
    int             nb_actions;

    /* échéance de la prochaine digestion : c'est l'horloge de la faim */
    uint64_t        food_end_us;
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
    int         nb_teams;

    /*
    ** Les œufs en attente : ce sont eux qui font les places disponibles.
    ** Tableau agrandi au besoin, le plus ancien œuf en premier.
    */
    egg_t      *eggs;
    int         nb_eggs;
    int         cap_eggs;
    int         next_egg_id;

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

    int         next_player_id;  /* compteur pour attribuer les #n aux GUI */
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

/* handshake.c -------------------------------------------------------------- */
void handle_team_name(server_t *srv, client_t *c, const char *name);

/* map.c -------------------------------------------------------------------- */
int     map_wrap(int value, int max);
tile_t *map_at(server_t *srv, int x, int y);
int     map_init(server_t *srv);
void    map_destroy(server_t *srv);

/* map_resources.c ---------------------------------------------------------- */
int         map_target_qty(server_t *srv, int res);
int         map_spawn_resources(server_t *srv);
const char *resource_name(int res);

/* action.c ----------------------------------------------------------------- */
void     action_enqueue(server_t *srv, client_t *c, const char *line);
void     client_run_actions(server_t *srv, client_t *c, uint64_t now);
uint64_t client_next_deadline(client_t *c);

/* commands.c --------------------------------------------------------------- */
void ai_execute(server_t *srv, client_t *c, const char *line);

/* hunger.c ----------------------------------------------------------------- */
void     tick_players(server_t *srv, uint64_t now);
uint64_t client_food_deadline(client_t *c);

/* cmd_look.c --------------------------------------------------------------- */
void cmd_look(server_t *srv, client_t *c);

/* cmd_broadcast.c ---------------------------------------------------------- */
void cmd_broadcast(server_t *srv, client_t *c, const char *text);

/* direction.c -------------------------------------------------------------- */
int  direction_from(server_t *srv, client_t *to, int sx, int sy);

/* cmd_fork.c --------------------------------------------------------------- */
void cmd_fork(server_t *srv, client_t *c);
void cmd_eject(server_t *srv, client_t *c);

/* egg.c -------------------------------------------------------------------- */
int  egg_add(server_t *srv, int team_idx, int parent_id, int x, int y);
int  egg_count(server_t *srv, int team_idx);
int  egg_take(server_t *srv, int team_idx, int *x, int *y);
int  eggs_init(server_t *srv);

/* gui_state.c -------------------------------------------------------------- */
void gui_send_state(server_t *srv, client_t *c);
void gui_send_tna(server_t *srv, client_t *c);

/* cmd_inventory.c ---------------------------------------------------------- */
void cmd_inventory(server_t *srv, client_t *c);
void cmd_take(server_t *srv, client_t *c, const char *name);
void cmd_set(server_t *srv, client_t *c, const char *name);

/* player.c ----------------------------------------------------------------- */
void player_spawn(server_t *srv, client_t *c, int x, int y);
void gui_broadcast(server_t *srv, const char *msg);
void gui_notify_pnw(server_t *srv, client_t *c);
void gui_notify_ppo(server_t *srv, client_t *c);
void gui_notify_pin(server_t *srv, client_t *c);

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

/* gui_query.c -------------------------------------------------------------- */
void gui_query(server_t *srv, client_t *c, const char *line);

#endif /* SERVER_H */
