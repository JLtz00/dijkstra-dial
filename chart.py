"""resultados.csv -> comparacion.html (SVG inline, sin dependencias).
Uso: python3 chart.py resultados.csv comparacion.html"""
import csv, sys, html
from collections import OrderedDict

src = sys.argv[1] if len(sys.argv) > 1 else "resultados.csv"
dst = sys.argv[2] if len(sys.argv) > 2 else "comparacion.html"

COL = {"parseo": "var(--neutral)", "Dial+mascara": "var(--series-1)",
       "Dial+barrido": "var(--series-2)", "Heap binario": "var(--series-3)",
       "Heap 4-ario": "var(--series-4)"}

rows = list(csv.DictReader(open(src)))
facets = OrderedDict()
for r in rows:
    facets.setdefault((r["grafo"], r["V"], r["E"]), []).append((r["variante"], float(r["ms_med"]), r["dist"]))

BH, GAP, LBL, PAD = 20, 8, 118, 54   # alto de barra, separacion, columna de etiquetas, margen derecho

def bar_path(x, y, w, h, r=4):
    if w < r: return f'<rect x="{x}" y="{y}" width="{max(w,0.8):.1f}" height="{h}"/>'
    return (f'<path d="M{x},{y}h{w-r:.1f}a{r},{r} 0 0 1 {r},{r}v{h-2*r}'
            f'a{r},{r} 0 0 1 {-r},{r}H{x}z"/>')

def facet(key, data):
    name, V, E = key
    hi = max(v for _, v, _ in data) or 1
    W, H = 440, len(data) * (BH + GAP) + 6
    plot = W - LBL - PAD
    out = [f'<figure class="facet"><figcaption><b>{html.escape(name[:-4])}</b>'
           f'<span>V={int(V):,} · E={int(E):,} · dist={data[0][2] if data[0][2]!="-" else data[1][2]}</span></figcaption>'
           f'<svg viewBox="0 0 {W} {H}" role="img">']
    for i, (var, ms, _) in enumerate(data):
        y = i * (BH + GAP) + 3
        w = plot * ms / hi
        out.append(f'<text class="lbl" x="{LBL-8}" y="{y+BH*0.72}" text-anchor="end">{html.escape(var)}</text>')
        out.append(f'<g fill="{COL.get(var,"var(--series-1)")}">{bar_path(LBL, y, w, BH)}</g>')
        out.append(f'<text class="val" x="{LBL+w+6:.1f}" y="{y+BH*0.72}">{ms:.2f} ms</text>')
    out.append("</svg></figure>")
    return "".join(out)

table = ["<table><thead><tr>" + "".join(f"<th>{h}</th>" for h in rows[0]) + "</tr></thead><tbody>"]
for r in rows:
    table.append("<tr>" + "".join(f"<td>{html.escape(v)}</td>" for v in r.values()) + "</tr>")
table.append("</tbody></table>")

open(dst, "w").write(f"""<!doctype html><html lang="es"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Dijkstra: comparacion de colas</title>
<style>
:root{{color-scheme:light;--surface-1:#fcfcfb;--surface-2:#f4f3f0;--text-primary:#0b0b0b;
--text-secondary:#52514e;--border:#dedcd5;--neutral:#8a8880;
--series-1:#2a78d6;--series-2:#eb6834;--series-3:#1baf7a;--series-4:#eda100}}
@media (prefers-color-scheme:dark){{:root:not([data-theme="light"]){{color-scheme:dark;
--surface-1:#1a1a19;--surface-2:#232321;--text-primary:#f0efec;--text-secondary:#c3c2b7;
--border:#3a3a37;--neutral:#8a8880;--series-1:#3987e5;--series-2:#d95926;--series-3:#199e70;--series-4:#c98500}}}}
body{{margin:0;padding:24px 16px;background:var(--surface-1);color:var(--text-primary);
font:15px/1.5 ui-sans-serif,system-ui,sans-serif}}
main{{max-width:1000px;margin:0 auto}}
h1{{font-size:1.4rem;margin:0 0 .25rem}}
p.sub{{color:var(--text-secondary);margin:0 0 1.5rem}}
.grid{{display:grid;gap:20px;grid-template-columns:repeat(auto-fit,minmax(330px,1fr))}}
.facet{{margin:0;background:var(--surface-2);border:1px solid var(--border);border-radius:10px;padding:12px 14px}}
figcaption{{display:flex;flex-direction:column;margin-bottom:8px}}
figcaption span{{color:var(--text-secondary);font-size:.78rem}}
svg{{width:100%;height:auto;overflow:visible}}
.lbl{{fill:var(--text-secondary);font-size:11px}}
.val{{fill:var(--text-primary);font-size:11px;font-weight:600;font-variant-numeric:tabular-nums}}
table{{border-collapse:collapse;font-size:.82rem;margin-top:10px;width:100%}}
th,td{{text-align:left;padding:4px 10px;border-bottom:1px solid var(--border)}}
details{{margin-top:28px}}
</style>
<main>
<h1>Dijkstra: parseo vs. cola de prioridad</h1>
<p class="sub">Milisegundos (mediana). Cada barra esta etiquetada con su valor; la escala es
propia de cada grafo. <b>parseo</b> = leer el archivo + construir las listas de adyacencia,
una sola vez por ejecucion.</p>
<div class="grid">{"".join(facet(k, v) for k, v in facets.items())}</div>
<details><summary>Ver los datos (min y mediana)</summary>{"".join(table)}</details>
</main></html>""")
print(f"-> {dst}")
