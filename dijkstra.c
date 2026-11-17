/* Dijkstra con pesos enteros 1..100.
 * Uso: ./dijkstra entrada.txt [cola]   |   ./dijkstra - [cola] < entrada.txt
 *      cola: dial (por defecto) | barrido | heap | heap4   -- ver queues.h
 *      TIEMPO=1 ./dijkstra ...  escribe el desglose de tiempos en stderr
 * Formato: "V E" / "s t" / V lineas de etiquetas / E lineas "u v w". */
/* _GNU_SOURCE antes de cualquier header: habilita MAP_POPULATE, MADV_HUGEPAGE
   y clock_gettime en glibc viejo (si no, falla al compilar en Colab) */
#define _GNU_SOURCE
#include <time.h>
#include "queues.h"

#define NO_PATH "No hay camino"   /* cambiar por "-1" si lo piden asi */
#define USO "cola desconocida; usa: dial | barrido | heap | heap4"

static double ms(void) {   /* CLOCK_MONOTONIC via vDSO: ~25 ns por llamada */
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

int main(int argc, char **argv) {
    double t0 = ms();
    const char *path = (argc > 1 && strcmp(argv[1], "-")) ? argv[1] : NULL;
    const char *cola = argc > 2 ? argv[2] : "dial";

    const char *p, *end;
    gr_load(path, &p, &end);

    struct Graph g;
    gr_parse(p, end, &g);
    double t1 = ms();

    unsigned d = 0;   /* s == t => 0, y gr_parse no llego a construir el grafo */
    if (g.s != g.t) {
        /* una sola reserva: las cubetas de Dial y el heap usan 8 bytes por entrada */
        void *scratch = malloc((g.narcs + 2) * 8);
        if (!scratch) die("sin memoria");
        d = !strcmp(cola, "dial")    ? solve_dial(g.node, g.arc, g.s, g.t, scratch)
          : !strcmp(cola, "barrido") ? solve_dial_scan(g.node, g.arc, g.s, g.t, scratch)
          : !strcmp(cola, "heap")    ? solve_heap_bin(g.node, g.arc, g.s, g.t, scratch)
          : !strcmp(cola, "heap4")   ? solve_heap_4ary(g.node, g.arc, g.s, g.t, scratch)
          : (die(USO), 0u);
    }
    double t2 = ms();

    if (d == INF) puts(NO_PATH);
    else printf("%u\n", d);

    /* a stderr: la salida del programa sigue siendo solo la distancia */
    if (getenv("TIEMPO"))
        fprintf(stderr, "parseo %.3f ms | %s %.3f ms | total %.3f ms\n",
                t1 - t0, cola, t2 - t1, t2 - t0);
    return 0;
}
