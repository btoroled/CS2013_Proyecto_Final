//
// Created by Benjamin Toro Leddihn on 21/02/26.
//

#ifndef INC_1_STREAMINGPLATFORM_H
#define INC_1_STREAMINGPLATFORM_H


#include <string>
#include <unordered_map>
#include <vector>
#include "../core/Movie.h"
#include "../core/User.h"
#include "../index/WordIndex.h"
#include "../index/NgramIndex.h"

struct SearchResult {
    int movie_id = -1;
    double score = 0.0;
};

class StreamingPlatform {
public:
    bool loadDataset(const std::string& path); // 6 cols
    void buildIndexes();

    // búsqueda general (palabra/frase/substring/tag:)
    std::vector<SearchResult> search(const std::string& user_input) const;

    // home
    std::vector<int> recommend(const User& u, int k) const;
    std::vector<std::string> topTags(int k) const;
    std::vector<int> randomByTag(const std::string& tag, int k) const;

    // util
    int movieCount() const { return (int)movies_.size(); }
    const Movie& movieById(int id) const { return movies_.at(id); }
    int idByImdb(const std::string& imdb_id) const;

private:
    // dataset
    std::vector<Movie> movies_;
    std::unordered_map<std::string, int> imdb_to_id_;
    std::unordered_map<std::string, std::vector<int>> tag_to_movies_;

    // índices
    WordIndex wordIndex_;
    NgramIndex ngramIndex_{3};

    // helpers
    static std::vector<std::vector<std::string>> parseSeparatedFile(const std::string& path, char delim);
    static bool isDigits(const std::string& s);

    // ranking weights
    static constexpr double W_TITLE = 10.0;
    static constexpr double W_TAG = 3.0;
    static constexpr double W_SYN = 1.0;
    static constexpr double BONUS_ALL_TOKENS = 5.0;
    static constexpr double BONUS_SUBSTRING = 2.0;
};

#endif //INC_1_STREAMINGPLATFORM_H