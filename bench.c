/* Mide parseo y cada cola de prioridad sobre el mismo grafo.
 * Uso: ./bench entrada.txt [repeticiones]   -> una linea CSV por variante */
/* _GNU_SOURCE antes de cualquier header: habilita MAP_POPULATE, MADV_HUGEPAGE
   y clock_gettime en glibc viejo (si no, falla al compilar en Colab) */
#define _GNU_SOURCE
#include <time.h>
#include "queues.h"

static double ms_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

static int cmpd(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv) {
    if (argc < 2) die("uso: ./bench entrada.txt [repeticiones]");
    int R = argc > 2 ? atoi(argv[2]) : 5;
    if (R < 1 || R > 99) R = 5;
    const char *name = strrchr(argv[1], '/');
    name = name ? name + 1 : argv[1];

    double *tm = malloc((size_t)R * sizeof *tm);
    struct Graph g;
    const char *p, *end;

    /* parseo: se repite releyendo el archivo (incluye E/S + construccion) */
    for (int r = 0; r < R; r++) {
        double t0 = ms_now();
        gr_load(argv[1], &p, &end);
        gr_parse(p, end, &g);
        tm[r] = ms_now() - t0;
        if (r + 1 < R) gr_free(&g);
    }
    qsort(tm, (size_t)R, sizeof *tm, cmpd);
    printf("%s,%u,%u,parseo,%.3f,%.3f,-\n", name, g.V, g.E, tm[0], tm[R / 2]);
    if (!g.node) die("s == t: nada que medir");

    size_t nb = (size_t)(g.V + 1) * sizeof(struct Node);
    struct Node *pristine = malloc(nb), *work = malloc(nb);
    struct Ent *pool = malloc((g.narcs + 2) * sizeof *pool);
    uint64_t *heap = malloc((g.narcs + 2) * sizeof *heap);
    if (!pristine || !work || !pool || !heap) die("sin memoria");
    memcpy(pristine, g.node, nb);

    const char *names[] = { "Dial+mascara", "Dial+barrido", "Heap binario", "Heap 4-ario" };
    unsigned res[4];
    for (int v = 0; v < 4; v++) {
        for (int r = 0; r < R; r++) {
            memcpy(work, pristine, nb);          /* fuera del cronometro */
            double t0 = ms_now();
            unsigned d = v == 0 ? solve_dial(work, g.arc, g.s, g.t, pool)
                       : v == 1 ? solve_dial_scan(work, g.arc, g.s, g.t, pool)
                       : v == 2 ? solve_heap_bin(work, g.arc, g.s, g.t, heap)
                                : solve_heap_4ary(work, g.arc, g.s, g.t, heap);
            tm[r] = ms_now() - t0;
            res[v] = d;
        }
        qsort(tm, (size_t)R, sizeof *tm, cmpd);
        printf("%s,%u,%u,%s,%.3f,%.3f,", name, g.V, g.E, names[v], tm[0], tm[R / 2]);
        if (res[v] == INF) puts("sin camino"); else printf("%u\n", res[v]);
    }
    for (int v = 1; v < 4; v++)
        if (res[v] != res[0]) { fprintf(stderr, "DISCREPANCIA en %s: %s=%u vs Dial=%u\n", name, names[v], res[v], res[0]); return 1; }
    return 0;
}
