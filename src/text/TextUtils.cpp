//
// Created by Benjamin Toro Leddihn on 21/02/26.
//

#include "../../include/text/TextUtils.h"

#include <algorithm>
#include <cctype>
#include <regex>

void text::ltrim_in_place(std::string& s) {
    size_t i = 0;
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) i++;
    s.erase(0, i);
}

void text::rtrim_in_place(std::string& s) {
    if (s.empty()) return;
    size_t i = s.size();
    while (i > 0 && std::isspace(static_cast<unsigned char>(s[i - 1]))) i--;
    s.erase(i);
}

void text::trim_in_place(std::string& s) {
    ltrim_in_place(s);
    rtrim_in_place(s);
}

bool text::starts_with(std::string_view s, std::string_view prefix) {
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

std::string text::normalize_ascii(std::string_view in) {
    std::string out;
    out.reserve(in.size());

    bool lastSpace = true;
    for (unsigned char ch : in) {
        if (ch < 128) {
            if (std::isalnum(ch)) {
                out.push_back(static_cast<char>(std::tolower(ch)));
                lastSpace = false;
            } else {
                if (!lastSpace) {
                    out.push_back(' ');
                    lastSpace = true;
                }
            }
        } else {
            // Si viene UTF-8 (tildes, etc.), lo tratamos como separador.
            if (!lastSpace) {
                out.push_back(' ');
                lastSpace = true;
            }
        }
    }

    if (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

std::vector<std::string> text::split_words(std::string_view norm) {
    std::vector<std::string> out;
    std::string cur;

    for (char c : norm) {
        if (c == ' ') {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

std::string text::compact_no_spaces(std::string_view norm) {
    std::string out;
    out.reserve(norm.size());
    for (unsigned char ch : norm) {
        if (ch < 128 && std::isalnum(ch)) out.push_back(static_cast<char>(ch));
    }
    return out;
}

std::string text::normalize_tag(std::string tagRaw) {
    trim_in_place(tagRaw);
    if (tagRaw.empty()) return {};

    auto strip_quotes = [](std::string& s) {
        while (!s.empty() && (s.front() == '\'' || s.front() == '"')) s.erase(s.begin());
        while (!s.empty() && (s.back() == '\'' || s.back() == '"')) s.pop_back();
    };
    strip_quotes(tagRaw);
    trim_in_place(tagRaw);

    std::string out;
    out.reserve(tagRaw.size());

    bool lastUnd = true;
    for (unsigned char ch : tagRaw) {
        if (ch < 128) {
            if (std::isalnum(ch)) {
                out.push_back(static_cast<char>(std::tolower(ch)));
                lastUnd = false;
            } else {
                if (!lastUnd) {
                    out.push_back('_');
                    lastUnd = true;
                }
            }
        } else {
            if (!lastUnd) {
                out.push_back('_');
                lastUnd = true;
            }
        }
    }

    while (!out.empty() && out.back() == '_') out.pop_back();
    while (!out.empty() && out.front() == '_') out.erase(out.begin());
    return out;
}

std::vector<std::string> text::split_tags_raw(std::string_view raw) {
    std::string s(raw);
    trim_in_place(s);
    if (s.empty()) return {};

    // Caso JSON-ish: ["tag1", "tag2"] / ['tag1','tag2']
    if (!s.empty() && (s.front() == '[' && s.back() == ']')) {
        std::vector<std::string> out;
        std::string cur;
        bool inQ = false;
        char q = 0;

        for (char c : s) {
            if (!inQ) {
                if (c == '\'' || c == '"') { inQ = true; q = c; cur.clear(); }
            } else {
                if (c == q) {
                    inQ = false;
                    trim_in_place(cur);
                    if (!cur.empty()) out.push_back(cur);
                    cur.clear();
                } else {
                    cur.push_back(c);
                }
            }
        }
        if (!out.empty()) return out;
    }

    // Caso CSV-ish: tag1,tag2;tag3|tag4
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == ',' || c == ';' || c == '|') {
            trim_in_place(cur);
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    trim_in_place(cur);
    if (!cur.empty()) out.push_back(cur);
    return out;
}

void text::remove_meta_text(std::string& text) {
    // Ejemplos: "Note:", "Warning:", "This synopsis ..." etc.
    static const std::regex metaRegex(
        "^(?:Note:|Warning:|This synopsis).*?(?:\\.|- )",
        std::regex_constants::icase
    );
    text = std::regex_replace(text, metaRegex, "");
}

std::string text::generate_compact_summary(std::string_view norm, int limitWords) {
    std::vector<std::string> words = split_words(norm);
    std::string out;

    int count = std::min(limitWords, (int)words.size());
    for (int i = 0; i < count; i++) {
        out += words[i];
        if (i < count - 1) out += " ";
    }
    return out;
}