# Programación III — Proyecto Final (2026-0)
Plataforma de streaming (CLI) con **búsqueda eficiente** y **recomendaciones** usando estructuras de datos (Trie, índices invertidos y n-grams).

---

## Integrantes
- **Benjamin Toro Leddihn**
- **Hector Miguel Espinoza Torres**
- **Leonardo Medina Gago**

VIDEO: https://youtu.be/G5WTTPlChWQ

---

## Descripción del proyecto
Este proyecto implementa una plataforma tipo “streaming” en consola que permite:
- Crear y gestionar hasta **4 perfiles de usuario**.
- Guardar películas en **Like** y **Ver más tarde**.
- Buscar películas por:
    - **Palabras / frases** (título, sinopsis, tags).
    - **Tags** con prefijo `tag:`.
    - **Substrings** (búsqueda aproximada por fragmentos usando n-grams).
- Generar **recomendaciones** personalizadas a partir del comportamiento del usuario.

El objetivo principal es demostrar el uso de **estructuras eficientes** para búsquedas a gran escala y una organización limpia del código en módulos.

---

## Estructura del proyecto
```txt
include/
  core/     -> Movie, User, StreamingPlatform, UserStore, Session, UserHistory
  index/    -> Trie, WordIndex, NgramIndex
  ui/       -> UI (menús por consola)
  text/     -> TextUtils (normalización y parsing)
src/
  core/     -> StreamingPlatform.cpp, UserStore.cpp, Session.cpp
  index/
  ui/
  text/
data/
  movies.csv
users.txt   -> se genera automáticamente (persistencia de perfiles / snapshots)
main.cpp
CMakeLists.txt
README.md
```
---
## Cómo se ejecuta

### Requisitos
- **CMake** (recomendado ≥ 3.16)
- Compilador con soporte **C++20**
    - macOS: AppleClang
    - Linux: g++ / clang++

### Compilación y ejecución (terminal)
Desde la raíz del repositorio:

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j 8
./1
```

> Nota: Si el nombre del ejecutable no es `1`, ejecútalo con el nombre que aparezca en tu configuración de CMake/CLion.

### Ejecución en CLion
1. Abrir el proyecto en CLion
2. Esperar el reload de CMake (si lo pide)
3. Build & Run

---

## Dataset

### Archivo esperado
El proyecto trabaja con:

```txt
data/movies.csv
```

### Formato (6 columnas)
El loader espera 6 columnas en este orden:

1. `imdb_id`
2. `title`
3. `plot_synopsis`
4. `tags`
5. `split`
6. `synopsis_source`

Ejemplo (resumido):

```csv
imdb_id,title,plot_synopsis,tags,split,synopsis_source
tt1234567,Movie Title,"A long synopsis...","['Drama','Comedy']",train,imdb
```

---
## Uso del programa (flujo general)

### 1) Pantalla de perfiles / Sesión
- Se muestran **4 slots** (máximo 4 usuarios).
- Solo puede existir **1 usuario activo por sesión** (enforced por `Session`).
- Para **cambiar de perfil**, primero se **cierra sesión** (logout) y se selecciona otro slot.

### 2) Home
- Muestra recomendaciones personalizadas.
- Muestra lista “Ver más tarde”.
- Muestra categorías (top tags).
- Acceso a búsqueda.

### 3) Búsqueda
La búsqueda soporta:
- **Búsqueda normal**
  - Ej: `batman`, `space travel`, `love story`
- **Búsqueda por tag**
  - Ej: `tag:horror`, `tag:action`
- **Búsqueda por fragmentos (substring)**
  - Soportada por `NgramIndex` (n-grams).

---

## Persistencia de usuarios (`users.txt`)

Los perfiles y sus listas se guardan automáticamente en un archivo de texto versionado.

Características:
- Máximo **4 usuarios**
- Guarda:
  - nombre
  - imdb_ids de `liked` (set)
  - imdb_ids de `watch_later` (mantiene orden)
- **Formato versionado** (para robustez al leer/escribir)

**Evidencia en código (snippet):**
``cpp
// UserStore::save(...)
out << "VERSION 1\n";
out << "SLOTS " << MAX_USERS << "\n";
´´

---

## Patrones de diseño

> Patrones **GoF implementados**: **Singleton**, **State**, **Memento**.  
> Patrones/estilos **arquitectónicos aplicados**: **Facade**, **Repository**, **Dependency Injection**.

---

### 1) Singleton — Sesión única global (`Session`)
**Qué resuelve:** garantiza una sola sesión activa en toda la app.

**Dónde:** `include/core/Session.h`, `src/core/Session.cpp`

**Snippet:**
cpp
// Session.h
static Session& instance();
Session(const Session&) = delete;
Session& operator=(const Session&) = delete;

// Session.cpp
Session& Session::instance() {
static Session s;
return s;
}

### 2) State (GoF) — LoggedOut / LoggedIn controlan el login

Qué resuelve: el comportamiento de login() cambia según estado:

LoggedOut permite login

LoggedIn bloquea doble-login (cumple el enunciado)

Dónde: include/core/Session.h, src/core/Session.cpp

// Session.h
struct State {
virtual bool login(Session&, UserStore&, int) = 0;
virtual void logout(Session&) = 0;
virtual bool logged() const = 0;
};

// Session.cpp  (clave: bloquea doble login)
bool login(Session&, UserStore&, int) override {
return false;
}

### 3) Memento (GoF) — Snapshot/Restore de usuarios + Caretaker (Undo/Redo)

Qué resuelve: capturar/restaurar el estado completo de UserStore para deshacer/rehacer cambios.

Roles GoF:

Originator: UserStore

Memento: UserStore::Memento (snapshot)

Caretaker: UserHistory (stacks undo/redo)

Dónde: include/core/UserHistory.h (+ UserStore para snapshot/restore)

Snippet (Caretaker):

// UserHistory.h
void checkpoint(const UserStore& store) {
undo_.push_back(store.snapshot());
redo_.clear();
}

bool undo(UserStore& store) {
if (undo_.empty()) return false;
redo_.push_back(store.snapshot());
store.restore(undo_.back());
undo_.pop_back();
return true;
}

### 4) Facade — StreamingPlatform simplifica el subsistema

Qué resuelve: expone una API de alto nivel para UI, ocultando índices/estructuras internas.

Dónde: StreamingPlatform.h/.cpp

Snippet:
// StreamingPlatform.h (API de alto nivel)
bool loadDataset(const std::string& path);
void buildIndexes();
std::vector<SearchResult> search(const std::string& user_input) const;
std::vector<int> recommend(const User& u, int k) const;

### 5) Repository — UserStore centraliza persistencia y acceso a usuarios

Qué resuelve: separa UI de la persistencia y CRUD de perfiles.

Dónde: UserStore.h/.cpp

Snippet:
bool load(const std::string& path);
bool save(const std::string& path) const;

void create(int slot, std::string name);
void remove(int slot);
User& get(int slot);
bool has(int slot) const;

### 6) Dependency Injection — UI recibe dependencias por constructor

Qué resuelve: reduce acoplamiento y facilita pruebas/mantenimiento.

Dónde: UI.h, main.cpp

Snippet:
// main.cpp
StreamingPlatform platform;
UserStore users;
UI ui(platform, users, USERS_FILE);

---
## Librerías usadas

Este proyecto se implementa con la **librería estándar de C++**, incluyendo (según módulos):

- `<vector>`, `<string>`, `<array>`
- `<unordered_map>`, `<unordered_set>`
- `<algorithm>`, `<random>`
- `<fstream>`, `<sstream>`
- `<regex>` (si se usa en utilidades de texto)

No se requiere instalación de librerías externas.

---
## Diseño del sistema (módulos)

### `StreamingPlatform` (core)
Actúa como el “cerebro” del sistema:

- Carga dataset.
- Construye índices.
- Ejecuta `search()`.
- Genera `recommend()`.
- Maneja tags (top y random).

### Índices (index)
Se usan dos estructuras principales:

#### 1) `WordIndex` (índice invertido)
- Tokeniza título/sinopsis/tags.
- Usa un `Trie` para mapear tokens a `term_id`.
- Para cada `term_id` guarda postings con conteos por fuente (título/sinopsis/tag).

#### 2) `NgramIndex` (n-grams / substring)
- Construye trigramas (por defecto n=3) de un texto compactado.
- Permite filtrar candidatos por intersección de postings de cada n-gram.
- Se usa como soporte para **búsqueda por fragmentos** (substrings).

### Normalización (text)
`TextUtils` se encarga de:
- convertir a minúsculas.
- eliminar símbolos y colapsar espacios.
- parsing de tags (ej. `["A","B"]`, `'tag1,tag2'`, etc.).

### `Session` (core) — Singleton + State
Responsable de:
- Mantener el **usuario activo** en la ejecución.
- Asegurar que **no existan dos usuarios activos** simultáneamente.
- Controlar el flujo `login/logout` mediante estados `LoggedOut/LoggedIn`.

### `UserHistory` (core) — Caretaker de Memento
Responsable de:
- Registrar **checkpoints** del estado de usuarios.
- Ejecutar `undo()` / `redo()` restaurando snapshots.

---
## Algoritmo de ranking (búsqueda)

La búsqueda retorna resultados con puntaje (score), combinando:
- Hits en **título**
- Hits en **sinopsis**
- Hits en **tags**
- Bonus si:
    - Matchea todos los tokens
    - Matchea substring (n-grams)

---
## Recomendaciones

Las recomendaciones se basan en el perfil del usuario:
- Perfil de **tags** a partir de likes.
- (Opcional según implementación) palabras frecuentes en títulos likeados.
- Excluye películas ya likeadas o guardadas en “ver más tarde”.

---
## Control de errores

El sistema contempla:
- Validación de lectura del dataset (si falla, termina con mensaje).
- Manejo de entradas vacías.
- Persistencia segura de usuarios (si el archivo no existe, inicia vacío).

---
## Benchmarks / tiempos 


Antes De aplicar programacion paralela
Dataset: data/movies.csv
N peliculas: 14828
Queries usadas:
- palabra: "tre"
- frase:   "note this"
- tag:     "tag:murder"
  (sink=306492)

| Operación             | Tamaño dataset | Tiempo (ms) | Notas              |
|----------------------|---------------:|------------:|-------------------|
| Carga dataset        | 14828 películas | 5144.398 | lectura + parsing |
| Build de índices     | 14828 películas | 34018.394 | WordIndex + Ngram |
| Búsqueda (1 palabra) | 14828 películas | 74.951 | promedio 10 runs  |
| Búsqueda (frase)     | 14828 películas | 31.366 | promedio 10 runs  |
| Búsqueda `tag:...`   | 14828 películas | 11.912 | lookup por tag    |

### Threads: 1

=== BENCHMARK ===
Dataset: data/movies.csv
N peliculas: 14828
Queries usadas:
- palabra: "tre"
- frase:   "note this"
- tag:     "tag:murder"
  (sink=306492)

| Operación             | Tamaño dataset | Tiempo (ms) | Notas              |
|----------------------|---------------:|------------:|-------------------|
| Carga dataset        | 14828 películas | 4556.558 | lectura + parsing |
| Build de índices     | 14828 películas | 22411.140 | WordIndex + Ngram |
| Búsqueda (1 palabra) | 14828 películas | 58.429 | promedio 10 runs  |
| Búsqueda (frase)     | 14828 películas | 26.277 | promedio 10 runs  |
| Búsqueda `tag:...`   | 14828 películas | 10.377 | lookup por tag    |

=== FIN BENCHMARK ===


### Threads: 2

=== BENCHMARK ===
Dataset: data/movies.csv
N peliculas: 14828
Queries usadas:
- palabra: "tre"
- frase:   "note this"
- tag:     "tag:murder"
  (sink=306492)

| Operación             | Tamaño dataset | Tiempo (ms) | Notas              |
|----------------------|---------------:|------------:|-------------------|
| Carga dataset        | 14828 películas | 4246.508 | lectura + parsing |
| Build de índices     | 14828 películas | 22060.336 | WordIndex + Ngram |
| Búsqueda (1 palabra) | 14828 películas | 69.492 | promedio 10 runs  |
| Búsqueda (frase)     | 14828 películas | 37.742 | promedio 10 runs  |
| Búsqueda `tag:...`   | 14828 películas | 16.306 | lookup por tag    |

=== FIN BENCHMARK ===


### Threads: 4

=== BENCHMARK ===
Dataset: data/movies.csv
N peliculas: 14828
Queries usadas:
- palabra: "tre"
- frase:   "note this"
- tag:     "tag:murder"
  (sink=306492)

| Operación             | Tamaño dataset | Tiempo (ms) | Notas              |
|----------------------|---------------:|------------:|-------------------|
| Carga dataset        | 14828 películas | 4384.990 | lectura + parsing |
| Build de índices     | 14828 películas | 23139.122 | WordIndex + Ngram |
| Búsqueda (1 palabra) | 14828 películas | 66.763 | promedio 10 runs  |
| Búsqueda (frase)     | 14828 películas | 29.685 | promedio 10 runs  |
| Búsqueda `tag:...`   | 14828 películas | 12.102 | lookup por tag    |

=== FIN BENCHMARK ===


### Threads: 8

=== BENCHMARK ===
Dataset: data/movies.csv
N peliculas: 14828
Queries usadas:
- palabra: "tre"
- frase:   "note this"
- tag:     "tag:murder"
  (sink=306492)

| Operación             | Tamaño dataset | Tiempo (ms) | Notas              |
|----------------------|---------------:|------------:|-------------------|
| Carga dataset        | 14828 películas | 5407.725 | lectura + parsing |
| Build de índices     | 14828 películas | 23368.079 | WordIndex + Ngram |
| Búsqueda (1 palabra) | 14828 películas | 56.504 | promedio 10 runs  |
| Búsqueda (frase)     | 14828 películas | 24.794 | promedio 10 runs  |
| Búsqueda `tag:...`   | 14828 películas | 9.568 | lookup por tag    |


---
## Conclusiones

- El uso de **índices invertidos + Trie** permite búsquedas rápidas por palabras y tags.
- El índice de **n-grams** soporta búsqueda por fragmentos sin escanear todo el dataset.
- La separación por módulos (`core`, `index`, `ui`, `text`) mejora mantenimiento y escalabilidad.
- La sesión única (**Singleton + State**) cumple el requisito de “un usuario por sesión”.
- El manejo de estado de usuarios mediante **Memento** permite restaurar cambios (undo/redo) y mantener consistencia.

---

## Referencias APA

Gamma, E., Helm, R., Johnson, R., & Vlissides, J. (1994). Design patterns: Elements of reusable object-oriented software (1st ed.). Addison-Wesley Professional.

Fowler, M., Rice, D., Foemmel, M., Hieatt, E., Mee, R., & Stafford, R. (2002). Patterns of enterprise application architecture (1st ed.). Addison-Wesley Professional.

Hieatt, E., & Mee, R. (2003, March 5). Repository. En M. Fowler (Ed.), Catalog of Patterns of Enterprise Application Architecture. Recuperado el 26 de febrero de 2026.

Evans, E. (2003). Domain-driven design: Tackling complexity in the heart of software (1st ed.). Addison-Wesley Professional.

Fowler, M. (2004, January 23). Inversion of Control Containers and the Dependency Injection pattern. Recuperado el 26 de febrero de 2026.

Microsoft. (2026, January 28). Dependency injection – .NET (Documentation). Microsoft Learn.

Microsoft. (2023, February 20). Designing the infrastructure persistence layer (Repository pattern). Microsoft Learn.
