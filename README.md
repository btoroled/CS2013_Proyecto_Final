# Programación III — Proyecto Final (2026-0)
Plataforma de streaming (CLI) con **búsqueda eficiente** y **recomendaciones** usando estructuras de datos (Trie, índices invertidos y n-grams).

---

## Integrantes
- **Benjamin Toro Leddihn**
- **Hector Miguel Espinoza Torres**
- **Leonardo Medina Gago**

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
  core/     -> Movie, User, StreamingPlatform, UserStore
  index/    -> Trie, WordIndex, NgramIndex
  ui/       -> UI (menús por consola)
  text/     -> TextUtils (normalización y parsing)
src/
  core/
  index/
  ui/
  text/
data/
  movies.csv
users.txt   -> se genera automáticamente (persistencia de perfiles)
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

### 1) Pantalla de perfiles
- Se muestran **4 slots** (máximo 4 usuarios).
- Puedes:
    - Elegir un perfil existente.
    - Crear uno si el slot está vacío.
    - (Opcional según implementación) Borrar un perfil.

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

---

## Persistencia de usuarios (`users.txt`)

Los perfiles y sus listas se guardan automáticamente en un archivo de texto.

Características:
- Máximo **4 usuarios**
- Guarda:
    - nombre
    - imdb_ids de liked
    - imdb_ids de watch_later (mantiene orden)

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
## Benchmarks / tiempos (plantilla)


| Operación             | Tamaño dataset | Tiempo (ms) | Notas              |
|----------------------|---------------:|------------:|-------------------|
| Carga dataset        | N películas    | ___         | lectura + parsing |
| Build de índices     | N películas    | ___         | WordIndex + Ngram |
| Búsqueda (1 palabra) | N películas    | ___         | promedio 10 runs  |
| Búsqueda (frase)     | N películas    | ___         | promedio 10 runs  |
| Búsqueda `tag:...`   | N películas    | ___         | lookup por tag    |

---
## Conclusiones

- El uso de **índices invertidos + Trie** permite búsquedas rápidas por palabras y tags.
- El índice de **n-grams** soporta búsqueda por fragmentos sin escanear todo el dataset.
- La separación por módulos (`core`, `index`, `ui`, `text`) mejora mantenimiento y escalabilidad.
- La persistencia permite mantener historial entre ejecuciones.

---
