#include <iostream>

#include <iostream>
#include "include/core/StreamingPlatform.h"
#include "include/core/UserStore.h"
#include "include/ui/UI.h"

int main() {
    const std::string DATA_FILE = "data/movies.csv"; // ajusta si tu archivo tiene otro nombre
    const std::string USERS_FILE = "users.txt";

    StreamingPlatform platform;
    if (!platform.loadDataset(DATA_FILE)) {
        std::cerr << "ERROR: No se pudo leer el dataset: " << DATA_FILE << "\n";
        return 1;
    }
    platform.buildIndexes();

    UserStore users;
    users.load(USERS_FILE); // si no existe, empieza vacío

    UI ui(platform, users, USERS_FILE);
    ui.run();
    return 0;
}
