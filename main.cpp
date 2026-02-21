/*
 * ============================================================
 * CINEMAX STREAMING PLATFORM
 * Programación III — Proyecto Final (2026-0)
 *
 * Estructuras usadas:
 *   - Trie (árbol de prefijos): búsqueda O(m) de substrings
 *   - unordered_map/set (hash): índice de tags O(1)
 *   - vector: almacenamiento secuencial de películas
 *
 * Conceptos aplicados:
 *   - POO: clases, encapsulamiento, polimorfismo
 *   - Templates: funciones genéricas
 *   - Lambdas: sorting con criterios custom
 *   - Concurrencia: std::async para construcción paralela de índices
 *   - Patrón Singleton: StreamingPlatform
 *   - Big O: búsqueda O(m), ranking O(k log k)
 * ============================================================
 */

 // Windows: fuerza la terminal a usar UTF-8 (código de página 65001)
// Así los acentos y caracteres españoles se muestran correctamente.
// En Linux/Mac no hace nada (el #ifdef lo excluye automáticamente).
#ifdef _WIN32
#define NOMINMAX        // evita que windows.h defina min/max propios
#include <windows.h>
#endif

#include "UI.h"
#include <iostream>
#include <string>
#include <chrono>

// Template genérico para medir tiempo de ejecución
// (Análisis de algoritmos — medición empírica)
template<typename Func>
auto measureTime(Func&& f) {
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end   = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

// ============================================================
// Muestra detalle de película y manejo de acciones Like / Watch Later
// ============================================================
void showMovieDetail(int movieId, StreamingPlatform& platform) {
    while (true) {
        UI::clearScreen();
        Movie& m = platform.getMovie(movieId);
        UI::printMovieDetail(m);

        std::string input;
        std::getline(std::cin, input);

        if (input == "L" || input == "l") {
            platform.like(movieId);
            std::cout << "¡Le diste Like a \"" << m.title << "\"!\n";
            std::cin.ignore(100, '\n'); // pausa breve
            // refrescar
        } else if (input == "W" || input == "w") {
            if (m.watch_later) {
                platform.removeWatchLater(movieId);
                std::cout << "Quitado de Ver más tarde.\n";
            } else {
                platform.addWatchLater(movieId);
                std::cout << "Añadido a Ver más tarde.\n";
            }
            std::cin.ignore(100, '\n');
        } else if (input == "B" || input == "b") {
            break;
        }
    }
}

// ============================================================
// Flujo de búsqueda por string/frase
// ============================================================
void searchFlow(StreamingPlatform& platform, bool byTag = false) {
    UI::clearScreen();
    UI::printHeader();
    if (byTag)
        std::cout << "BUSCAR POR TAG\n";
    else
        std::cout << "BUSCAR PELÍCULA (palabra, frase o substring)\n";
    UI::printSeparator();
    std::cout << "Ingresa tu búsqueda: ";

    std::string query;
    std::getline(std::cin, query);
    if (query.empty()) return;

    // Guardar la query para que UI pueda mostrar el snippet de contexto
    UI::lastQuery = query;

    std::vector<int> results;
    long long ms = measureTime([&]() {
        results = byTag ? platform.searchByTag(query) : platform.search(query);
    });

    if (results.empty()) {
        std::cout << "\nNo se encontraron resultados para: \"" << query << "\"\n";
        std::cout << "Presiona Enter para continuar...";
        std::cin.ignore(1000, '\n');
        UI::lastQuery = "";
        return;
    }

    std::cout << "\nEncontradas " << results.size() << " películas en " << ms << "ms.\n";

    while (true) {
        int sel = UI::showResults(results, platform);
        if (sel == -1) break;
        showMovieDetail(sel, platform);
    }
}

// ============================================================
// Pantalla de inicio — Watch Later + Recomendaciones
// ============================================================
void homeScreen(StreamingPlatform& platform) {
    UI::clearScreen();
    UI::printHeader();

    // Watch Later
    const auto& wl = platform.getWatchLater();
    if (!wl.empty()) {
        std::cout << "\nVER MÁS TARDE (" << wl.size() << " películas):\n";
        UI::printSeparator('-');
        int show = std::min((int)wl.size(), 5);
        for (int i = 0; i < show; ++i)
            UI::printMovieCard(platform.getMovie(wl[i]), i+1);
        if (wl.size() > 5)
            std::cout << "  ... y " << (wl.size()-5) << " más\n";
    }

    // Recomendaciones
    auto recs = platform.getRecommendations(5);
    if (!recs.empty()) {
        std::cout << "\nRECOMENDADAS (basado en tus Likes):\n";
        UI::printSeparator('-');
        for (int i = 0; i < (int)recs.size(); ++i)
            UI::printMovieCard(platform.getMovie(recs[i]), i+1);
    }

    std::cout << "\n";
    UI::printSeparator();
}

// ============================================================
// MENÚ PRINCIPAL
// ============================================================
void mainMenu(StreamingPlatform& platform) {
    while (true) {
        homeScreen(platform);

        std::cout << "MENÚ PRINCIPAL:\n";
        std::cout << "  [1] Buscar película (palabra / frase / substring)\n";
        std::cout << "  [2] Buscar por Tag (cult, horror, etc.)\n";
        std::cout << "  [3] Ver lista 'Ver más tarde'\n";
        std::cout << "  [4] Ver películas que me gustaron\n";
        std::cout << "  [0] Salir\n";
        UI::printSeparator();
        std::cout << "Tu opción: ";

        std::string input;
        std::getline(std::cin, input);

        if (input == "1") {
            searchFlow(platform, false);
        } else if (input == "2") {
            searchFlow(platform, true);
        } else if (input == "3") {
            UI::lastQuery = "";
            const auto& wl = platform.getWatchLater();
            if (wl.empty()) {
                std::cout << "Tu lista está vacía. Presiona Enter...\n";
                std::cin.ignore(1000, '\n');
            } else {
                int sel = UI::showResults(wl, platform);
                if (sel != -1) showMovieDetail(sel, platform);
            }
        } else if (input == "4") {
            UI::lastQuery = "";
            const auto& lk = platform.getLikedMovies();
            if (lk.empty()) {
                std::cout << "No tienes películas con Like. Presiona Enter...\n";
                std::cin.ignore(1000, '\n');
            } else {
                int sel = UI::showResults(lk, platform);
                if (sel != -1) showMovieDetail(sel, platform);
            }
        } else if (input == "0") {
            std::cout << "\n¡Hasta luego!\n";
            break;
        }
    }
}

// ============================================================
// MAIN
// ============================================================
int main(int argc, char* argv[]) {
    // ── Fix de encoding para Windows ──────────────────────
    // Cambia la terminal a UTF-8 (página de código 65001).
    // Sin esto, los acentos y la ñ se ven como caracteres raros.
    // No afecta el contenido: las sinopsis en inglés o español
    // están guardadas en UTF-8 y se leen igual en ambos casos.
#ifdef _WIN32
    SetConsoleOutputCP(65001);  // stdout → UTF-8
    SetConsoleCP(65001);        // stdin  → UTF-8 (por si el usuario escribe acentos)
#endif

    std::string csvPath = "data/movies_clean.csv";
    if (argc >= 2) csvPath = argv[1];

    // Singleton
    StreamingPlatform& platform = StreamingPlatform::getInstance();

    std::cout << "Cargando base de datos...\n";
    long long ms = measureTime([&]() {
        platform.loadData(csvPath);
    });
    std::cout << "Listo en " << ms << "ms. Presiona Enter para continuar...\n";
    std::cin.ignore(1000, '\n');

    mainMenu(platform);
    return 0;
}
