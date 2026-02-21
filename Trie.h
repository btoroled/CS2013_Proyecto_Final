#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

// ============================================================
// TRIE — Árbol de prefijos para búsqueda rápida de subcadenas
// Cada nodo almacena un carácter; las hojas/nodos intermedios
// registran qué películas contienen ese prefijo.
// Justificación: O(m) búsqueda donde m = longitud del query,
// independientemente de N (número de películas).
// ============================================================
struct TrieNode {
    std::unordered_map<char, TrieNode*> children;
    std::unordered_set<int> movie_ids; // películas que pasan por este nodo
};

class Trie {
    TrieNode* root;

    // Normaliza: minúsculas, sin acentos básicos
    static char normalize(char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

public:
    Trie() : root(new TrieNode()) {}

    // Inserta todos los sufijos del texto (para substring search)
    // Optimizado: solo insertamos sufijos de palabras, no de cada char
    void insert(const std::string& text, int movie_id) {
        std::string norm;
        norm.reserve(text.size());
        for (char c : text) {
            char nc = normalize(c);
            norm += nc;
        }

        // Insertamos cada sufijo que inicia en una posición
        // Usamos ventana deslizante: insertamos desde cada inicio de substring
        int n = static_cast<int>(norm.size());
        for (int i = 0; i < n; ++i) {
            TrieNode* cur = root;
            for (int j = i; j < std::min(i + 30, n); ++j) { // max 30 chars por sufijo
                char c = norm[j];
                if (!cur->children.count(c))
                    cur->children[c] = new TrieNode();
                cur = cur->children[c];
                cur->movie_ids.insert(movie_id);
            }
        }
    }

    // Busca todas las películas que contienen el query como substring
    std::unordered_set<int> search(const std::string& query) const {
        TrieNode* cur = root;
        for (char c : query) {
            char nc = normalize(c);
            auto it = cur->children.find(nc);
            if (it == cur->children.end())
                return {};
            cur = it->second;
        }
        return cur->movie_ids;
    }

    ~Trie() {
        // BFS delete
        std::vector<TrieNode*> stack = {root};
        while (!stack.empty()) {
            TrieNode* n = stack.back(); stack.pop_back();
            for (auto& [c, child] : n->children)
                stack.push_back(child);
            delete n;
        }
    }
};
