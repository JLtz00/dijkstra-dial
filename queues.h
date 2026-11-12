/* Cuatro colas de prioridad para el mismo Dijkstra, para compararlas.
 * Cada solver recibe el grafo (node[] mutable: usa node[].dist como scratch)
 * y devuelve la distancia s->t, o INF si no hay camino. */
#ifndef QUEUES_H
#define QUEUES_H
#include <stdint.h>
#include "graph.h"

#define NB (WMAX + 1)   /* 101 cubetas circulares de Dial */

struct Ent { int node; int next; };   /* entrada de cubeta (lista enlazada en pool) */

static inline int ctz128(unsigned __int128 x) {
    unsigned long long lo = (unsigned long long)x;
    return lo ? __builtin_ctzll(lo) : 64 + __builtin_ctzll((unsigned long long)(x >> 64));
}

/* ---- 1) Dial con mascara de bits (salta cubetas vacias en O(1)) ---- */
static unsigned solve_dial(struct Node *node, const struct Arc *arc,
                           unsigned s, unsigned t, struct Ent *pool) {
    int bhead[NB];
    memset(bhead, 0xff, sizeof bhead);
    unsigned __int128 mask = 1;
    int np = 0, b = 0;
    unsigned curd = 0;

    node[s].dist = 0;
    pool[np].node = (int)s; pool[np].next = -1; bhead[0] = np++;

    while (mask) {
        unsigned __int128 hi = mask >> b;                  /* cubetas en [b, 100] */
        int k = hi ? ctz128(hi) : NB - b + ctz128(mask);   /* si no, da la vuelta */
        curd += (unsigned)k;
        b += k; if (b >= NB) b -= NB;

        for (int e = bhead[b]; e >= 0; e = bhead[b]) {
            bhead[b] = pool[e].next;
            unsigned u = (unsigned)pool[e].node;
            if (node[u].dist != curd) continue;            /* entrada obsoleta */
            if (u == t) return curd;
            for (int a = node[u].head; a >= 0; a = arc[a].nx) {
                unsigned tw = (unsigned)arc[a].tw, v = tw >> 7, w = tw & 127;
                unsigned nd = curd + w;
                if (nd < node[v].dist) {
                    node[v].dist = nd;
                    int b2 = b + (int)w; if (b2 >= NB) b2 -= NB;   /* w<=100 => b2!=b */
                    pool[np].node = (int)v; pool[np].next = bhead[b2]; bhead[b2] = np++;
                    mask |= (unsigned __int128)1 << b2;
                }
            }
        }
        mask &= ~((unsigned __int128)1 << b);
    }
    return INF;
}

/* ---- 2) Dial sin mascara: busca la siguiente cubeta recorriendolas ---- */
static unsigned solve_dial_scan(struct Node *node, const struct Arc *arc,
                                unsigned s, unsigned t, struct Ent *pool) {
    int bhead[NB];
    memset(bhead, 0xff, sizeof bhead);
    int np = 0, b = 0, pend = 1;
    unsigned curd = 0;

    node[s].dist = 0;
    pool[np].node = (int)s; pool[np].next = -1; bhead[0] = np++;

    while (pend > 0) {
        while (bhead[b] < 0) { b++; if (b >= NB) b -= NB; curd++; }
        for (int e = bhead[b]; e >= 0; e = bhead[b]) {
            bhead[b] = pool[e].next;
            pend--;
            unsigned u = (unsigned)pool[e].node;
            if (node[u].dist != curd) continue;
            if (u == t) return curd;
            for (int a = node[u].head; a >= 0; a = arc[a].nx) {
                unsigned tw = (unsigned)arc[a].tw, v = tw >> 7, w = tw & 127;
                unsigned nd = curd + w;
                if (nd < node[v].dist) {
                    node[v].dist = nd;
                    int b2 = b + (int)w; if (b2 >= NB) b2 -= NB;
                    pool[np].node = (int)v; pool[np].next = bhead[b2]; bhead[b2] = np++;
                    pend++;
                }
            }
        }
    }
    return INF;
}

/* clave empaquetada: distancia en los bits altos, nodo en los 20 bajos
   => comparar prioridades es comparar un solo uint64 */
#define KEY(d, v) (((uint64_t)(d) << 20) | (uint64_t)(v))

/* ---- 3) Heap binario, borrado perezoso ---- */
static unsigned solve_heap_bin(struct Node *node, const struct Arc *arc,
                               unsigned s, unsigned t, uint64_t *h) {
    size_t n = 0;
    node[s].dist = 0;
    h[n++] = KEY(0, s);
    while (n) {
        uint64_t top = h[0];
        uint64_t last = h[--n];
        if (n) {                                  /* baja el ultimo desde la raiz */
            size_t i = 0;
            for (;;) {
                size_t c = 2 * i + 1;
                if (c >= n) break;
                if (c + 1 < n && h[c + 1] < h[c]) c++;
                if (h[c] >= last) break;
                h[i] = h[c]; i = c;
            }
            h[i] = last;
        }
        unsigned u = (unsigned)(top & 0xfffff), curd = (unsigned)(top >> 20);
        if (node[u].dist != curd) continue;
        if (u == t) return curd;
        for (int a = node[u].head; a >= 0; a = arc[a].nx) {
            unsigned tw = (unsigned)arc[a].tw, v = tw >> 7, w = tw & 127;
            unsigned nd = curd + w;
            if (nd < node[v].dist) {
                node[v].dist = nd;
                uint64_t k = KEY(nd, v);
                size_t i = n++;
                while (i && h[(i - 1) / 2] > k) { h[i] = h[(i - 1) / 2]; i = (i - 1) / 2; }
                h[i] = k;
            }
        }
    }
    return INF;
}

/* ---- 4) Heap 4-ario (mas plano = menos fallos de cache) ---- */
static unsigned solve_heap_4ary(struct Node *node, const struct Arc *arc,
                                unsigned s, unsigned t, uint64_t *h) {
    size_t n = 0;
    node[s].dist = 0;
    h[n++] = KEY(0, s);
    while (n) {
        uint64_t top = h[0];
        uint64_t last = h[--n];
        if (n) {
            size_t i = 0;
            for (;;) {
                size_t c = 4 * i + 1;
                if (c >= n) break;
                size_t lim = c + 4 < n ? c + 4 : n, best = c;
                for (size_t j = c + 1; j < lim; j++) if (h[j] < h[best]) best = j;
                if (h[best] >= last) break;
                h[i] = h[best]; i = best;
            }
            h[i] = last;
        }
        unsigned u = (unsigned)(top & 0xfffff), curd = (unsigned)(top >> 20);
        if (node[u].dist != curd) continue;
        if (u == t) return curd;
        for (int a = node[u].head; a >= 0; a = arc[a].nx) {
            unsigned tw = (unsigned)arc[a].tw, v = tw >> 7, w = tw & 127;
            unsigned nd = curd + w;
            if (nd < node[v].dist) {
                node[v].dist = nd;
                uint64_t k = KEY(nd, v);
                size_t i = n++;
                while (i && h[(i - 1) / 4] > k) { h[i] = h[(i - 1) / 4]; i = (i - 1) / 4; }
                h[i] = k;
            }
        }
    }
    return INF;
}
#endif
