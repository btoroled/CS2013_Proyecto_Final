//
// Created by Benjamin Toro Leddihn on 21/02/26.
//

#ifndef INC_1_WORDINDEX_H
#define INC_1_WORDINDEX_H


#include <cstdint>
#include <unordered_map>
#include <vector>
#include <string>
#include "./Trie.h"

struct HitCount {
    uint16_t title = 0;
    uint16_t synopsis = 0;
    uint16_t tag = 0;
};

class WordIndex {
public:
    enum class Source { Title, Synopsis, Tag };

    void addToken(const std::string& token, int movie_id, Source src) {
        if (token.empty()) return;
        int term_id = trie_.insertAndGetId(token);
        if ((int)postings_.size() <= term_id) postings_.resize(term_id + 1);

        HitCount& h = postings_[term_id][movie_id];
        switch (src) {
            case Source::Title:   h.title++; break;
            case Source::Synopsis:h.synopsis++; break;
            case Source::Tag:     h.tag++; break;
        }
    }

    const std::unordered_map<int, HitCount>* lookup(const std::string& token) const {
        int term_id = trie_.findId(token);
        if (term_id < 0 || term_id >= (int)postings_.size()) return nullptr;
        return &postings_[term_id];
    }

private:
    Trie trie_;
    std::vector<std::unordered_map<int, HitCount>> postings_; // term_id -> {movie_id -> hitcount}
};

#endif //INC_1_WORDINDEX_H