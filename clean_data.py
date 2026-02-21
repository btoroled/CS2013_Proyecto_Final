"""
=============================================================
LIMPIEZA DEL DATASET — MPST Full Data
=============================================================
Dataset original: mpst_full_data.csv
Columnas: imdb_id, title, plot_synopsis, tags, split, synopsis_source

Problemas que resuelve este script:
  1. Comillas mal escapadas dentro de campos CSV
  2. Saltos de línea (\n) dentro de sinopsis
  3. Separadores inconsistentes en tags (, ; |)
  4. Registros con campos esenciales vacíos
  5. Películas duplicadas (mismo imdb_id)
  6. Espacios y caracteres de control basura
=============================================================
"""

import csv
import re

# ── Rutas ──────────────────────────────────────────────────
INPUT_FILE  = 'mpst_full_data_-_mpst_full_data_csv'   # archivo original (descomprimido)
OUTPUT_FILE = 'data/movies_clean.csv'                  # resultado limpio


# =============================================================
# FUNCIÓN 1: limpiar un campo de texto individual
# =============================================================
def clean_text(text):
    """
    Limpia un string genérico:
    - Elimina espacios al inicio/final
    - Colapsa múltiples espacios en uno solo
    - Elimina caracteres de control invisibles (null bytes, etc.)
    """
    if not text:
        return ""

    text = text.strip()                        # quita espacios extremos
    text = re.sub(r'\s+', ' ', text)           # múltiples espacios → uno
    text = re.sub(r'[\x00-\x08\x0b\x0c\x0e-\x1f\x7f]', '', text)  # control chars

    return text


# =============================================================
# FUNCIÓN 2: normalizar los tags de una película
# =============================================================
def clean_tags(raw_tags):
    """
    Los tags vienen con separadores inconsistentes, por ejemplo:
      "cult, horror; gothic|murder"  →  "cult,horror,gothic,murder"

    Pasos:
      1. Separar por coma, punto y coma, o pipe
      2. Limpiar espacios de cada tag individual
      3. Descartar tags vacíos
      4. Reunir con coma como separador estándar
    """
    if not raw_tags:
        return ""

    # Separar con cualquiera de los 3 delimitadores posibles
    parts = re.split(r'[,;|]', raw_tags)

    # Limpiar cada parte individualmente
    cleaned = [p.strip() for p in parts if p.strip()]

    return ','.join(cleaned)   # reunir con coma estándar


# =============================================================
# SCRIPT PRINCIPAL
# =============================================================
def main():
    movies   = []      # lista de películas válidas
    seen_ids = set()   # para detectar duplicados
    skipped  = 0       # contador de filas descartadas

    # ── Leer el CSV original ──────────────────────────────
    # errors='replace' maneja caracteres con encoding roto sin crashear
    with open(INPUT_FILE, 'r', encoding='utf-8', errors='replace') as f:

        reader = csv.DictReader(f)   # lee la primera fila como nombres de columna

        for row in reader:
            try:
                # Extraer y limpiar cada campo
                imdb_id  = clean_text(row.get('imdb_id', ''))
                title    = clean_text(row.get('title', ''))
                synopsis = clean_text(row.get('plot_synopsis', ''))
                tags     = clean_tags(row.get('tags', ''))
                split    = clean_text(row.get('split', ''))
                source   = clean_text(row.get('synopsis_source', ''))

                # ── Validación 1: campos esenciales ──────
                # Sin id o sin título no podemos identificar la película
                if not imdb_id or not title:
                    skipped += 1
                    continue

                # ── Validación 2: duplicados ──────────────
                # Si ya vimos este imdb_id, ignoramos la fila repetida
                if imdb_id in seen_ids:
                    skipped += 1
                    continue
                seen_ids.add(imdb_id)

                # Guardar registro limpio
                movies.append({
                    'imdb_id':         imdb_id,
                    'title':           title,
                    'plot_synopsis':   synopsis,
                    'tags':            tags,
                    'split':           split,
                    'synopsis_source': source
                })

            except Exception as e:
                # Si una fila tiene un error inesperado, la saltamos
                skipped += 1
                print(f"  [WARN] Fila omitida por error: {e}")

    # ── Escribir el CSV limpio ────────────────────────────
    with open(OUTPUT_FILE, 'w', encoding='utf-8', newline='') as f:

        fieldnames = ['imdb_id', 'title', 'plot_synopsis', 'tags', 'split', 'synopsis_source']
        writer = csv.DictWriter(f, fieldnames=fieldnames)

        writer.writeheader()    # escribe la fila de cabecera
        writer.writerows(movies)

    # ── Resumen final ─────────────────────────────────────
    print(f"\n✅ Limpieza completada:")
    print(f"   Películas válidas guardadas : {len(movies):,}")
    print(f"   Filas descartadas           : {skipped:,}")
    print(f"   Archivo de salida           : {OUTPUT_FILE}")

    # Muestra 2 ejemplos para verificar visualmente
    print("\n📽  Muestra de los primeros 2 registros:")
    for m in movies[:2]:
        print(f"  [{m['imdb_id']}] {m['title'][:60]}")
        print(f"    Tags: {m['tags'][:80]}")


if __name__ == '__main__':
    main()
