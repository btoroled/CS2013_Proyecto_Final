#pragma once
#include "Trie.h"
#include "Movie.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <mutex>
#include <sstream>

// ============================================================
// STREAMING PLATFORM — Motor principal
// Patrón Singleton. Estructuras:
//   - Trie: búsqueda de substrings en títulos (O(m))
//   - Inverted Index (unordered_map): búsqueda de palabras en sinopsis (O(1))
//   - Tag Index: búsqueda por tag (O(1))
// ============================================================
class StreamingPlatform {
private:
    std::vector<Movie> movies;

    // Árbol Trie — para búsqueda de substrings en TÍTULOS
    Trie titleTrie;

    // Índice invertido — palabra -> lista de movie ids (para SINOPSIS)
    // Más eficiente que Trie para textos largos: O(1) lookup por palabra
    std::unordered_map<std::string, std::vector<int>> wordIndex;

    // Índice de tags
    std::unordered_map<std::string, std::vector<int>> tagIndex;

    // Estado usuario
    std::vector<int> watchLater;
    std::vector<int> likedMovies;

    StreamingPlatform() = default;

    // Extraer palabras únicas de un texto
    static std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> words;
        std::string word;
        for (char c : text) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            } else {
                if (word.size() >= 2) words.push_back(word);
                word.clear();
            }
        }
        if (word.size() >= 2) words.push_back(word);
        return words;
    }

public:
    static StreamingPlatform& getInstance() {
        static StreamingPlatform instance;
        return instance;
    }
    StreamingPlatform(const StreamingPlatform&) = delete;
    void operator=(const StreamingPlatform&) = delete;

    void loadData(const std::string& csvPath) {
        movies = CSVParser::loadCSV(csvPath);

        for (auto& m : movies) {
            // 1. Trie de títulos
            titleTrie.insert(m.title, m.id);

            // 2. Índice invertido de sinopsis (por palabras)
            auto words = tokenize(m.synopsis);
            std::unordered_set<std::string> seen;
            for (auto& w : words) {
                if (seen.insert(w).second) // solo una vez por película
                    wordIndex[w].push_back(m.id);
            }
            // También palabras del título en wordIndex
            auto titleWords = tokenize(m.title);
            for (auto& w : titleWords) {
                if (seen.insert("t_"+w).second)
                    wordIndex[w].push_back(m.id);
            }

            // 3. Tags
            for (auto& tag : m.tags) {
                std::string tl = CSVParser::toLower(tag);
                tagIndex[tl].push_back(m.id);
            }
        }

        std::cerr << "[INFO] Índices: " << wordIndex.size() << " palabras, "
                  << tagIndex.size() << " tags.\n";
    }

    // ============================================================
    // BÚSQUEDA — híbrida: Trie para substrings, wordIndex para palabras
    // Tokens múltiples → unión (OR) con scoring
    // ============================================================
    std::vector<int> search(const std::string& query) {
        std::unordered_map<int, int> scores;

        auto tokens = tokenize(query);
        if (tokens.empty()) {
            // query sin alfanuméricos, buscar como substring en trie
            auto ids = titleTrie.search(query);
            return std::vector<int>(ids.begin(), ids.end());
        }

        for (auto& token : tokens) {
            // a) Substring en títulos via Trie (peso 5)
            auto titleIds = titleTrie.search(token);
            for (int id : titleIds) scores[id] += 5;

            // b) Palabra exacta en wordIndex (peso 2 synopsis, peso 4 title)
            auto it = wordIndex.find(token);
            if (it != wordIndex.end()) {
                for (int id : it->second) scores[id] += 2;
            }
        }

        // Buscar frase completa en título (bonus)
        if (tokens.size() > 1) {
            auto phraseIds = titleTrie.search(query);
            for (int id : phraseIds) scores[id] += 10;
        }

        std::vector<int> result;
        for (auto& [id, sc] : scores) {
            movies[id].relevance_score = sc;
            result.push_back(id);
        }
        std::sort(result.begin(), result.end(), [this](int a, int b) {
            return movies[a].relevance_score > movies[b].relevance_score;
        });
        return result;
    }

    std::vector<int> searchByTag(const std::string& tag) {
        std::string tl = CSVParser::toLower(tag);
        if (tagIndex.count(tl)) return tagIndex[tl];
        // Búsqueda parcial
        std::unordered_set<int> found;
        for (auto& [t, ids] : tagIndex) {
            if (t.find(tl) != std::string::npos)
                for (int id : ids) found.insert(id);
        }
        return std::vector<int>(found.begin(), found.end());
    }

    void like(int id){ 
        movies[id].liked = true;
        if (std::find(likedMovies.begin(), likedMovies.end(), id) == likedMovies.end())
            likedMovies.push_back(id); 
    }
    void addWatchLater(int id){
        movies[id].watch_later = true;
        if (std::find(watchLater.begin(), watchLater.end(), id) == watchLater.end())
            watchLater.push_back(id); 
    }
    
    void removeWatchLater(int id){
        movies[id].watch_later = false;
        watchLater.erase(std::remove(watchLater.begin(), watchLater.end(), id), watchLater.end());
    }

    const std::vector<int>& getWatchLater()  const { return watchLater; }
    const std::vector<int>& getLikedMovies() const { return likedMovies; }
    Movie& getMovie(int id)             { return movies[id]; }
    const Movie& getMovie(int id) const { return movies[id]; }
    int movieCount() const              { return static_cast<int>(movies.size()); }

    // Recomendaciones: Tag-frequency similarity
    std::vector<int> getRecommendations(int maxResults = 10) {
        if (likedMovies.empty()) return {};
        std::unordered_map<std::string, int> tagFreq;
        for (int id : likedMovies)
            for (auto& tag : movies[id].tags)
                tagFreq[CSVParser::toLower(tag)]++;

        std::unordered_map<int, double> simScores;
        for (auto& [tag, freq] : tagFreq) {
            auto it = tagIndex.find(tag);
            if (it == tagIndex.end()) continue;
            for (int id : it->second)
                if (!movies[id].liked) simScores[id] += freq;
        }

        std::vector<int> recs;
        for (auto& [id, sc] : simScores) recs.push_back(id);
        std::sort(recs.begin(), recs.end(), [&](int a, int b){
            return simScores[a] > simScores[b];
        });
        if ((int)recs.size() > maxResults) recs.resize(maxResults);
        return recs;
    }
};
