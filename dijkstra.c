/* Dijkstra con cubetas de Dial (pesos enteros 1..100).
 * Uso: ./dijkstra entrada.txt   |   ./dijkstra < entrada.txt
 * Formato: "V E" / "s t" / V lineas de etiquetas / E lineas "u v w". */
#include "queues.h"

#define NO_PATH "No hay camino"   /* cambiar por "-1" si lo piden asi */

int main(int argc, char **argv) {
    const char *p, *end;
    gr_load(argc > 1 ? argv[1] : NULL, &p, &end);

    struct Graph g;
    gr_parse(p, end, &g);
    if (g.s == g.t) { puts("0"); return 0; }

    struct Ent *pool = malloc((g.narcs + 2) * sizeof *pool);
    if (!pool) die("sin memoria");

    unsigned d = solve_dial(g.node, g.arc, g.s, g.t, pool);
    if (d == INF) puts(NO_PATH);
    else printf("%u\n", d);
    return 0;
}
