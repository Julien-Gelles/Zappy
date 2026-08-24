# Zappy

Projet Epitech : un jeu multijoueur en réseau où des drones autonomes
explorent une carte, récoltent des ressources et tentent de faire
évoluer leur équipe jusqu'au niveau max via des incantations rituelles.

Le projet est composé de **trois programmes indépendants** qui
communiquent par sockets TCP :

| Binaire        | Rôle                                  | Langage |
| -------------- | ------------------------------------- | ------- |
| `zappy_server` | L'arbitre : fait respecter les règles | C       |
| `zappy_ai`     | Un drone autonome, piloté par une IA  | C       |
| `zappy_gui`    | Un client graphique en lecture seule  | C++     |

Le sujet complet et le protocole GUI sont dans
[`G-YEP-400_zappy.pdf`](G-YEP-400_zappy.pdf) et
[`G-YEP-400_zappy_GUI_protocol.pdf`](G-YEP-400_zappy_GUI_protocol.pdf).

## Construire

```bash
make        # build les 3 binaires
make clean  # supprime les .o
make fclean # supprime aussi les binaires
make re     # fclean + all
```

## Lancer

```bash
./zappy_server -p 4242 -x 10 -y 10 -n team1 team2 -c 3 -f 100
```

| Option | Signification                                 |
| ------ | --------------------------------------------- |
| `-p`   | port d'écoute                                 |
| `-x`   | largeur de la carte (défaut 20)               |
| `-y`   | hauteur de la carte (défaut 20)               |
| `-n`   | noms d'équipes (un ou plusieurs)              |
| `-c`   | nombre de clients (drones) par équipe         |
| `-f`   | fréquence de jeu, inverse de l'unité de temps |

Un client IA se connecte ensuite avec le nom d'une équipe déclarée,
et le GUI avec le nom réservé `GRAPHIC` :

```bash
./zappy_ai -p 4242 -n team1 -h localhost
./zappy_gui -p 4242 -h localhost
```

## Structure du dépôt

```
server/include/server.h   structures (server_t, client_t) et prototypes
server/src/main.c         point d'entrée, orchestration
server/src/args.c         parsing des arguments -p -x -y -n -c -f
server/src/server.c       socket d'écoute + boucle poll()
server/src/client.c       accept, buffering, aiguillage des lignes
server/src/handshake.c    nom d'équipe -> GUI ou drone
server/src/map.c          carte torique : allocation et accès aux cases
server/src/map_resources.c  densités et dispersion des 7 ressources
server/src/clock.c        horloge du jeu, timeout du poll(), événements dus
server/src/action.c       file des commandes en attente et leur coût
server/src/commands.c     exécution : Forward, Right, Left, Connect_nbr
server/src/cmd_look.c     Look : le cône de vision
server/src/cmd_inventory.c  Inventory, Take, Set
server/src/player.c       naissance d'un drone, messages pnw/ppo/pin
server/src/gui.c          commandes GUI de la carte (msz, sgt, bct, mct)
server/src/gui_query.c    requêtes GUI sur un joueur (ppo, plv, pin)

ai/src/main.c             stub du client IA (à implémenter)
gui/src/main.cpp          stub du client GUI (à implémenter)

zappy_ref-v3.0.1/         binaires de référence fournis par l'école,
                          utiles pour tester son propre serveur/GUI
                          en remplaçant l'un des deux composants
```

## État d'avancement

Fait :

- [x] parsing des arguments, socket non bloquante, boucle `poll()`
- [x] bufferisation entrée/sortie par client, découpage des lignes
- [x] handshake IA et GUI (`WELCOME` → nom d'équipe → `ok`/`ko`)
- [x] carte torique + génération des ressources aux densités du sujet
- [x] horloge de jeu (`action / f`) et réapparition toutes les 20 unités
- [x] commandes IA : `Forward`, `Right`, `Left`, `Look`, `Inventory`,
      `Take`, `Set`, `Connect_nbr`, avec leur coût en temps
- [x] commandes GUI : `msz`, `sgt`, `bct X Y`, `mct`, `ppo/plv/pin #n`
- [x] événements GUI : `pnw`, `ppo`, `pin`, `pgt`, `pdr`, `pdi`

Les densités ont été relevées sur le serveur de référence, en comparant
les totaux de `mct` sur plusieurs tailles de carte. La quantité visée est
`width * height * densité`, arrondie à l'entier le plus proche :

| Ressource | Densité | 121 cases |
| --------- | ------- | --------- |
| food      | 0.50    | 61        |
| linemate  | 0.30    | 36        |
| deraumere | 0.15    | 18        |
| sibur     | 0.10    | 12        |
| mendiane  | 0.10    | 12        |
| phiras    | 0.08    | 10        |
| thystame  | 0.05    | 6         |

Le coût des commandes et le repère de la carte ont été relevés de la même
façon, en chronométrant le serveur de référence :

| Commande | Coût (unités de temps) |
| -------- | ---------------------- |
| `Forward`, `Right`, `Left` | 7 |
| `Look`, `Take`, `Set`      | 7 |
| `Inventory`                | 1 |
| `Connect_nbr`              | 0 (immédiat) |

Orientations : `1` = Nord (`y-1`), `2` = Est (`x+1`), `3` = Sud (`y+1`),
`4` = Ouest (`x-1`). Le `Look` liste la case du joueur, puis chaque rangée
de gauche à droite. Un joueur ne peut avoir que **10 commandes en
attente** ; les suivantes sont ignorées.

Un écart assumé avec la référence : nos drones démarrent avec **10**
unités de nourriture (valeur du sujet), là où le serveur de référence en
affiche 9.

Reste à implémenter :

- [ ] commandes IA restantes (`Broadcast`, `Eject`, `Fork`, `Incantation`)
- [ ] timer de faim et mort des joueurs
- [ ] reste du protocole GUI (`tna`, `sst`, `enw`/`ebo`, `pic`, `seg`...)
- [ ] client `zappy_ai`
- [ ] client `zappy_gui`

## Licence

[MIT](LICENSE)
