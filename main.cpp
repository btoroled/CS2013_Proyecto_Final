#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>

#include "include/core/StreamingPlatform.h"
#include "include/core/UserStore.h"
#include "include/ui/UI.h"
#include "include/text/TextUtils.h"

using Clock = std::chrono::steady_clock;

static double ms_between(const Clock::time_point& a, const Clock::time_point& b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

static std::string pick_word_query(const StreamingPlatform& p) {
    // Busca una palabra "decente" (>=3 chars) en títulos/sinopsis de las primeras películas
    int limit = std::min(p.movieCount(), 50);
    for (int i = 0; i < limit; i++) {
        const auto& m = p.movieById(i);

        {
            auto tks = text::split_words(text::normalize_ascii(m.title));
            for (auto& tk : tks) if (tk.size() >= 3) return tk;
        }
        {
            auto tks = text::split_words(text::normalize_ascii(m.plot_synopsis));
            for (auto& tk : tks) if (tk.size() >= 3) return tk;
        }
        for (const auto& tag : m.tags) {
            if (tag.size() >= 3) return tag; // tags ya vienen normalizados
        }
    }
    return "the"; // fallback
}

static std::string pick_phrase_query(const StreamingPlatform& p) {
    int limit = std::min(p.movieCount(), 50);
    for (int i = 0; i < limit; i++) {
        const auto& m = p.movieById(i);

        auto syn = text::split_words(text::normalize_ascii(m.plot_synopsis));
        if (syn.size() >= 2) return syn[0] + " " + syn[1];

        auto ttl = text::split_words(text::normalize_ascii(m.title));
        if (ttl.size() >= 2) return ttl[0] + " " + ttl[1];
    }
    return "love story"; // fallback
}

static std::string pick_tag_query(const StreamingPlatform& p) {
    auto top = p.topTags(1);
    if (!top.empty()) return "tag:" + top[0];

    // fallback: intenta desde primeras pelis
    int limit = std::min(p.movieCount(), 50);
    for (int i = 0; i < limit; i++) {
        const auto& m = p.movieById(i);
        if (!m.tags.empty()) return "tag:" + m.tags[0];
    }
    return "tag:drama";
}

static double avg_search_ms(const StreamingPlatform& p, const std::string& q, int runs, size_t& sink) {
    // Warmup
    for (int i = 0; i < 2; i++) {
        auto r = p.search(q);
        sink += r.size();
    }

    auto t0 = Clock::now();
    for (int i = 0; i < runs; i++) {
        auto r = p.search(q);
        sink += r.size();
    }
    auto t1 = Clock::now();
    return ms_between(t0, t1) / runs;
}

static int run_benchmark(const std::string& data_file) {
    StreamingPlatform p;

    // 1) Carga dataset
    auto t0 = Clock::now();
    bool ok = p.loadDataset(data_file);
    auto t1 = Clock::now();
    if (!ok) {
        std::cerr << "ERROR: No se pudo leer el dataset: " << data_file << "\n";
        return 1;
    }
    double load_ms = ms_between(t0, t1);

    // 2) Build índices
    auto t2 = Clock::now();
    p.buildIndexesParallel();
    auto t3 = Clock::now();
    double build_ms = ms_between(t2, t3);

    int N = p.movieCount();

    // Queries representativas (derivadas del dataset)
    std::string q_word   = pick_word_query(p);
    std::string q_phrase = pick_phrase_query(p);
    std::string q_tag    = pick_tag_query(p);

    // 3) Búsquedas (promedio 10 runs)
    size_t sink = 0;
    const int runs = 10;

    double word_ms   = avg_search_ms(p, q_word, runs, sink);
    double phrase_ms = avg_search_ms(p, q_phrase, runs, sink);
    double tag_ms    = avg_search_ms(p, q_tag, runs, sink);

    // Output listo para README (Markdown)
    std::cout << "\n=== BENCHMARK ===\n";
    std::cout << "Dataset: " << data_file << "\n";
    std::cout << "N peliculas: " << N << "\n";
    std::cout << "Queries usadas:\n";
    std::cout << "- palabra: \"" << q_word << "\"\n";
    std::cout << "- frase:   \"" << q_phrase << "\"\n";
    std::cout << "- tag:     \"" << q_tag << "\"\n";
    std::cout << "(sink=" << sink << ")\n\n";

    std::cout << "| Operación             | Tamaño dataset | Tiempo (ms) | Notas              |\n";
    std::cout << "|----------------------|---------------:|------------:|-------------------|\n";
    std::cout << "| Carga dataset        | " << N << " películas | " << std::fixed << std::setprecision(3) << load_ms  << " | lectura + parsing |\n";
    std::cout << "| Build de índices     | " << N << " películas | " << std::fixed << std::setprecision(3) << build_ms << " | WordIndex + Ngram |\n";
    std::cout << "| Búsqueda (1 palabra) | " << N << " películas | " << std::fixed << std::setprecision(3) << word_ms  << " | promedio 10 runs  |\n";
    std::cout << "| Búsqueda (frase)     | " << N << " películas | " << std::fixed << std::setprecision(3) << phrase_ms<< " | promedio 10 runs  |\n";
    std::cout << "| Búsqueda `tag:...`   | " << N << " películas | " << std::fixed << std::setprecision(3) << tag_ms   << " | lookup por tag    |\n";

    std::cout << "\n=== FIN BENCHMARK ===\n\n";
    return 0;
}

int main(int argc, char** argv) {
    const std::string DEFAULT_DATA_FILE = "data/movies.csv";
    const std::string USERS_FILE = "users.txt";

    // Modo benchmark:
    //   ./tu_programa --bench
    //   ./tu_programa --bench data/otro.csv
    if (argc >= 2 && std::string(argv[1]) == "--bench") {
        std::string data_file = (argc >= 3) ? std::string(argv[2]) : DEFAULT_DATA_FILE;
        return run_benchmark(data_file);
    }

    unsigned threads = 0; // 0 = auto
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--threads" && i + 1 < argc) {
            threads = (unsigned)std::stoi(argv[++i]);
        }
    }
    // Modo normal (UI)
    StreamingPlatform platform;
    platform.setThreadCount(threads);

    if (!platform.loadDataset(DEFAULT_DATA_FILE)) {
        std::cerr << "ERROR: No se pudo leer el dataset: " << DEFAULT_DATA_FILE << "\n";
        return 1;
    }
    platform.buildIndexesParallel();

    UserStore users;
    users.load(USERS_FILE);

    UI ui(platform, users, USERS_FILE);
    ui.run();
    return 0;
}
