# Proyecto Final — Plataforma de Streaming (Búsqueda tipo Netflix)

## A. Setup del proyecto
- [X] Crear repo (GitHub) con estructura:
  - [X] `src/`
  - [X] `include/`
  - [X] `data/`
  - [X] `tests/`
  - [X] `docs/`
- [X] Definir estándar de compilación (C++17 o C++20) + `CMakeLists.txt`
- [ ] Crear “modo demo” (main) con menú básico (sin búsquedas aún)


---

## B. Carga y preprocesamiento del dataset
- [ ] Descargar dataset y guardarlo en `data/`
- [ ] Confirmar separador real (en tu caso es **TSV por tabs**)
- [ ] Implementar `Catalog::load(file)` que cargue las 1428 filas
- [ ] Implementar preprocesamiento:
  - [ ] `normalize(text)` → minúsculas + limpiar signos + colapsar espacios
  - [ ] `split_tags("cult, horror,...")` → `vector<string>` con trim
  - [ ] Guardar por movie:
    - [ ] `title_norm`
    - [ ] `synopsis_norm`
    - [ ] `text_compact` (sin espacios) para substring/verify
- [ ] Log de validación: imprimir 3 películas + contar tags


---

## C. Modelo de datos (clases ADT)
- [ ] Clase `Movie` (id, imdb_id, title, synopsis, tags, campos normalizados)
- [ ] Clase `UserState` (likes, ver_mas_tarde)
- [ ] Clase `SearchResult` (movie_id, score, razones opcional)


---

## D. Índice principal: búsqueda por palabra/frase (Trie de tokens)
- [ ] Implementar `TrieNode` (nodos por **caracter**, children, terminal)
- [ ] Implementar `TrieIndexWords`:
  - [ ] `insert(word, movie_id, source)` donde `source = title|synopsis|tag`
  - [ ] `find(word)` → retorna lista de películas donde aparece
- [ ] Construcción del índice:
  - [ ] Tokenizar `title_norm` y `synopsis_norm`
  - [ ] Indexar tags como tokens
- [ ] Estructura de postings para ranking (contadores por movie)


---

## E. Índice para substring: búsqueda por string “bar” (Trie de n-gramas)
- [ ] Elegir n (recomendado **3**: trigramas)
- [ ] Implementar `TrieIndexNgrams`:
  - [ ] `insert(ngram, movie_id)`
  - [ ] `find(ngram)` → candidatos
- [ ] Query substring:
  - [ ] generar trigramas de la query (si query < 3, manejar caso especial)
  - [ ] intersectar candidatos
  - [ ] verificar substring real con `text_compact.find(query_compact)`

---

## F. Motor de búsqueda (interpreta query y combina resultados)
- [ ] Parsear query del usuario:
  - [ ] si tiene espacios → es “frase” (pero el filtro es OR con bonus AND)
  - [ ] si no tiene espacios → palabra
- [ ] Combinar:
  - [ ] OR por tokens (unión de resultados)
  - [ ] bonus si contiene **todas** las palabras (AND)
- [ ] Buscar tags si el usuario escribe `tag:horror` (o una opción del menú)


---

## G. Ranking (importancia) + paginación 5 en 5
- [ ] Definir scoring (simple pero defendible), por ejemplo:
  - [ ] `title_hit > tag_hit > synopsis_hit`
  - [ ] bonus por AND
  - [ ] bonus por substring match
- [ ] Ordenar resultados por score desc (y empate por título o imdb_id)
- [ ] Implementar paginación:
  - [ ] mostrar 5
  - [ ] `n` siguiente 5, `p` anterior, `q` salir


---

## H. Vista de película + acciones (Like / Ver más tarde)
- [ ] Opción “seleccionar película #k” desde resultados
- [ ] Mostrar:
  - [ ] título
  - [ ] sinopsis
  - [ ] tags
- [ ] Acciones:
  - [ ] `Like` (toggle)
  - [ ] `Ver más tarde` (agregar/retirar)


---

## I. Persistencia del usuario (para que al iniciar se mantenga)
- [ ] Guardar `UserState` a archivo (`user_state.txt` o JSON simple)
- [ ] Cargar al iniciar
- [ ] Al iniciar, mostrar:
  - [ ] lista “Ver más tarde”
  - [ ] recomendados basados en likes


---

## J. Recomendaciones “similares a likes”
- [ ] Definir algoritmo propio (simple y justificable):
  - [ ] Similaridad por tags (Jaccard) con el “perfil” de likes
  - [ ] + bonus por palabras frecuentes en likes
- [ ] Mostrar top N recomendadas al inicio (que no estén ya en likes/watch later)


---

## K. Pruebas y casos borde
- [ ] Query vacía / solo espacios
- [ ] Query con símbolos raros
- [ ] Query corta (1–2 letras) para substring
- [ ] Películas sin synopsis/tags (si existieran)
- [ ] Rendimiento: construir índices y buscar rápido


---

## L. Documentación obligatoria en el repo
- [ ] Explicar por qué elegiste:
  - [ ] Trie de tokens + Trie de n-gramas (vs suffix tree completo)
- [ ] Complejidad aproximada:
  - [ ] construcción
  - [ ] búsqueda palabra/frase
  - [ ] búsqueda substring
- [ ] Cómo se corre el proyecto (comandos)
- [ ] Ejemplos de búsqueda (capturas o logs)

