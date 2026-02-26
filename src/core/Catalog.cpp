#include "../../include/core/Catalog.h"

bool Catalog::parse_csv_row(std::istream& in, std::vector<std::string>& row) {
    row.clear();
    std::string field;
    bool in_quotes = false;
    char c;

    while (in.get(c)) {
        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (c == ',' && !in_quotes) {
            row.push_back(field);
            field.clear();
        } else if (c == '\n' && !in_quotes) {
            row.push_back(field);
            return true;
        } else if (c != '\r') {
            field += c;
        }
    }
    if (!field.empty() || !row.empty()) {
        row.push_back(field);
        return true;
    }
    return false;
}

bool Catalog::load(const std::string& file) {
    std::ifstream in(file);
    if (!in.is_open()) return false;

    std::vector<std::string> row;
    parse_csv_row(in, row);

    int current_id = 0;
    while (parse_csv_row(in, row)) {
        if (row.size() < 6) continue;

        Movie m;
        m.id = current_id++;
        m.imdb_id = row[0];
        m.title = row[1];
        m.plot_synopsis = row[2];

        std::string raw_tags = row[3];
        m.split = row[4];
        m.synopsis_source = row[5];

        // Normalizar Título
        m.title_norm = text::normalize_ascii(m.title);

        // Limpiar Meta-texto y normalizar Sinopsis
        std::string temp_synopsis = m.plot_synopsis;
        text::remove_meta_text(temp_synopsis);
        m.synopsis_norm = text::normalize_ascii(temp_synopsis);

        // Generar 'compact'
        m.compact = text::generate_compact_summary(m.synopsis_norm);

        // Procesar Tags
        std::vector<std::string> parsed_tags_raw = text::split_tags_raw(raw_tags);
        for (const auto& tag : parsed_tags_raw) {
            std::string clean_tag = text::normalize_tag(tag);
            if (!clean_tag.empty()) {
                m.tags.push_back(clean_tag);
            }
        }

        base_datos.push_back(m);
    }
    in.close();
    return true;
}

void Catalog::printValidation() {
    std::cout << "Validacion: " << std::endl;
    std::cout << "Peliculas cargadas exitosamente: " << base_datos.size() << std::endl;

    if (!base_datos.empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(0, base_datos.size() - 1);

        std::cout << "---------------------------------------------" << std::endl;
        std::cout << "Mostrando 3 casos al azar:" << std::endl;
        int limit = std::min(3, (int)base_datos.size());

        for (int i = 0; i < limit; i++) {
            int idx = distrib(gen);
            const Movie& m = base_datos[idx];

            std::cout << std::endl;
            std::cout << "[" << i + 1 << "] ID Interno: " << m.id << " | IMDB: " << m.imdb_id << std::endl;
            std::cout << "Original Title : " << m.title << std::endl;
            std::cout << "Title Norm     : " << m.title_norm << std::endl;
            std::cout << "Tags (" << m.tags.size() << ")      : ";
            for (const auto& t : m.tags) std::cout << t << " ";
            std::cout << std::endl << "Compact (30w)  : " << m.compact << "..." << std::endl;
        }
    }

    int total_tags = 0;
    std::cout << "---------------------------------------------" << std::endl;
    for (const auto& m : base_datos) total_tags += m.tags.size();
    std::cout << "Total de Tags en sistema: " << total_tags << std::endl;
}