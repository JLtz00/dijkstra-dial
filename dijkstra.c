/* Dijkstra con pesos enteros 1..100.
 * Uso: ./dijkstra entrada.txt [cola]   |   ./dijkstra - [cola] < entrada.txt
 *      cola: dial (por defecto) | barrido | heap | heap4   -- ver queues.h
 * Formato: "V E" / "s t" / V lineas de etiquetas / E lineas "u v w". */
#include "queues.h"

#define NO_PATH "No hay camino"   /* cambiar por "-1" si lo piden asi */
#define USO "cola desconocida; usa: dial | barrido | heap | heap4"

int main(int argc, char **argv) {
    const char *path = (argc > 1 && strcmp(argv[1], "-")) ? argv[1] : NULL;
    const char *cola = argc > 2 ? argv[2] : "dial";

    const char *p, *end;
    gr_load(path, &p, &end);

    struct Graph g;
    gr_parse(p, end, &g);
    if (g.s == g.t) { puts("0"); return 0; }

    /* una sola reserva: las cubetas de Dial y el heap usan 8 bytes por entrada */
    void *scratch = malloc((g.narcs + 2) * 8);
    if (!scratch) die("sin memoria");

    unsigned d = !strcmp(cola, "dial")    ? solve_dial(g.node, g.arc, g.s, g.t, scratch)
               : !strcmp(cola, "barrido") ? solve_dial_scan(g.node, g.arc, g.s, g.t, scratch)
               : !strcmp(cola, "heap")    ? solve_heap_bin(g.node, g.arc, g.s, g.t, scratch)
               : !strcmp(cola, "heap4")   ? solve_heap_4ary(g.node, g.arc, g.s, g.t, scratch)
               : (die(USO), 0u);

    if (d == INF) puts(NO_PATH);
    else printf("%u\n", d);
    return 0;
}
