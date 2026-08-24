/*
** ZAPPY - zappy_gui (STUB)
** USAGE: ./zappy_gui -p port -h machine
**
** A COMPLETER (C++) : se connecter au serveur, envoyer "GRAPHIC" comme nom
** d'équipe, puis lire le flux d'événements (msz, bct, pnw, ppo, pic, ...) et
** dessiner le monde (SFML conseillé pour la 2D). Bufferiser les I/O comme le serveur.
*/

#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "USAGE: ./zappy_gui -p port -h machine\n"
                  << "\t-p port\t\tport number\n"
                  << "\t-h machine\thostname of the server\n";
        return 0;
    }
    std::cerr << "[zappy_gui] stub - a implementer.\n";
    return 0;
}
