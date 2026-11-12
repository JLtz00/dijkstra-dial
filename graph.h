/* Carga y parseo de la entrada (compartido por dijkstra.c y bench.c). */
#ifndef GRAPH_H
#define GRAPH_H
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define UNDIRECTED 1              /* "arista entre u y v" => doble sentido */
#define WMAX 100
#define INF 0xffffffffu

struct Node { unsigned dist; int head; };   /* dist y head juntos: una linea de cache */
struct Arc  { int tw; int nx; };            /* tw = (destino << 7) | peso */

struct Graph {
    unsigned V, E, s, t;
    size_t narcs;
    struct Node *node;   /* indices 1..V */
    struct Arc *arc;
};

static void die(const char *m) { fprintf(stderr, "error: %s\n", m); exit(1); }

static inline unsigned rd(const char **pp, const char *end) {
    const char *p = *pp;
    while (p < end && (unsigned)(*p - '0') > 9u) p++;
    unsigned x = 0, c;
    while (p < end && (c = (unsigned)(*p - '0')) <= 9u) { x = x * 10 + c; p++; }
    *pp = p;
    return x;
}

/* Salta n lineas contando '\n' en bloques de 64 (el compilador lo vectoriza):
   un memchr por linea costaba 1M de llamadas = ~5 ms con V=1.000.000. */
static const char *skip_lines(const char *p, const char *end, long n) {
    while (n > 0 && end - p >= 64) {
        long c = 0;
        for (int i = 0; i < 64; i++) c += (p[i] == '\n');
        if (c >= n) break;   /* el salto n-esimo esta en este bloque: termina byte a byte */
        n -= c; p += 64;
    }
    while (n > 0 && p < end) { if (*p == '\n') n--; p++; }
    return p;
}

/* Paginas grandes (2 MB): menos fallos de pagina al inicializar los 8 MB de
   nodos y menos fallos de TLB en los accesos aleatorios durante el parseo. */
static void *gr_alloc(size_t n) {
    void *m = mmap(NULL, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (m == MAP_FAILED) die("sin memoria");
    madvise(m, n, MADV_HUGEPAGE);
    return m;
}

/* libera lo que reservo gr_parse (solo lo usa bench.c al repetir el parseo) */
static void gr_free(struct Graph *g) {
    if (g->node) munmap(g->node, (size_t)(g->V + 1) * sizeof *g->node);
    if (g->arc) munmap(g->arc, (g->narcs ? g->narcs : 1) * sizeof *g->arc);
    g->node = NULL; g->arc = NULL;
}

static char *slurp_fd(int fd, size_t *len) {
    size_t cap = 1u << 22, n = 0;
    char *b = malloc(cap);
    if (!b) return NULL;
    for (;;) {
        if (n == cap) { cap *= 2; char *tb = realloc(b, cap); if (!tb) { free(b); return NULL; } b = tb; }
        ssize_t r = read(fd, b + n, cap - n);
        if (r <= 0) break;
        n += (size_t)r;
    }
    *len = n;
    return b;
}

/* mmap del archivo (o lectura completa si es stdin/pipe) */
static void gr_load(const char *path, const char **pp, const char **pend) {
    if (!path) {
        size_t n; char *b = slurp_fd(0, &n);
        if (!b) die("sin memoria");
        *pp = b; *pend = b + n;
        return;
    }
    int fd = open(path, O_RDONLY);
    if (fd < 0) die("no se pudo abrir el archivo");
    struct stat st;
    char *m = MAP_FAILED;
    size_t sz = 0;
    if (!fstat(fd, &st) && S_ISREG(st.st_mode)) {
        sz = (size_t)st.st_size;
        m = sz ? mmap(NULL, sz, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0) : (char *)"";
    }
    if (m == MAP_FAILED) {
        size_t n; char *b = slurp_fd(fd, &n);
        if (!b) die("sin memoria");
        *pp = b; *pend = b + n;
    } else {
        *pp = m; *pend = m + sz;
    }
    close(fd);
}

/* parsea cabecera + etiquetas + aristas y construye las listas de adyacencia */
static void gr_parse(const char *p, const char *end, struct Graph *g) {
    g->V = rd(&p, end); g->E = rd(&p, end);
    g->s = rd(&p, end); g->t = rd(&p, end);
    unsigned V = g->V, E = g->E;
    if (V < 1 || g->s < 1 || g->s > V || g->t < 1 || g->t > V) die("V, s o t fuera de rango");
    if (g->s == g->t) { g->node = NULL; g->arc = NULL; g->narcs = 0; return; }   /* nada que parsear */

    p = skip_lines(p, end, (long)V + 1);   /* resto de la linea 2 + V etiquetas */

    g->narcs = (size_t)E * (UNDIRECTED ? 2 : 1);
    g->node = gr_alloc((size_t)(V + 1) * sizeof *g->node);
    g->arc = gr_alloc((g->narcs ? g->narcs : 1) * sizeof *g->arc);
    if (!g->node || !g->arc) die("sin memoria");
    memset(g->node, 0xff, (size_t)(V + 1) * sizeof *g->node);   /* dist=INF, head=-1 */

    struct Node *node = g->node;
    struct Arc *arc = g->arc;
    int na = 0;
    for (unsigned i = 0; i < E; i++) {
        unsigned u = rd(&p, end), v = rd(&p, end), w = rd(&p, end);
        if (u < 1 || u > V || v < 1 || v > V || w < 1 || w > WMAX) die("arista fuera de rango");
        arc[na].tw = (int)((v << 7) | w); arc[na].nx = node[u].head; node[u].head = na++;
#if UNDIRECTED
        arc[na].tw = (int)((u << 7) | w); arc[na].nx = node[v].head; node[v].head = na++;
#endif
    }
}
#endif
