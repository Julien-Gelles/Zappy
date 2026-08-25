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

| Option | Signification                                     |
| ------ | ------------------------------------------------- |
| `-p`   | port d'écoute                                     |
| `-x`   | largeur de la carte (défaut 20)                   |
| `-y`   | hauteur de la carte (défaut 20)                   |
| `-n`   | noms d'équipes (un ou plusieurs)                  |
| `-c`   | nombre de clients (drones) par équipe             |
| `-f`   | fréquence : 1 unité de temps = 1/f s (défaut 100) |

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
server/src/cmd_broadcast.c  Broadcast : le son
server/src/direction.c    d'où vient un son ou une poussée (1 à 8)
server/src/cmd_fork.c     Fork (pondre un œuf) et Eject (pousser)
server/src/egg.c          les œufs, c'est-à-dire les places d'équipe
server/src/cmd_inventory.c  Inventory, Take, Set
server/src/incant_rules.c   conditions d'une élévation (table du sujet)
server/src/cmd_incant.c   le rituel : début, gel, fin, échec
server/src/victory.c      montée de niveau et fin de partie
server/src/hunger.c       digestion, mort de faim, tick des drones
server/src/player.c       naissance d'un drone, messages pnw/ppo/pin
server/src/gui.c          commandes GUI de la carte (msz, sgt, bct, mct)
server/src/gui_query.c    requêtes GUI sur un joueur (ppo, plv, pin)
server/src/gui_state.c    état complet poussé à un GUI qui se connecte
server/src/gui_server.c   sst (unité de temps) et smg (message serveur)

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
- [x] horloge de jeu (`action / f`) et réapparition des ressources toutes
      les 20 unités (0,2 s à `f=100`)
- [x] commandes IA : `Forward`, `Right`, `Left`, `Look`, `Inventory`,
      `Take`, `Set`, `Connect_nbr`, avec leur coût en temps
- [x] commandes GUI : `msz`, `sgt`, `bct X Y`, `mct`, `ppo/plv/pin #n`
- [x] événements GUI : `pnw`, `ppo`, `pin`, `pgt`, `pdr`, `pdi`
- [x] faim : digestion toutes les 126 unités (1,26 s à f=100) et mort
- [x] `Broadcast` directionnel (`message K, texte`) et `pbc` pour les GUI
- [x] œufs : `Fork`, `Eject`, `Connect_nbr`, naissance sur l'œuf consommé
- [x] état complet envoyé à un GUI qui se connecte en cours de partie
- [x] `Incantation` : conditions, gel des participants, échec sur `Eject`
- [x] condition de victoire (`seg`) : 6 joueurs d'une équipe au niveau 8
- [x] **protocole GUI complet**, `sst` et `smg` compris

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
façon, en chronométrant le serveur de référence. Une unité de temps vaut
`1/f` seconde, soit **10 ms** avec le `f` par défaut de 100 :

| Commande                                    | Unités | À `f=100` |
| ------------------------------------------- | ------ | --------- |
| `Inventory`                                 | 1      | 0,01 s    |
| `Forward`, `Right`, `Left`                  | 7      | 0,07 s    |
| `Look`, `Take`, `Set`, `Broadcast`, `Eject` | 7      | 0,07 s    |
| `Fork`                                      | 42     | 0,42 s    |
| `Connect_nbr`                               | 0      | immédiat  |

Les autres durées du jeu, dans la même unité :

| Événement                           | Unités | À `f=100` |
| ----------------------------------- | ------ | --------- |
| Réapparition des ressources         | 20     | 0,2 s     |
| Digestion d'une unité de nourriture | 126    | 1,26 s    |

Orientations : `1` = Nord (`y-1`), `2` = Est (`x+1`), `3` = Sud (`y+1`),
`4` = Ouest (`x-1`). Le `Look` liste la case du joueur, puis chaque rangée
de gauche à droite. Un joueur ne peut avoir que **10 commandes en
attente** ; les suivantes sont ignorées.

### La faim

Un drone digère une unité de nourriture toutes les **126 unités de temps**
(1,26 s à `f=100`). Quand l'échéance arrive et que son sac est vide, il
meurt : il reçoit `dead`, sa connexion est fermée et les GUI reçoivent
`pdi #n`. Sa place ne revient pas (voir les œufs plus bas).

Sa durée de vie est donc `(nourriture + 1) × 126` unités — les 126
unités de la dernière digestion, celle qui échoue, comptent aussi. Avec le
`START_FOOD` de 50, cela fait 6426 unités, soit **64 s à `f=100`**. La
constante de 126 a été mesurée sur le serveur de référence, dont les
drones vivent exactement 1260 unités (12,6 s) avec 9 nourritures.

`START_FOOD` (dans `server.h`) fixe la réserve de départ.

### Le Broadcast directionnel

Un cri est entendu par tous les autres drones sous la forme
`message K, texte`, où `K` indique d'où vient le son **du point de vue de
celui qui écoute** : `1` devant, `3` à gauche, `5` derrière, `7` à droite,
les nombres pairs pour les diagonales, et `0` si l'émetteur est sur la
même case. Le monde étant un tore, le son prend le chemin le plus court.

### Les œufs

Ce ne sont pas les joueurs qui sont comptés, mais les **œufs**. Chaque
équipe commence avec `-c` œufs posés au hasard sur la carte. Une IA qui se
connecte en consomme un et **naît exactement là où il se trouvait** ;
`Fork` (42 unités, 0,42 s à `f=100`) en pond un nouveau, ce qui crée une
place de plus. `Connect_nbr` répond le nombre d'œufs restants.

Conséquence vérifiée sur le serveur de référence : **la mort d'un drone ne
rend pas sa place**. L'œuf a été consommé pour de bon ; sans `Fork`, une
équipe ne peut jamais dépasser `-c` drones sur toute la partie.

`Eject` (7 unités, 0,07 s à `f=100`) balaie la case :

- tous les **autres** drones qui s'y trouvent sont poussés d'une case dans
  la direction que regarde l'éjecteur — **alliés comme ennemis**, sans
  distinction — et reçoivent `eject: K`, avec la même numérotation que
  `Broadcast`, indiquant le côté d'où vient la poussée ;
- tous les **œufs** qui s'y trouvent sont écrasés (`edi #n` pour les GUI),
  y compris ceux de sa propre équipe.

C'est le rapport des coûts qui fait l'intérêt de la commande : écraser un
œuf coûte 7 unités là où en pondre un en coûte 42, et disperser des drones
réunis pour une incantation ruine 300 unités de préparation.

Deux écarts assumés avec le serveur de référence : il ne détruit pas les
œufs (on suit ici le sujet), et il répond `ok` même quand il n'y avait
rien à éjecter (on s'aligne sur lui sur ce point).

### L'incantation

Pour monter d'un niveau, il faut réunir sur **une même case** un certain
nombre de drones du même niveau et un certain nombre de pierres :

| Élévation | Joueurs | linemate | deraumere | sibur | mendiane | phiras | thystame | Durée | À `f=10` |
| --------- | ------: | -------: | --------: | ----: | -------: | -----: | -------: | ----: | -------: |
| 1 → 2     |       1 |        1 |         0 |     0 |        0 |      0 |        0 |    60 |      6 s |
| 2 → 3     |       2 |        1 |         1 |     1 |        0 |      0 |        0 |   100 |     10 s |
| 3 → 4     |       2 |        2 |         0 |     1 |        0 |      2 |        0 |   140 |     14 s |
| 4 → 5     |       4 |        1 |         1 |     2 |        0 |      1 |        0 |   180 |     18 s |
| 5 → 6     |       4 |        1 |         2 |     1 |        3 |      0 |        0 |   220 |     22 s |
| 6 → 7     |       6 |        1 |         2 |     3 |        0 |      1 |        0 |   260 |     26 s |
| 7 → 8     |       6 |        2 |         2 |     2 |        2 |      2 |        1 |   300 |     30 s |

Deux règles du sujet faciles à manquer :

- **les participants n'ont pas besoin d'être de la même équipe**, seul leur
  niveau compte : un ennemi présent sur la case monte de niveau avec vous ;
- les conditions sont vérifiées **au début et à la fin**. Pendant tout le
  rituel les participants sont **figés** : leurs commandes s'empilent sans
  s'exécuter. Il suffit qu'un `Eject` chasse l'un d'eux de la case pour que
  la vérification finale échoue et que tout le monde reçoive `ko` — les
  pierres, elles, ne sont pas consommées.

**Écart assumé sur la durée.** Le sujet impose 300 unités pour tous les
paliers ; nous la faisons croître avec le niveau — 60 unités puis 40 de
plus à chaque palier, ce qui retombe exactement sur les 300 du sujet pour
le dernier. Les premières élévations deviennent rapides, et la dernière une
longue cérémonie de 30 s qu'un `Eject` de 0,7 s peut ruiner. La formule
tient en une ligne (`INCANT_UNITS` dans `server.h`) : la remplacer par
`300` restaure le comportement du sujet.

Un dernier écart, à l'inverse : le serveur de référence accepte le palier
2 → 3 **sans sibur**, ce qui contredit sa propre table. Nous suivons le
sujet.

La partie s'arrête quand une équipe compte **6 joueurs au niveau 8** : les
GUI reçoivent alors `seg <équipe>`.

### Le protocole GUI

Toutes les commandes et tous les événements du document de protocole sont
implémentés. Ce que le GUI peut demander :

| Commande                       | Réponse                  |
| ------------------------------ | ------------------------ |
| `msz`                          | `msz X Y`                |
| `bct X Y`                      | `bct X Y q0…q6`          |
| `mct`                          | un `bct` par case        |
| `tna`                          | un `tna N` par équipe    |
| `ppo #n` · `plv #n` · `pin #n` | l'état du joueur demandé |
| `sgt`                          | `sgt T`                  |
| `sst T`                        | change l'unité de temps  |

Le serveur émet de lui-même `pnw`, `ppo`, `pin`, `pex`, `pbc`, `pic`,
`pie`, `pfk`, `pdr`, `pgt`, `pdi`, `enw`, `ebo`, `edi`, `seg` et `smg`.
Une commande inconnue reçoit `suc`, un paramètre invalide `sbp`.

Reste à implémenter :

- [ ] client `zappy_ai`
- [ ] client `zappy_gui`

**Le serveur est terminé** : règles du jeu et protocole GUI complets.

## Licence

[MIT](LICENSE)
