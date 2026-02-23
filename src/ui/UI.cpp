//
// Created by Benjamin Toro Leddihn on 21/02/26.
//

#include "../../include/ui/UI.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream> //se necesita para istringstream
#include "../../include/text/TextUtils.h"

using namespace std;

UI::UI(StreamingPlatform& platform, UserStore& users, const std::string& usersFile)
    : platform_(platform), users_(users), usersFile_(usersFile) {}

void UI::clear() { cout << "\x1B[2J\x1B[H" << flush; }

int UI::readInt() {
    int x;
    while (true) {
        cout << "> ";
        if (cin >> x) return x;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Entrada invalida. Intenta de nuevo.\n";
    }
}

string UI::readLineTrim() {
    string s;
    getline(cin, s);
    text::trim_in_place(s);
    return s;
}

void UI::pause() {
    cout << "Presiona ENTER para continuar...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

static string padCenter(const string& s, int width) {
    if ((int)s.size() >= width) return s.substr(0, width);
    int left = (width - (int)s.size()) / 2;
    int right = width - (int)s.size() - left;
    return string(left, ' ') + s + string(right, ' ');
}

static string truncate(const string& s, int maxlen) {
    if ((int)s.size() <= maxlen) return s;
    if (maxlen <= 1) return s.substr(0, maxlen);
    return s.substr(0, maxlen - 1) + "…";
}

void UI::renderProfilesRow() const {
    constexpr int BOX_W = 22;
    const string GAP = "  ";

    auto makeBox = [&](int idx, const string& title, const string& subtitle) {
        array<string, 5> lines;
        lines[0] = "+" + string(BOX_W - 2, '-') + "+";
        lines[1] = "|" + padCenter(to_string(idx) + ") " + title, BOX_W - 2) + "|";
        lines[2] = "|" + padCenter(subtitle, BOX_W - 2) + "|";
        lines[3] = "|" + string(BOX_W - 2, ' ') + "|";
        lines[4] = "+" + string(BOX_W - 2, '-') + "+";
        return lines;
    };

    array<array<string, 5>, UserStore::MAX_USERS> boxes;

    for (int i = 0; i < UserStore::MAX_USERS; i++) {
        int idx = i + 1;
        if (users_.has(i)) {
            const auto& u = users_.get(i);
            string title = truncate(u.name, 12);
            string sub = "Likes " + to_string(u.liked.size()) + " | WL " + to_string(u.watch_later.size());
            boxes[i] = makeBox(idx, title, truncate(sub, 18));
        } else {
            boxes[i] = makeBox(idx, "VACIO", "+ Crear usuario");
        }
    }

    for (int line = 0; line < 5; line++) {
        for (int b = 0; b < UserStore::MAX_USERS; b++) {
            cout << boxes[b][line];
            if (b != UserStore::MAX_USERS - 1) cout << GAP;
        }
        cout << "\n";
    }

    cout << "\nElige un perfil (1-4), 5 para borrar un perfil, o 0 para salir.\n";
    if (!users_.canCreate()) cout << "Cupo lleno: no se pueden crear mas usuarios.\n";
}

int UI::profilesScreen() {
    while (true) {
        clear();
        cout << "=== PROFILES ===\n\n";
        renderProfilesRow();

        int choice = readInt();
        if (choice == 0) return -1;

        if (choice == 5) {
            clear();
            cout << "Borrar perfil: elige slot (1-4) o 0 cancelar\n";
            int s = readInt();
            if (s >= 1 && s <= 4) {
                users_.remove(s - 1);
                users_.save(usersFile_);
            }
            continue;
        }

        if (choice < 1 || choice > 4) continue;

        int slot = choice - 1;

        if (users_.has(slot)) return slot;

        if (!users_.canCreate()) {
            cout << "No hay cupo para crear usuarios.\n";
            pause();
            continue;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        clear();
        cout << "=== Crear usuario (" << (users_.countUsers() + 1) << "/" << UserStore::MAX_USERS << ") ===\n";
        cout << "Nombre: ";
        string name = readLineTrim();
        users_.create(slot, name);
        users_.save(usersFile_);
        return slot;
    }
}

static bool isLiked(const User& u, const string& imdb) {
    return u.liked.find(imdb) != u.liked.end();
}
static bool inWatchLater(const User& u, const string& imdb) {
    return find(u.watch_later.begin(), u.watch_later.end(), imdb) != u.watch_later.end();
}

void UI::movieDetailScreen(int slot, int movie_id) {
    User& user = users_.get(slot);
    const Movie& m = platform_.movieById(movie_id);

    while (true) {
        clear();
        cout << "=== MOVIE ===\n\n";
        cout << "Title: " << m.title << "\n";
        cout << "IMDB:  " << m.imdb_id << "\n";
        cout << "Tags:  ";
        for (size_t i = 0; i < m.tags.size(); i++) {
            cout << m.tags[i] << (i + 1 < m.tags.size() ? ", " : "");
        }
        cout << "\n\n";

        cout << "Sinopsis:\n";
        cout << m.plot_synopsis << "\n\n";

        bool liked = isLiked(user, m.imdb_id);
        bool wl = inWatchLater(user, m.imdb_id);

        cout << "1) " << (liked ? "Quitar Like" : "Like") << "\n";
        cout << "2) " << (wl ? "Quitar de Ver mas tarde" : "Ver mas tarde") << "\n";
        cout << "0) Volver\n";

        int op = readInt();
        if (op == 0) break;

        if (op == 1) {
            if (!liked) user.liked.insert(m.imdb_id);
            else user.liked.erase(m.imdb_id);
            users_.save(usersFile_);
        } else if (op == 2) {
            if (!wl) user.watch_later.push_back(m.imdb_id);
            else {
                auto it = find(user.watch_later.begin(), user.watch_later.end(), m.imdb_id);
                if (it != user.watch_later.end()) user.watch_later.erase(it);
            }
            users_.save(usersFile_);
        }
    }
}

// Extrae ~120 chars alrededor del primer token del query encontrado en el texto.
// Muestra al usuario POR QUÉ apareció esa película en los resultados.
static string getSnippet(const string& text, const string& query) {
    if (query.empty() || text.empty()) return "";

    string textLow = text, queryLow = query;
    transform(textLow.begin(), textLow.end(), textLow.begin(),
        [](unsigned char c){ return tolower(c); });
    transform(queryLow.begin(), queryLow.end(), queryLow.begin(),
        [](unsigned char c){ return tolower(c); });

    // Probar cada token hasta encontrar uno en el texto
    istringstream iss(queryLow);
    string tok;
    while (iss >> tok) {
        size_t pos = textLow.find(tok);
        if (pos == std::string::npos) continue;
        size_t start = (pos > 40) ? pos - 40 : 0;
        return (start > 0 ? "..." : "")
             + text.substr(start, 120)
             + (start + 120 < text.size() ? "..." : "");
    }
    return "";
}

void UI::searchResultsScreen(int slot, const vector<SearchResult>& results, const string& title) {
    const int PAGE = 5;
    int page = 0;

    while (true) {
        clear();
        cout << "=== " << title << " ===\n";
        cout << "Total: " << results.size() << "\n\n";

        int start = page * PAGE;
        int end = min((int)results.size(), start + PAGE);

        if (start >= (int)results.size()) {
            cout << "No hay mas resultados.\n\n";
        } else {
            for (int i = start; i < end; i++) {
                const Movie& m = platform_.movieById(results[i].movie_id);
                cout << (i - start + 1) << ") " << m.title << "  [score=" << results[i].score << "]\n";
                
                // Mostrar dónde apareció el match - título o sinopsis
                bool inTitle = (m.title_norm.find(text::normalize_ascii(lastQuery_)) != string::npos);
                string snip = getSnippet(m.plot_synopsis, lastQuery_);
            
                if (inTitle)
                    cout << "   >> Match en titulo\n";
                else if (!snip.empty())
                    cout << "   >> \"" << snip << "\"\n";            
            }
            cout << "\n";
        }

        cout << "Opciones: (1-5) abrir, n siguiente, p anterior, 0 volver\n";
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        string cmd;
        getline(cin, cmd);
        text::trim_in_place(cmd);

        if (cmd == "0") return;
        if (cmd == "n") { if ((page + 1) * PAGE < (int)results.size()) page++; continue; }
        if (cmd == "p") { if (page > 0) page--; continue; }

        // número 1..5
        if (!cmd.empty() && all_of(cmd.begin(), cmd.end(), ::isdigit)) {
            int k = stoi(cmd);
            if (k >= 1 && k <= (end - start)) {
                int idx = start + (k - 1);
                movieDetailScreen(slot, results[idx].movie_id);
            }
        }
    }
}

void UI::listMoviesScreen(int slot, const vector<int>& movie_ids, const string& title) {
    vector<SearchResult> tmp;
    tmp.reserve(movie_ids.size());
    for (int id : movie_ids) tmp.push_back(SearchResult{id, 1.0});
    searchResultsScreen(slot, tmp, title);
}

void UI::searchScreen(int slot) {
    while (true) {
        clear();
        cout << "=== SEARCH ===\n";
        cout << "Buscar por palabra/frase/substring.\n";
        cout << "Tip: tag:horror para buscar por tag.\n\n";
        cout << "Query (vacío para volver): ";

        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        string q;
        getline(cin, q);
        text::trim_in_place(q);
        if (q.empty()) return;

        auto res = platform_.search(q);
        if (res.empty()) {
            cout << "\nNo se encontraron resultados.\n\n";
            pause();
            continue;
        }
        searchResultsScreen(slot, res, "SEARCH RESULTS");
    }
}

void UI::homeScreen(int slot) {
    User& u = users_.get(slot);

    while (true) {
        clear();
        cout << "=== HOME ===\n";
        cout << "Perfil: " << u.name << "\n\n";

        // Sección: recomendaciones
        auto rec = platform_.recommend(u, 5);
        cout << "Recomendaciones:\n";
        if (rec.empty()) cout << "  (sin recomendaciones)\n";
        else {
            for (size_t i = 0; i < rec.size(); i++)
                cout << "  " << (i+1) << ") " << platform_.movieById(rec[i]).title << "\n";
        }
        cout << "\n";

        // Sección: ver más tarde
        cout << "Ver mas tarde:\n";
        if (u.watch_later.empty()) cout << "  (vacio)\n";
        else {
            int shown = 0;
            for (size_t i = 0; i < u.watch_later.size() && shown < 5; i++, shown++) {
                int id = platform_.idByImdb(u.watch_later[i]);
                if (id >= 0) cout << "  " << (shown+1) << ") " << platform_.movieById(id).title << "\n";
            }
        }
        cout << "\n";

        // Sección: top tags + random 5
        auto tags = platform_.topTags(5);
        cout << "Categorias (top tags):\n";
        for (size_t i = 0; i < tags.size(); i++) cout << "  " << (i+1) << ") " << tags[i] << "\n";
        cout << "\n";

        cout << "Menu:\n";
        cout << "1) Ver recomendaciones (lista)\n";
        cout << "2) Ver ver-mas-tarde (lista)\n";
        cout << "3) Ver 5 random por categoria/tag\n";
        cout << "4) Buscar\n";
        cout << "9) Cambiar perfil\n";
        cout << "0) Salir\n";

        int op = readInt();
        if (op == 0) exit(0);
        if (op == 9) return;

        if (op == 1) {
            auto recFull = platform_.recommend(u, 50);
            listMoviesScreen(slot, recFull, "RECOMENDACIONES");
        } else if (op == 2) {
            vector<int> ids;
            for (auto& imdb : u.watch_later) {
                int id = platform_.idByImdb(imdb);
                if (id >= 0) ids.push_back(id);
            }
            listMoviesScreen(slot, ids, "VER MAS TARDE");
        } else if (op == 3) {
            clear();
            cout << "Elige categoria/tag (1-" << tags.size() << ") o 0 volver\n";
            int t = readInt();
            if (t == 0) continue;
            if (t < 1 || t > (int)tags.size()) continue;

            auto rnd = platform_.randomByTag(tags[t-1], 5);
            listMoviesScreen(slot, rnd, "RANDOM: " + tags[t-1]);
        } else if (op == 4) {
            searchScreen(slot);
        }
    }
}

void UI::run() {
    while (true) {
        int slot = profilesScreen();
        if (slot < 0) return;
        homeScreen(slot);
    }
}
