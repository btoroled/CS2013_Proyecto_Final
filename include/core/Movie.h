//
// Created by Benjamin Toro Leddihn on 21/02/26.
//

#ifndef INC_1_MOVIE_H
#define INC_1_MOVIE_H

#pragma once
#include <string>
#include <vector>

struct Movie {
    int id = -1;
    //DATOS
    std::string imdb_id;
    std::string title;
    std::string plot_synopsis;
    std::vector<std::string> tags;

    // preprocesado
    std::string title_norm;
    std::string synopsis_norm;
    std::string compact;

    // metadata opcional del dataset
    std::string split;
    std::string synopsis_source;
};


#endif //INC_1_MOVIE_H