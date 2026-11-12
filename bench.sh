#!/bin/sh
# Mide parseo y las 4 colas sobre todos los grafos de t/ -> resultados.csv + comparacion.html
# Uso: ./bench.sh [repeticiones]   (por defecto 7; se reporta la mediana)
set -e
cd "$(dirname "$0")"
R=${1:-7}
gcc -O2 -march=native -o bench bench.c
[ -f t/cadena-1M.txt ] || python3 gen.py

echo "grafo,V,E,variante,ms_min,ms_med,dist" > resultados.csv
for f in t/cadena-1M.txt t/malla-250k.txt t/disperso-1M.txt \
         t/denso-50k-E100k.txt t/denso-50k-E300k.txt t/denso-50k-E500k.txt; do
    printf '%s... ' "$f" >&2
    ./bench "$f" "$R" >> resultados.csv
    echo ok >&2
done

python3 chart.py resultados.csv comparacion.html
column -s, -t resultados.csv
echo "-> resultados.csv y comparacion.html (abrir en el navegador)"
