"""Genera grafos de prueba en t/ con distintas formas (peso 1..100)."""
import random, os
random.seed(7)
os.makedirs("t", exist_ok=True)

def write(name, V, edges, s, t):
    with open(f"t/{name}.txt", "w") as f:
        f.write(f"{V} {len(edges)}\n{s} {t}\n")
        f.write("".join(f"n{i}\n" for i in range(1, V + 1)))
        f.write("".join(f"{u} {v} {w}\n" for u, v, w in edges))
    print(f"t/{name}.txt  V={V} E={len(edges)}")

w = lambda: random.randint(1, 100)

# cadena larga: distancias enormes (~2.5e7), peor caso para saltar cubetas
write("cadena-1M", 1_000_000, [(i, i + 1, w()) for i in range(1, 500_001)], 1, 500_001)

# malla 500x500: parecido a una red de calles
n = 500
E = [(r * n + c + 1, r * n + c + 2, w()) for r in range(n) for c in range(n - 1)]
E += [(r * n + c + 1, (r + 1) * n + c + 1, w()) for r in range(n - 1) for c in range(n)]
write("malla-250k", n * n, E, 1, n * n)

# 1M de nodos con aristas al azar: casi todo aislado, sin camino (explora todo)
write("disperso-1M", 1_000_000,
      [(random.randint(1, 1_000_000), random.randint(1, 1_000_000), w()) for _ in range(500_000)],
      1, 1_000_000)

# pocos nodos, muchas aristas: estresa la cola (3 tamanos para ver como escala)
for e in (100_000, 300_000, 500_000):
    write(f"denso-50k-E{e//1000}k", 50_000,
          [(random.randint(1, 50_000), random.randint(1, 50_000), w()) for _ in range(e)],
          1, 50_000)
