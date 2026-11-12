#!/bin/sh
# Compila y verifica: casos a mano + comparacion contra una referencia en Python.
set -e
cd "$(dirname "$0")"
gcc -O2 -march=native -o dijkstra dijkstra.c

chk() { # chk <esperado> <archivo>
  got=$(./dijkstra "$2"); [ "$got" = "$1" ] && echo "ok  $2 -> $got" \
    || { echo "FALLO $2: esperado '$1', obtenido '$got'"; exit 1; }
}

mkdir -p t
printf '4 4\n1 4\na\nb\nc\nd\n1 2 3\n2 4 5\n1 3 2\n3 4 9\n' > t/basico.txt   # 1-2-4 = 8
printf '4 1\n1 4\n1\n2\n3\n4\n1 2 7\n'                       > t/sincamino.txt
printf '3 1\n2 2\n1\n2\n3\n1 2 4\n'                          > t/mismo.txt
printf '2 1\n1 2\n1\n2\n1 2 100\n'                           > t/unaarista.txt
chk 8               t/basico.txt
chk "No hay camino" t/sincamino.txt
chk 0               t/mismo.txt
chk 100             t/unaarista.txt
echo 0 | ./dijkstra - < t/basico.txt >/dev/null && echo "ok  stdin"

# las cuatro colas deben dar lo mismo
for q in dial barrido heap heap4; do
  got=$(./dijkstra t/basico.txt $q)
  [ "$got" = "8" ] || { echo "FALLO cola $q: obtenido '$got'"; exit 1; }
done
echo "ok  dial / barrido / heap / heap4 coinciden"
./dijkstra t/basico.txt lista 2>/dev/null && { echo "FALLO: cola invalida no dio error"; exit 1; }
echo "ok  cola invalida -> error"

# TIEMPO=1 no debe contaminar stdout
out=$(TIEMPO=1 ./dijkstra t/basico.txt 2>/dev/null)
err=$(TIEMPO=1 ./dijkstra t/basico.txt 2>&1 >/dev/null)
[ "$out" = "8" ] && [ -n "$err" ] || { echo "FALLO TIEMPO=1: stdout='$out' stderr='$err'"; exit 1; }
echo "ok  TIEMPO=1 -> stdout limpio, tiempos en stderr"

# aleatorios contra referencia
python3 - <<'PY'
import random, subprocess, heapq
for it in range(40):
    V=random.randint(2,60); E=random.randint(1,120)
    es=[(random.randint(1,V),random.randint(1,V),random.randint(1,100)) for _ in range(E)]
    s,t=random.randint(1,V),random.randint(1,V)
    txt="%d %d\n%d %d\n"%(V,E,s,t)+"".join("n%d\n"%i for i in range(1,V+1))+"".join("%d %d %d\n"%e for e in es)
    g={i:[] for i in range(1,V+1)}
    for u,v,w in es: g[u].append((v,w)); g[v].append((u,w))
    d={s:0}; q=[(0,s)]
    while q:
        dd,u=heapq.heappop(q)
        if dd>d.get(u,1<<60): continue
        for v,w in g[u]:
            if dd+w<d.get(v,1<<60): d[v]=dd+w; heapq.heappush(q,(dd+w,v))
    exp=str(d[t]) if t in d else "No hay camino"
    got=subprocess.run(["./dijkstra"],input=txt,capture_output=True,text=True).stdout.strip()
    assert got==exp, (it,exp,got,txt)
print("ok  40 grafos aleatorios vs referencia")
PY

# grande y determinista: cadena de 500.000 aristas de peso 100 (distancia maxima 5e7)
# ademas ejercita el salto por bloques de 1.000.000 de etiquetas
python3 -c "
V,E=1000000,500000
L=['%d %d'%(V,E),'1 %d'%(E+1)]+['n%d'%i for i in range(1,V+1)]
L+=['%d %d 100'%(i,i+1) for i in range(1,E+1)]
open('t/grande.txt','w').write('\\n'.join(L)+'\\n')"
chk 50000000 t/grande.txt
echo "--- tiempo total con V=1.000.000 / E=500.000 (15 MB):"
time ./dijkstra t/grande.txt
