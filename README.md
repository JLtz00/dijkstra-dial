# Dijkstra (pesos enteros 1..100)

    gcc -O2 -march=native -o dijkstra dijkstra.c
    ./dijkstra entrada.txt              # Dial (por defecto)
    ./dijkstra - < entrada.txt          # lee de stdin
    ./dijkstra entrada.txt heap         # elegir otra cola de prioridad

Colas disponibles (el 2do argumento), implementadas en `queues.h`:

| | |
|---|---|
| `dial`    | 101 cubetas circulares + mascara de bits para saltar las vacias (por defecto) |
| `barrido` | las mismas cubetas, pero buscando la siguiente recorriendolas una por una |
| `heap`    | heap binario, clave `(dist<<20)|nodo` en un uint64 |
| `heap4`   | heap 4-ario (mas plano, menos fallos de cache) |

Las cuatro dan el mismo resultado (`test.sh` lo verifica); lo que cambia es el tiempo.
Para medirlas de forma seria usa `./bench.sh`, que parsea una vez y cronometra
solo el algoritmo. Para una comparacion rapida sobre tu propia entrada:

    for q in dial barrido heap heap4; do
        printf "%-8s " $q; /usr/bin/time -f "%e s" ./dijkstra entrada.txt $q
    done

Entrada: `V E` / `s t` / V lineas de etiquetas / E lineas `u v w`.
Salida: la distancia, o `No hay camino` (macro `NO_PATH` en `dijkstra.c`).
Aristas tratadas como no dirigidas (`UNDIRECTED` en `graph.h`).

## Archivos
| | |
|---|---|
| `graph.h`   | lectura con mmap y parseo (el 50% del tiempo total) |
| `queues.h`  | 4 colas de prioridad: Dial con mascara, Dial con barrido, heap binario, heap 4-ario |
| `dijkstra.c`| el programa que se entrega (usa Dial con mascara) |
| `bench.c`   | mide parseo y cada cola sobre un mismo grafo |
| `gen.py`    | genera grafos de prueba en `t/` (cadena, malla, disperso, denso) |
| `chart.py`  | `resultados.csv` -> `comparacion.html` |

## Medir
    ./test.sh            # correccion: casos borde + 40 grafos contra una referencia en Python
    ./bench.sh 9         # 9 repeticiones -> resultados.csv + comparacion.html
    ./bench t/malla-250k.txt 9    # un solo grafo, CSV por stdout

Se reporta el minimo y la mediana: la primera corrida tras compilar puede salir
2-3x mas lenta (cache frio, frecuencia del CPU). Para medir a mano:

    time (for i in $(seq 20); do ./dijkstra t/grande.txt >/dev/null; done)

## Resultados (i9-13900H, gcc 15, `-O2 -march=native`, mediana de 9 corridas)

Milisegundos. `comparacion.html` tiene los mismos datos como grafico.

| grafo | parseo | Dial+mascara | Dial+barrido | Heap binario | Heap 4-ario |
|---|---|---|---|---|---|
| cadena 1M nodos (dist 2,5e7) | 7,9 | 7,99 | 28,9 | **2,70** | **2,67** |
| malla 500x500 | 5,7 | **7,13** | 7,26 | 23,2 | 15,1 |
| denso V=50k E=500k | 7,7 | **7,14** | 7,27 | 11,2 | 10,0 |
| disperso 1M, sin camino | 9,9 | 0,00 | 0,00 | 0,00 | 0,00 |

- La mascara de bits solo importa en la cadena (3,6x), donde cada paso salta ~50
  cubetas vacias; en malla y denso las cubetas estan llenas y da igual.
- El heap gana en la cadena (la cola tiene 1-2 elementos), pero su peor caso es
  23 ms contra los 8 ms de Dial: por eso el programa entregado usa Dial.
- Con 1M de nodos el archivo cuesta mas que el algoritmo. El parseo bajo de 14,2
  a 7,9 ms contando los saltos de linea en bloques de 64 bytes en vez de un
  memchr por etiqueta (-4 ms) y reservando los arreglos con paginas de 2 MB (-1,5 ms).
  Probados y descartados por medicion: prefetch de aristas y enlazado estatico.
- Tiempo total del proceso con un archivo de 15 MB: ~20 ms (2 arranque + 8 parseo
  + 8 Dijkstra + 2 liberar mapeos).
