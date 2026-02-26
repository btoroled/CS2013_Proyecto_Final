#ifndef INC_1_TEXTUTILS_H
#define INC_1_TEXTUTILS_H

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>
#include <regex>

namespace text {

void ltrim_in_place(std::string& s);
void rtrim_in_place(std::string& s);
void trim_in_place(std::string& s);

bool starts_with(std::string_view s, std::string_view prefix);

// Normaliza a ASCII “suave”:
// - lower
// - letras/dígitos se conservan
// - todo lo demás se convierte en espacio
// - colapsa espacios múltiples
std::string normalize_ascii(std::string_view in);

// split por espacios (asumiendo entrada ya normalizada)
std::vector<std::string> split_words(std::string_view norm);

// elimina espacios y deja solo [a-z0-9] (en ASCII) — útil para trigramas/substrings
std::string compact_no_spaces(std::string_view norm);

// normaliza tags a formato estable (lower + '_' como separador)
std::string normalize_tag(std::string tagRaw);

// Soporta formatos típicos de tags:
// - "tag1,tag2"
// - "[\"tag1\", \"tag2\"]"
// - "['tag1','tag2']"
std::vector<std::string> split_tags_raw(std::string_view raw);


void remove_meta_text(std::string& text);

// genera resumen compacto con N palabras desde un texto normalizado
std::string generate_compact_summary(std::string_view norm, int limitWords = 30);

} // namespace text

#endif
