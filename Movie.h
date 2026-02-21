#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>

// ============================================================
// MOVIE — Entidad principal 
// ============================================================
struct Movie {
    int id;                    // índice interno
    std::string imdb_id;
    std::string title;
    std::string synopsis;
    std::vector<std::string> tags;
    bool liked      = false;
    bool watch_later = false;

    // Relevancia para ranking (cuántas veces aparece el query)
    int relevance_score = 0;

    std::string tagsStr() const {
        std::string s;
        for (size_t i = 0; i < tags.size(); ++i) {
            if (i != 0) s += ", ";
            s += tags[i];
        }
        return s;
    }
};

// ============================================================
// CSV Parser robusto — maneja comillas escapadas y newlines
// ============================================================
namespace CSVParser {

static std::string toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c){ return std::tolower(c); });
    return r;
}

// Cuenta ocurrencias de substr en text (case-insensitive)
static int countOccurrences(const std::string& text, const std::string& sub) {
    if (sub.empty()) return 0;
    std::string tl = toLower(text);
    std::string sl = toLower(sub);
    int count = 0;
    size_t pos = 0;
    while ((pos = tl.find(sl, pos)) != std::string::npos) {
        ++count;
        pos += sl.size();
    }
    return count;
}

// Parser de un campo CSV (maneja comillas dobles)
static std::string parseField(const std::string& line, size_t& pos) {
    std::string field;
    if (pos >= line.size()) return field;

    if (line[pos] == '"') {
        ++pos; // skip opening quote
        while (pos < line.size()) {
            if (line[pos] == '"') {
                if (pos + 1 < line.size() && line[pos+1] == '"') {
                    field += '"';
                    pos += 2;
                } else {
                    ++pos; // skip closing quote
                    break;
                }
            } else {
                field += line[pos++];
            }
        }
        if (pos < line.size() && line[pos] == ',') ++pos;
    } else {
        while (pos < line.size() && line[pos] != ',') {
            field += line[pos++];
        }
        if (pos < line.size()) ++pos;
    }
    return field;
}

static std::vector<Movie> loadCSV(const std::string& filepath) {
    std::vector<Movie> movies;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir " << filepath << "\n";
        return movies;
    }

    std::string line, fullLine;
    std::getline(file, line); // skip header

    int id = 0;
    while (std::getline(file, fullLine)) {
        // Manejo de líneas multi-línea dentro de comillas
        while (std::count(fullLine.begin(), fullLine.end(), '"') % 2 != 0) {
            std::string extra;
            if (!std::getline(file, extra)) break;
            fullLine += "\n" + extra;
        }

        size_t pos = 0;
        std::string imdb  = parseField(fullLine, pos);
        std::string title = parseField(fullLine, pos);
        std::string synopsis = parseField(fullLine, pos);
        std::string tagStr   = parseField(fullLine, pos);

        if (imdb.empty() || title.empty()) continue;

        Movie m;
        m.id       = id++;
        m.imdb_id  = imdb;
        m.title    = title;
        m.synopsis = synopsis;

        // Parse tags
        std::stringstream ss(tagStr);
        std::string tag;
        while (std::getline(ss, tag, ',')) {
            // trim
            tag.erase(0, tag.find_first_not_of(" \t"));
            tag.erase(tag.find_last_not_of(" \t") + 1);
            if (!tag.empty()) m.tags.push_back(tag);
        }

        movies.push_back(std::move(m));
    }

    std::cerr << "[INFO] Cargadas " << movies.size() << " películas.\n";
    return movies;
}

} // namespace CSVParser
