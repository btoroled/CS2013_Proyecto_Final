//
// Created by Benjamin Toro Leddihn on 21/02/26.
//

#ifndef INC_1_UI_H
#define INC_1_UI_H

#include <string>
#include <vector>
#include "../core/StreamingPlatform.h"
#include "../core/UserStore.h"

class UI {
public:
    UI(StreamingPlatform& platform, UserStore& users, const std::string& usersFile);

    void run();

private:
    StreamingPlatform& platform_;
    UserStore& users_;
    std::string usersFile_;

    // terminal helpers
    static void clear();
    static int readInt();
    static std::string readLineTrim();
    static void pause();

    // screens
    int profilesScreen();               // retorna slot activo o -1 salir
    void homeScreen(int slot);
    void searchScreen(int slot);
    void listMoviesScreen(int slot, const std::vector<int>& movie_ids, const std::string& title);
    void searchResultsScreen(int slot, const std::vector<SearchResult>& results, const std::string& title);
    void movieDetailScreen(int slot, int movie_id);

    // render
    void renderProfilesRow() const;
};

#endif //INC_1_UI_H