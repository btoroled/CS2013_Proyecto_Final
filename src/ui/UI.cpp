//
// Created by Benjamin Toro Leddihn on 21/02/26.
//

#include "../../include/ui/UI.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>

#include "../../include/text/TextUtils.h"

using namespace std;

// ---------------------------
// Helpers
// ---------------------------

static void eatPendingNewline() {
    if (std::cin.peek() == '\n') std::cin.get();
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

static std::string repeatStr(const std::string& s, int n) {
    std::string out;
    if (n <= 0) return out;
    out.reserve(s.size() * (size_t)n);
    for (int i = 0; i < n; ++i) out += s;
    return out;
}

namespace ansi {
    static constexpr const char* RESET    = "\x1b[0m";
    static constexpr const char* BOLD     = "\x1b[1m";
    static constexpr const char* DIM      = "\x1b[2m";
    static constexpr const char* INV      = "\x1b[7m";
    static constexpr const char* RED      = "\x1b[31m";
    static constexpr const char* GRAY     = "\x1b[90m";
    static constexpr const char* RED_BOLD = "\x1b[1;31m";
}

static std::string styl(const std::string& s, const char* code) {
    return std::string(code) + s + ansi::RESET;
}

static std::vector<std::string> wrapText(const std::string& text, int width, int maxLines) {
    std::vector<std::string> out;
    if (width <= 10 || maxLines <= 0) return out;

    std::istringstream iss(text);
    std::string word, line;
    while (iss >> word) {
        if (line.empty()) line = word;
        else if ((int)line.size() + 1 + (int)word.size() <= width) line += " " + word;
        else {
            out.push_back(line);
            line = word;
            if ((int)out.size() >= maxLines) break;
        }
    }
    if ((int)out.size() < maxLines && !line.empty()) out.push_back(line);

    if ((int)out.size() == maxLines && iss.rdbuf()->in_avail() > 0) {
        out.back() = truncate(out.back(), std::max(1, width - 1)) + "…";
    }
    for (auto& s : out) s = truncate(s, width);
    return out;
}

static std::string joinTags(const std::vector<std::string>& tags, int maxCount = 4) {
    std::string out;
    for (int i = 0; i < (int)tags.size() && i < maxCount; i++) {
        if (!out.empty()) out += " · ";
        out += tags[i];
    }
    return out;
}

static std::array<std::string, 3> makeCard(const std::string& title, int w) {
    const int innerW = std::max(4, w - 2);
    std::string t = truncate(title, innerW);
    return {
        "┌" + repeatStr("─", innerW) + "┐",
        "│" + padCenter(t, innerW) + "│",
        "└" + repeatStr("─", innerW) + "┘"
    };
}

struct HomeRow {
    std::string title;
    std::vector<int> ids;
    int offset = 0;
};

static bool isLiked(const User& u, const string& imdb) {
    return u.liked.find(imdb) != u.liked.end();
}
static bool inWatchLater(const User& u, const string& imdb) {
    return find(u.watch_later.begin(), u.watch_later.end(), imdb) != u.watch_later.end();
}

// Extrae ~120 chars alrededor del primer token del query encontrado en el texto.
static string getSnippet(const string& text, const string& query) {
    if (query.empty() || text.empty()) return "";

    string textLow = text, queryLow = query;
    transform(textLow.begin(), textLow.end(), textLow.begin(),
        [](unsigned char c){ return (char)tolower(c); });
    transform(queryLow.begin(), queryLow.end(), queryLow.begin(),
        [](unsigned char c){ return (char)tolower(c); });

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

// ---------------------------
// UI methods
// ---------------------------

UI::UI(StreamingPlatform& platform, UserStore& users, const std::string& usersFile)
    : platform_(platform), users_(users), usersFile_(usersFile) {}

void UI::clear() {
#if defined(_WIN32)
    std::system("cls");
#else
    int rc = std::system("clear");
    if (rc != 0) {
        // fallback ANSI
        std::cout << "\x1B[2J\x1B[H";
    }
#endif
    std::cout.flush();
}

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
    eatPendingNewline();
    string dummy;
    getline(cin, dummy);
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



        if (choice == 5) {  // BORRAR
            clear();
            cout << "Borrar perfil: elige slot (1-4) o 0 cancelar\n";
            int s = readInt();
            if (s >= 1 && s <= 4) {
                history_.checkpoint(users_);
                users_.remove(s - 1);
                users_.save(usersFile_);
            }
            continue;
        }

        if (choice < 1 || choice > 4) continue;
        if (choice == 7) {
            if (history_.undo(users_)) users_.save(usersFile_);
            else { cout << "Nada para deshacer.\n"; pause(); }
        } else if (choice == 8) {
            if (history_.redo(users_)) users_.save(usersFile_);
            else { cout << "Nada para rehacer.\n"; pause(); }
        }

        int slot = choice - 1;

        if (users_.has(slot)) return slot;

        if (!users_.canCreate()) {
            cout << "No hay cupo para crear usuarios.\n";
            pause();
            continue;
        }

        eatPendingNewline();
        clear();
        cout << "=== Crear usuario (" << (users_.countUsers() + 1) << "/" << UserStore::MAX_USERS << ") ===\n";
        cout << "Nombre: ";
        string name = readLineTrim();
        history_.checkpoint(users_);
        users_.create(slot, name);
        users_.save(usersFile_);
        return slot;
    }
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
        string syn = m.plot_synopsis;
        if (syn.size() > 600)
            syn = syn.substr(0, 600) + "...\n[IMDB: https://www.imdb.com/title/" + m.imdb_id + "/]";
        cout << syn << "\n\n";

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
            history_.checkpoint(users_);
            users_.save(usersFile_);
        } else if (op == 2) {
            if (!wl) user.watch_later.push_back(m.imdb_id);
            else {
                auto it = find(user.watch_later.begin(), user.watch_later.end(), m.imdb_id);
                if (it != user.watch_later.end()) user.watch_later.erase(it);
            }
            history_.checkpoint(users_);
            users_.save(usersFile_);
        }
    }
}

void UI::searchResultsScreen(int slot,
    const vector<SearchResult>& results,
    const string& title,
    const string& query) {
    lastQuery_ = query;

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
            const string q_norm = text::normalize_ascii(lastQuery_);

            for (int i = start; i < end; i++) {
                const Movie& m = platform_.movieById(results[i].movie_id);
                cout << (i - start + 1) << ") " << m.title << "  [score=" << results[i].score << "]\n";

                if (!q_norm.empty()) {
                    bool inTitle = (m.title_norm.find(q_norm) != string::npos);
                    string snip = getSnippet(m.plot_synopsis, lastQuery_);

                    if (inTitle) cout << "   >> Match en titulo\n";
                    else if (!snip.empty()) cout << "   >> \"" << snip << "\"\n";
                }
            }
            cout << "\n";
        }

        cout << "Opciones: (1-5) abrir, n siguiente, p anterior, 0 volver\n";
        eatPendingNewline();
        string cmd;
        getline(cin, cmd);
        text::trim_in_place(cmd);

        if (cmd == "0") return;
        if (cmd == "n") { if ((page + 1) * PAGE < (int)results.size()) page++; continue; }
        if (cmd == "p") { if (page > 0) page--; continue; }

        bool allDigits = !cmd.empty() && std::all_of(cmd.begin(), cmd.end(), [](unsigned char ch){ return std::isdigit(ch); });
        if (allDigits) {
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
    searchResultsScreen(slot, tmp, title, "");
}

void UI::searchScreen(int slot) {
    while (true) {
        clear();
        cout << "=== SEARCH ===\n";
        cout << "Buscar por palabra/frase/substring.\n";
        cout << "Tip: tag:horror para buscar por tag.\n\n";
        cout << "Query (vacío para volver): ";

        eatPendingNewline();
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
        searchResultsScreen(slot, res, "SEARCH RESULTS", q);
    }
}

void UI::homeScreen(int slot) {
    User& u = users_.get(slot);

    constexpr int VISIBLE = 6;         // posters visibles por fila
    constexpr int CARD_W  = 18;        // ancho “poster"
    constexpr int ROWS_PER_TAB = 4;    // filas (categorías) visibles por pantalla

    int tab = 0;

    std::vector<HomeRow> rows;

    auto buildRows = [&] {
        rows.clear();

        rows.push_back(HomeRow{"Recomendado para ti", platform_.recommend(u, 80), 0});

        // Mi lista (ver más tarde)
        std::vector<int> wl;
        wl.reserve(u.watch_later.size());
        for (const auto& imdb : u.watch_later) {
            int id = platform_.idByImdb(imdb);
            if (id >= 0) wl.push_back(id);
        }
        rows.push_back(HomeRow{"Mi lista", wl, 0});

        // Me gusta
        std::vector<int> liked;
        liked.reserve(u.liked.size());
        for (const auto& imdb : u.liked) {
            int id = platform_.idByImdb(imdb);
            if (id >= 0) liked.push_back(id);
        }
        rows.push_back(HomeRow{"Me gusta", liked, 0});

        // Tags top (paginados por pestañas)
        auto tags = platform_.topTags(10);
        for (const auto& t : tags) {
            rows.push_back(HomeRow{t, platform_.randomByTag(t, 100), 0});
        }
    };

    auto tabCount = [&]() -> int {
        if (rows.empty()) return 1;
        return (int)((rows.size() + ROWS_PER_TAB - 1) / ROWS_PER_TAB);
    };

    auto clampSel = [&](int& r, int& c) {
        if (rows.empty()) { r = 0; c = 0; tab = 0; return; }

        r = std::max(0, std::min((int)rows.size() - 1, r));

        if (rows[r].ids.empty()) {
            c = 0;
            rows[r].offset = 0;
        } else {
            c = std::max(0, std::min((int)rows[r].ids.size() - 1, c));

            if (c < rows[r].offset) rows[r].offset = c;
            if (c >= rows[r].offset + VISIBLE) rows[r].offset = c - (VISIBLE - 1);

            int maxOff = std::max(0, (int)rows[r].ids.size() - VISIBLE);
            rows[r].offset = std::max(0, std::min(rows[r].offset, maxOff));
        }

        tab = r / ROWS_PER_TAB;
        tab = std::max(0, std::min(tab, tabCount() - 1));
    };

    auto render = [&](int selRow, int selCol) {
        // Header
        std::cout << styl("MOVIE", ansi::RED_BOLD)
                  << "   " << styl("Perfil:", ansi::DIM) << " " << styl(u.name, ansi::BOLD)
                  << "   " << styl("(/) Buscar  (P) Perfiles  (Q) Salir", ansi::BOLD)
                  << "\n" << styl(repeatStr("─", 100), ansi::BOLD) << "\n";

        std::cout << styl(
            "Pestaña " + std::to_string(tab + 1) + "/" + std::to_string(tabCount()) +
            "   (e y r para cambiar)",
            ansi::BOLD) << "\n\n";

        // Hero
        if (!rows.empty() && selRow >= 0 && selRow < (int)rows.size() && !rows[selRow].ids.empty()) {
            const Movie& m = platform_.movieById(rows[selRow].ids[selCol]);
            std::cout << styl(m.title, ansi::BOLD) << "\n";
            auto tags = joinTags(m.tags);
            if (!tags.empty()) std::cout << styl(tags, ansi::BOLD) << "\n";

            auto lines = wrapText(m.plot_synopsis, 98, 3);
            for (auto& l : lines) std::cout << l << "\n";
            std::cout << "\n";

            bool likedNow = (u.liked.find(m.imdb_id) != u.liked.end());
            bool wlNow = (std::find(u.watch_later.begin(), u.watch_later.end(), m.imdb_id) != u.watch_later.end());

            std::cout << styl("[Enter] Detalles", ansi::BOLD)
                      << "   " << styl(likedNow ? "[L] Quitar Like" : "[L] Like", ansi::BOLD)
                      << "   " << styl(wlNow ? "[M] Quitar Mi lista" : "[M] +Mi lista", ansi::BOLD)
                      << "   " << styl("[WASD] Mover", ansi::BOLD)
                      << "\n";
        } else {
            std::cout << styl("(No hay películas para mostrar)", ansi::BOLD) << "\n";
        }

        std::cout << "\n" << styl(repeatStr("─", 100), ansi::BOLD) << "\n";

        // Carruseles visibles por pestaña
        int startRow = tab * ROWS_PER_TAB;
        int endRow = std::min((int)rows.size(), startRow + ROWS_PER_TAB);

        for (int r = startRow; r < endRow; r++) {
            bool activeRow = (r == selRow);

            std::cout << (activeRow ? styl("▶ ", ansi::RED) : "  ")
                      << (activeRow ? styl(rows[r].title, ansi::BOLD) : rows[r].title)
                      << "\n";

            if (rows[r].ids.empty()) {
                std::cout << "  " << styl("(vacío)", ansi::BOLD) << "\n\n";
                continue;
            }

            int start = rows[r].offset;
            int end = std::min((int)rows[r].ids.size(), start + VISIBLE);

            std::vector<std::array<std::string, 3>> cards;
            cards.reserve(end - start);
            for (int i = start; i < end; i++) {
                const Movie& m = platform_.movieById(rows[r].ids[i]);
                cards.push_back(makeCard(m.title, CARD_W));
            }

            const std::string GAP = "  ";
            for (int line = 0; line < 3; line++) {
                std::cout << "  ";
                for (int i = 0; i < (int)cards.size(); i++) {
                    int globalIdx = start + i;
                    bool selected = activeRow && (globalIdx == selCol);
                    std::string chunk = cards[i][line];
                    if (selected) chunk = styl(chunk, ansi::INV);
                    std::cout << chunk << (i + 1 < (int)cards.size() ? GAP : "");
                }
                std::cout << "\n";
            }
            std::cout << "\n";
        }

        std::cout << styl("Comando:", ansi::BOLD) << " ";
    };

    buildRows();
    int selRow = 0, selCol = 0;
    clampSel(selRow, selCol);

    while (true) {
        clear();
        render(selRow, selCol);

        eatPendingNewline();
        std::string cmd;
        std::getline(std::cin, cmd);
        text::trim_in_place(cmd);

        // Enter = detalles
        if (cmd.empty()) {
            if (!rows.empty() && !rows[selRow].ids.empty()) {
                movieDetailScreen(slot, rows[selRow].ids[selCol]);
                buildRows();
                clampSel(selRow, selCol);
            }
            continue;
        }

        char c = (char)std::tolower((unsigned char)cmd[0]);

        if (c == 'q') std::exit(0);
        if (c == 'p') {Session::instance().logout(); return;} // cambiar perfil
        if (c == '/') {
            searchScreen(slot);
            buildRows();
            clampSel(selRow, selCol);
            continue;
        }

        if (c == 'e') {
            if (tab > 0) {
                tab--;
                selRow = tab * ROWS_PER_TAB;
                selCol = 0;
                clampSel(selRow, selCol);
            }
            continue;
        }
        if (c == 'r') {
            if (tab + 1 < tabCount()) {
                tab++;
                selRow = tab * ROWS_PER_TAB;
                selCol = 0;
                clampSel(selRow, selCol);
            }
            continue;
        }

        if (rows.empty()) continue;

        // mover dentro de la pestaña actual
        int startRow = tab * ROWS_PER_TAB;
        int endRow = std::min((int)rows.size(), startRow + ROWS_PER_TAB);

        if (c == 'w') {
            if (selRow > startRow) selRow--;
        } else if (c == 's') {
            if (selRow + 1 < endRow) selRow++;
        } else if (c == 'a') {
            selCol--;
        } else if (c == 'd') {
            selCol++;
        } else if (c == 'l') {
            if (!rows[selRow].ids.empty()) {
                const Movie& m = platform_.movieById(rows[selRow].ids[selCol]);
                if (u.liked.count(m.imdb_id)) u.liked.erase(m.imdb_id);
                else u.liked.insert(m.imdb_id);
                history_.checkpoint(users_);
                users_.save(usersFile_);
                buildRows();
            }
        } else if (c == 'm') {
            if (!rows[selRow].ids.empty()) {
                const Movie& m = platform_.movieById(rows[selRow].ids[selCol]);
                auto it = std::find(u.watch_later.begin(), u.watch_later.end(), m.imdb_id);
                if (it != u.watch_later.end()) u.watch_later.erase(it);
                else u.watch_later.push_back(m.imdb_id);
                history_.checkpoint(users_);
                users_.save(usersFile_);
                buildRows();
            }
        }

        clampSel(selRow, selCol);
    }
}



void UI::run() {
    auto& ses = Session::instance();

    while (true) {
        ses.logout();          // seguridad
        history_.clear();

        int slot = profilesScreen();
        if (slot < 0) return;

        if (!ses.login(users_, slot)) {
            // Si esto pasa, es porque alguien intentó loguear con sesión ocupada
            cout << "ERROR: ya hay una sesion activa.\n";
            pause();
            continue;
        }

        history_.checkpoint(users_); // estado base para poder undo
        homeScreen(slot);
    }
}
