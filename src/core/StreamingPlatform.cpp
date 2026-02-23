//
// Created by Benjamin Toro Leddihn on 21/02/26.
//

#include "../../include/core/StreamingPlatform.h"
#include <algorithm>
#include <fstream>
#include <random>
#include <unordered_set>
#include "../../include/text/TextUtils.h"

using namespace std;

static string join_norm_for_compact(const string& a_norm, const string& b_norm) {
    if (a_norm.empty()) return b_norm;
    if (b_norm.empty()) return a_norm;
    return a_norm + " " + b_norm;
}

int StreamingPlatform::idByImdb(const std::string& imdb_id) const {
    auto it = imdb_to_id_.find(imdb_id);
    return (it == imdb_to_id_.end()) ? -1 : it->second;
}

bool StreamingPlatform::loadDatasetTSV(const std::string& path) {
    auto rows = parseSeparatedFile(path, ',');
    if (rows.empty()) return false;

    // header esperado: imdb_id title plot_synopsis tags split synopsis_source
    size_t start = 0;
    if (!rows.empty() && !rows[0].empty() && rows[0][0] == "imdb_id") start = 1;

    movies_.clear();
    imdb_to_id_.clear();
    tag_to_movies_.clear();

    int id = 0;
    for (size_t r = start; r < rows.size(); r++) {
        auto& row = rows[r];
        if (row.size() < 6) continue;

        Movie m;
        m.id = id++;
        m.imdb_id = row[0];
        m.title = row[1];
        m.plot_synopsis = row[2];
        string tags_raw = row[3];
        m.split = row[4];
        m.synopsis_source = row[5];

        // tags
        auto rawTags = text::split_tags_raw(tags_raw);
        for (auto& t : rawTags) {
            string tn = text::normalize_tag(t);
            if (!tn.empty()) m.tags.push_back(tn);
        }

        // normalización
        m.title_norm = text::normalize_ascii(m.title);
        m.synopsis_norm = text::normalize_ascii(m.plot_synopsis);
        m.compact = text::compact_no_spaces(join_norm_for_compact(m.title_norm, m.synopsis_norm));

        imdb_to_id_[m.imdb_id] = m.id;

        // tag index base
        for (auto& t : m.tags) tag_to_movies_[t].push_back(m.id);

        movies_.push_back(std::move(m));
    }

    return !movies_.empty();
}

void StreamingPlatform::buildIndexes() {
    wordIndex_ = WordIndex{};
    ngramIndex_ = NgramIndex{3};

    for (const auto& m : movies_) {
        // words from title
        for (auto& w : text::split_words(m.title_norm))
            wordIndex_.addToken(w, m.id, WordIndex::Source::Title);

        // words from synopsis
        for (auto& w : text::split_words(m.synopsis_norm))
            wordIndex_.addToken(w, m.id, WordIndex::Source::Synopsis);

        // tags
        for (auto& t : m.tags)
            wordIndex_.addToken(t, m.id, WordIndex::Source::Tag);

        // substring index (trigramas) sobre title+synopsis compact
        ngramIndex_.addTextCompact(m.compact, m.id);
    }

    ngramIndex_.finalize();
}

vector<SearchResult> StreamingPlatform::search(const std::string& user_input) const {
    string input = user_input;
    text::trim_in_place(input);
    if (input.empty()) return {};

    // soporte: tag:horror
    // (normalizamos la parte del tag)
    if (text::starts_with(input, "tag:") || text::starts_with(input, "TAG:")) {
        string tagRaw = input.substr(4);
        text::trim_in_place(tagRaw);
        string tag = text::normalize_tag(tagRaw);

        vector<SearchResult> res;
        auto it = tag_to_movies_.find(tag);
        if (it == tag_to_movies_.end()) return res;

        res.reserve(it->second.size());
        for (int id : it->second) {
            SearchResult sr; sr.movie_id = id; sr.score = 100.0; // tag match fuerte
            res.push_back(sr);
        }

        // desempate por título
        sort(res.begin(), res.end(), [&](const auto& a, const auto& b){
            if (a.score != b.score) return a.score > b.score;
            return movies_[a.movie_id].title < movies_[b.movie_id].title;
        });
        return res;
    }

    // normalizar consulta
    string q_norm = text::normalize_ascii(input);
    auto tokens = text::split_words(q_norm);

    // acumuladores
    unordered_map<int, HitCount> acc;
    unordered_map<int, int> matchedTokens; // movie_id -> cuántos tokens pegó

    // OR por tokens
    for (const auto& tk : tokens) {
        const auto* post = wordIndex_.lookup(tk);
        if (!post) continue;

        for (const auto& [movie_id, hit] : *post) {
            HitCount& a = acc[movie_id];
            a.title    += hit.title;
            a.synopsis += hit.synopsis;
            a.tag      += hit.tag;

            matchedTokens[movie_id]++; // cuenta match por token (aprox)
        }
    }

    // substring (solo cuando es 1 token, tipo "bar")
    unordered_set<int> substringOK;
    string q_compact = text::compact_no_spaces(q_norm);

    if (tokens.size() == 1 && (int)q_compact.size() >= ngramIndex_.n()) {
        auto cand = ngramIndex_.candidatesForCompactQuery(q_compact);
        for (int id : cand) {
            if (movies_[id].compact.find(q_compact) != string::npos) {
                substringOK.insert(id);
                // si no entró por wordIndex, igual debe aparecer en resultados
                acc.try_emplace(id, HitCount{});
            }
        }
    } else if (tokens.size() == 1 && (int)q_compact.size() > 0 && (int)q_compact.size() < ngramIndex_.n()) {
        // fallback para query muy corta (1-2 chars): escaneo simple (dataset pequeño)
        for (const auto& m : movies_) {
            if (m.compact.find(q_compact) != string::npos) {
                substringOK.insert(m.id);
                acc.try_emplace(m.id, HitCount{});
            }
        }
    }

    // construir resultados
    vector<SearchResult> results;
    results.reserve(acc.size());

    const int needAll = (int)tokens.size();

    for (const auto& [movie_id, hit] : acc) {
        bool hasAll = (needAll <= 1) ? true : (matchedTokens[movie_id] >= needAll);
        bool sub = (substringOK.find(movie_id) != substringOK.end());

        double score =
            W_TITLE * hit.title +
            W_TAG   * hit.tag +
            W_SYN   * hit.synopsis +
            (hasAll ? BONUS_ALL_TOKENS : 0.0) +
            (sub    ? BONUS_SUBSTRING  : 0.0);

        if (score <= 0.0) continue;

        results.push_back(SearchResult{movie_id, score});
    }

    sort(results.begin(), results.end(), [&](const SearchResult& a, const SearchResult& b){
        if (a.score != b.score) return a.score > b.score;
        return movies_[a.movie_id].title < movies_[b.movie_id].title;
    });

    return results;
}

vector<int> StreamingPlatform::recommend(const User& u, int k) const {
    // Métrica 1: Perfil de tags basado en likes
    unordered_map<string, int> tagFreq;
    
    //Métrica 2: palabras frecuentes en títulos likeados
    unordered_map<string, int> wordFreq;

    for (const auto& imdb : u.liked) {
        int id = idByImdb(imdb);
        if (id < 0) continue;
        // acumular tags
        for (const auto& t : movies_[id].tags)
            tagFreq[t]++;
        // acumular palabras del título
        for (const auto& w : text::split_words(movies_[id].title_norm))
            if (w.size() >= 3) wordFreq[w]++; // ignorar palabras muy cortas
    }

    
    // si no hay likes, recomendar random global
    if (tagFreq.empty()) {
        vector<int> all;
        all.reserve(movies_.size());
        for (const auto& m : movies_) all.push_back(m.id);
        std::mt19937 rng((unsigned)std::random_device{}());
        std::shuffle(all.begin(), all.end(), rng);
        if ((int)all.size() > k) all.resize(k);
        return all;
    }

    // Excluir liked y watch_later
    unordered_set<string> excluded = u.liked;
    for (const auto& imdb : u.watch_later)
        excluded.insert(imdb);

    //Scoring: 
    //score = Σ tagFreq[tag] * 2  (peso mayor)
    //        + Σ wordFreq[word]    (bonus por palabras del título)

    vector<pair<int,double>> scored;
    scored.reserve(movies_.size());

    for (const auto& m : movies_) {
        if (excluded.count(m.imdb_id)) continue;

        double s = 0.0;

        // componente 1: similitud por tags
        for (const auto& t : m.tags) {
            auto it = tagFreq.find(t);
            if (it != tagFreq.end()) s += it->second*2.0;
        }

        // componente 2: bonus por palabras frecuentes en título
        for (const auto& w : text::split_words(m.title_norm)) {
            auto it = wordFreq.find(w);
            if (it != wordFreq.end()) s += it->second * 1.0;
        }
        
        if (s > 0.0) scored.push_back({m.id, s});
    }

    sort(scored.begin(), scored.end(), [&](auto& a, auto& b){
        if (a.second != b.second) return a.second > b.second;
        return movies_[a.first].title < movies_[b.first].title;
    });

    vector<int> out;
    for (int i = 0; i < (int)scored.size() && (int)out.size() < k; i++)
        out.push_back(scored[i].first);
    return out;
}

vector<string> StreamingPlatform::topTags(int k) const {
    vector<pair<string,int>> v;
    v.reserve(tag_to_movies_.size());
    for (const auto& [tag, ids] : tag_to_movies_) v.push_back({tag, (int)ids.size()});

    sort(v.begin(), v.end(), [](const auto& a, const auto& b){
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
    });

    vector<string> out;
    for (int i = 0; i < (int)v.size() && (int)out.size() < k; i++)
        out.push_back(v[i].first);
    return out;
}

vector<int> StreamingPlatform::randomByTag(const std::string& tag_in, int k) const {
    string tag = text::normalize_tag(tag_in);
    auto it = tag_to_movies_.find(tag);
    if (it == tag_to_movies_.end()) return {};

    vector<int> ids = it->second;
    std::mt19937 rng((unsigned)std::random_device{}());
    std::shuffle(ids.begin(), ids.end(), rng);

    if ((int)ids.size() > k) ids.resize(k);
    return ids;
}

// Parser genérico (CSV/TSV) con quotes y multilinea
vector<vector<string>> StreamingPlatform::parseSeparatedFile(const std::string& path, char delim) {
    ifstream in(path, ios::binary);
    if (!in) return {};

    vector<vector<string>> rows;
    vector<string> row;
    string field;
    bool inQuotes = false;

    char c;
    while (in.get(c)) {
        if (c == '\r') continue;

        if (c == '"') {
            if (inQuotes) {
                if (in.peek() == '"') { // escaped quote
                    field.push_back('"');
                    in.get();
                } else {
                    inQuotes = false;
                }
            } else {
                inQuotes = true;
            }
            continue;
        }

        if (!inQuotes && c == delim) {
            row.push_back(field);
            field.clear();
            continue;
        }

        if (!inQuotes && c == '\n') {
            row.push_back(field);
            field.clear();
            rows.push_back(row);
            row.clear();
            continue;
        }

        field.push_back(c);
    }

    // last
    if (!field.empty() || !row.empty()) {
        row.push_back(field);
        rows.push_back(row);
    }

    return rows;
}
