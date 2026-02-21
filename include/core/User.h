//
// Created by Benjamin Toro Leddihn on 21/02/26.
//

#ifndef INC_1_USER_H
#define INC_1_USER_H

#include <string>
#include <unordered_set>
#include <vector>

struct User {
    std::string name;
    std::unordered_set<std::string> liked; // imdb_id
    std::vector<std::string> watch_later;  // imdb_id (orden)
};

#endif //INC_1_USER_H