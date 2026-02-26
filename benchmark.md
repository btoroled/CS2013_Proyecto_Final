# Threads: 1

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


## Threads: 2

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


## Threads: 4

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


## Threads: 8

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

=== FIN BENCHMARK ===


