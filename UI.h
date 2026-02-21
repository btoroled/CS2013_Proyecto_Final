#pragma once
#include "StreamingPlatform.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

// ============================================================
// UI — Funciones de presentación en consola
// ============================================================
namespace UI {

// Última query usada — para mostrar contexto del match en resultados
static std::string lastQuery = "";

// ============================================================
// getSnippet: Extrae un fragmento (~120 chars) de 'text' alrededor de
// la primera aparición de 'query'. Así el usuario ve POR QUÉ
// la película fue encontrada aunque no aparezca en el preview.
//
// FIX: antes solo buscaba el PRIMER token de la query.
// Si buscabas "barco fantasma" y la película matcheó por
// "fantasma" (no "barco"), no encontraba nada y no mostraba nada.
//
// Ahora prueba cada token de la query en orden hasta encontrar
// uno presente en el texto, siempre muestra el contexto correcto.
// ============================================================

static std::string getSnippet(const std::string& text, const std::string& query) {
    if (query.empty() || text.empty()) return "";

    // Texto en minúsculas para búsqueda case-insensitive
    std::string textLow = text;
    std::transform(textLow.begin(), textLow.end(), textLow.begin(),
        [](unsigned char c){ return std::tolower(c); });
    
    // Tokenizar la query en palabras individuales
    std::string queryLow = query;
    std::transform(queryLow.begin(), queryLow.end(), queryLow.begin(),
        [](unsigned char c){ return std::tolower(c); });

    std::vector<std::string> tokens;
    std::istringstream iss(queryLow);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    
    // Probar cada token hasta encontrar uno presente en el texto
    for (const std::string& token : tokens) {
        size_t pos = textLow.find(token);
        if (pos == std::string::npos) continue;

        // Ventana de 120 chars centrada en el match
        size_t start = (pos > 40) ? pos - 40 : 0;
        std::string snippet = (start > 0 ? "..." : "")
                            + text.substr(start, 120)
                            + (start + 120 < text.size() ? "..." : "");
        return snippet;
    }
    return ""; // ningún token presente en este texto
}

static void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void printSeparator(char c = '=', int width = 70) {
    std::cout << std::string(width, c) << "\n";
}

static void printHeader() {
    printSeparator();
    std::cout << "     CINEMAX STREAMING  — Plataforma de Películas\n";
    printSeparator();
}

// ============================================================
// Tarjeta de resultado: título + tags + snippet de dónde matcheó
// ============================================================
static void printMovieCard(const Movie& m, int rank = -1) {
    if (rank >= 0)
        std::cout << "  [" << std::setw(2) << rank << "] ";
    else
        std::cout << "       ";

    std::cout << m.title;
    if (m.liked)       std::cout << " ❤";
    if (m.watch_later) std::cout << " 🕐";
    std::cout << "\n";

    if (!m.tags.empty())
        std::cout << "        Tags: " << m.tagsStr() << "\n";

    //Mostrar DONDE aparece el match — resuelve la confusión del usuario    
    if (!lastQuery.empty()) {
        //¿Matchea en titulo?
        std::string titleLow = m.title;
        std::string qLow = lastQuery;
        std::transform(titleLow.begin(), titleLow.end(), titleLow.begin(),
            [](unsigned char c){ return std::tolower(c); });
        std::transform(qLow.begin(), qLow.end(), qLow.begin(),
            [](unsigned char c){ return std::tolower(c); });

        // Revisar si algún token del query está en el título
        bool inTitle = false;
        std::istringstream iss(qLow);
        std::string tok;
        while (iss >> tok) {
            if (titleLow.find(tok) != std::string::npos) { inTitle = true; break; }
        }

        if (inTitle) {
            std::cout << "        >> Match en titulo\n";
        } else {
            // Mostrar snippet de sinopsis con el token que matcheó
            std::string snip = getSnippet(m.synopsis, lastQuery);
            if (!snip.empty())
                std::cout << "        >> Sinopsis: \"" << snip << "\"\n";
        }
    }
}    
static void printMovieDetail(const Movie& m) {
    printSeparator();
    std::cout << "TÍTULO: " << m.title << "\n";
    std::cout << "IMDB:   " << m.imdb_id << "\n";
    std::cout << "TAGS:   " << m.tagsStr() << "\n";
    printSeparator('-');
    std::cout << "SINOPSIS:\n";

    // Mostrar sinopsis cortada a 600 chars
    std::string syn = m.synopsis;
    if (syn.size() > 600) {
        syn = syn.substr(0, 600) + "...\n[Sinopsis completa en IMDB: https://www.imdb.com/title/" + m.imdb_id + "/]";
    }
    std::cout << syn << "\n";
    printSeparator('-');

    std::cout << "Opciones:\n";
    std::cout << "  [L] Like";
    if (m.liked) std::cout << " (ya diste Like ❤)";
    std::cout << "\n";
    std::cout << "  [W] Ver más tarde";
    if (m.watch_later) std::cout << " (ya está en tu lista 🕐)";
    std::cout << "\n";
    std::cout << "  [B] Volver\n";
    printSeparator();
}

// Muestra hasta 5 resultados con paginación
// Retorna el índice seleccionado (-1 = volver)
static int showResults(const std::vector<int>& results,
                       StreamingPlatform& platform,
                       int page = 0)
{
    const int PAGE_SIZE = 5;
    int total = static_cast<int>(results.size());
    int start = page * PAGE_SIZE;
    int end   = std::min(start + PAGE_SIZE, total);

    printSeparator();
    std::cout << "Resultados " << start+1 << "-" << end << " de " << total << ":\n\n";

    for (int i = start; i < end; ++i) {
        printMovieCard(platform.getMovie(results[i]), i - start + 1);
    }

    std::cout << "\nOpciones:\n";
    std::cout << "  [1-5] Seleccionar película\n";
    if (end < total)
        std::cout << "  [N]   Siguiente página\n";
    if (page > 0)
        std::cout << "  [P]   Página anterior\n";
    std::cout << "  [B]   Volver\n";
    printSeparator();
    std::cout << "Tu opción: ";

    std::string input;
    std::getline(std::cin, input);

    if (input == "N" || input == "n") {
        if (end < total) return showResults(results, platform, page + 1);
    } else if (input == "P" || input == "p") {
        if (page > 0) return showResults(results, platform, page - 1);
    } else if (input == "B" || input == "b") {
        return -1;
    } else {
        try {
            int sel = std::stoi(input) - 1;
            if (sel >= 0 && sel < end - start) {
                return results[start + sel];
            }
        } catch (...) {}
    }
    return showResults(results, platform, page);
}

} // namespace UI
