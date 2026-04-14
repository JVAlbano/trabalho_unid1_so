#!/bin/bash
# etapa2.sh
# Automação da Etapa 2 – Sequencial vs Paralelo
#
# Uso: ./etapa2.sh
# Requisitos: auxiliar, sequencial, paralelo_threads e paralelo_processos
#             compilados no mesmo diretório.

set -e

# Configurações
REPETICOES=10
THREADS_LISTA=(2 4 8 16)
PROCESSOS_LISTA=(2 4 8 16)
TEMPO_MINIMO_SEQ=180   # 3 minutos em segundos
DIR_MATRIZES="matrizes"
DIR_RESULTADOS="resultados"
RELATORIO="relatorio_etapa2.csv"

mkdir -p "$DIR_MATRIZES" "$DIR_RESULTADOS"


# Funções utilitárias

# Extrai o tempo (última linha) de um arquivo de resultado
extrair_tempo() {
    tail -n 1 "$1"
}

# Extrai o maior tempo entre vários arquivos parciais (threads/processos)
maior_tempo() {
    local base="$1"
    local n="$2"
    local max=0
    for ((i=0; i<n; i++)); do
        t=$(extrair_tempo "${base}_${i}.txt")
        max=$(awk -v a="$max" -v b="$t" 'BEGIN { print (a+0 > b+0) ? a : b }')
    done
    echo "$max"
}

log() { echo "[$(date '+%H:%M:%S')] $*"; }

# Cabeçalho do relatório CSV
echo "tamanho,algoritmo,T_ou_P,repeticao,tempo_segundos" > "$RELATORIO"

# PASSO 1 – Descobrir o maior tamanho onde sequencial >= 3 minutos
log "=== PASSO 1: Determinando tamanho máximo (seq >= ${TEMPO_MINIMO_SEQ}s) ==="

DIM=100
ULTIMO_DIM=0

while true; do
    M1="${DIR_MATRIZES}/m1_${DIM}.txt"
    M2="${DIR_MATRIZES}/m2_${DIM}.txt"

    log "Gerando matrizes ${DIM}x${DIM}..."
    ./auxiliar "$DIM" "$DIM" "$DIM" "$DIM"
    mv matrix1.txt "$M1"
    mv matrix2.txt "$M2"

    log "Testando sequencial ${DIM}x${DIM} (1 execução de sondagem)..."
    RES_SONDA="${DIR_RESULTADOS}/sonda_seq_${DIM}.txt"
    ./sequencial "$M1" "$M2" "$RES_SONDA"
    T_SONDA=$(extrair_tempo "$RES_SONDA")
    log "  Tempo sonda: ${T_SONDA}s"

    PASSOU=$(awk -v t="$T_SONDA" -v lim="$TEMPO_MINIMO_SEQ" \
             'BEGIN { print (t+0 >= lim+0) ? "1" : "0" }')

    if [ "$PASSOU" = "1" ]; then
        log "Tamanho ${DIM}x${DIM} atingiu >= 3 minutos. Parando busca."
        ULTIMO_DIM="$DIM"
        break
    fi

    ULTIMO_DIM="$DIM"
    DIM=$((DIM * 2))
done

log "Maior tamanho a testar: ${ULTIMO_DIM}x${ULTIMO_DIM}"

# PASSO 2 – Garante que todos os pares de matrizes até ULTIMO_DIM existem
log "=== PASSO 2: Garantindo todos os pares de matrizes até ${ULTIMO_DIM}x${ULTIMO_DIM} ==="

TAMANHOS=()
D=100
while [ "$D" -le "$ULTIMO_DIM" ]; do
    TAMANHOS+=("$D")
    M1="${DIR_MATRIZES}/m1_${D}.txt"
    M2="${DIR_MATRIZES}/m2_${D}.txt"
    if [ ! -f "$M1" ] || [ ! -f "$M2" ]; then
        log "Gerando matrizes ${D}x${D}..."
        ./auxiliar "$D" "$D" "$D" "$D"
        mv matrix1.txt "$M1"
        mv matrix2.txt "$M2"
    else
        log "Matrizes ${D}x${D} já existem, reutilizando."
    fi
    D=$((D * 2))
done

log "Tamanhos a testar: ${TAMANHOS[*]}"

# PASSO 3 – Testes sequenciais (10x cada tamanho)
log "=== PASSO 3: Testes SEQUENCIAIS (${REPETICOES} repeticoes cada) ==="

for DIM in "${TAMANHOS[@]}"; do
    M1="${DIR_MATRIZES}/m1_${DIM}.txt"
    M2="${DIR_MATRIZES}/m2_${DIM}.txt"
    SOMA=0

    for ((r=1; r<=REPETICOES; r++)); do
        RES="${DIR_RESULTADOS}/seq_${DIM}_r${r}.txt"
        ./sequencial "$M1" "$M2" "$RES" > /dev/null
        T=$(extrair_tempo "$RES")
        SOMA=$(awk -v s="$SOMA" -v t="$T" 'BEGIN { printf "%.9f", s+t }')
        echo "${DIM},sequencial,1,${r},${T}" >> "$RELATORIO"
        log "  seq ${DIM}x${DIM} rep ${r}/${REPETICOES}: ${T}s"
    done

    MEDIA=$(awk -v s="$SOMA" -v n="$REPETICOES" 'BEGIN { printf "%.9f", s/n }')
    log "  >> Media sequencial ${DIM}x${DIM}: ${MEDIA}s"
done

# PASSO 4 – Testes PARALELO THREADS (T=2,4,8,16 | 10x cada)
log "=== PASSO 4: Testes PARALELO THREADS ==="

for T_VAL in "${THREADS_LISTA[@]}"; do
    log "--- Threads T=${T_VAL} ---"
    for DIM in "${TAMANHOS[@]}"; do
        M1="${DIR_MATRIZES}/m1_${DIM}.txt"
        M2="${DIR_MATRIZES}/m2_${DIM}.txt"
        SOMA=0

        for ((r=1; r<=REPETICOES; r++)); do
            BASE="${DIR_RESULTADOS}/thr_${DIM}_T${T_VAL}_r${r}"
            ./paralelo_threads "$M1" "$M2" "$T_VAL" "$BASE" > /dev/null
            T=$(maior_tempo "$BASE" "$T_VAL")
            SOMA=$(awk -v s="$SOMA" -v t="$T" 'BEGIN { printf "%.9f", s+t }')
            echo "${DIM},threads,${T_VAL},${r},${T}" >> "$RELATORIO"
            log "  thr ${DIM}x${DIM} T=${T_VAL} rep ${r}/${REPETICOES}: ${T}s"
        done

        MEDIA=$(awk -v s="$SOMA" -v n="$REPETICOES" 'BEGIN { printf "%.9f", s/n }')
        log "  >> Media threads ${DIM}x${DIM} T=${T_VAL}: ${MEDIA}s"
    done
done

# PASSO 5 – Testes PARALELO PROCESSOS (P=2,4,8,16 | 10x cada)
log "=== PASSO 5: Testes PARALELO PROCESSOS ==="

for P_VAL in "${PROCESSOS_LISTA[@]}"; do
    log "--- Processos P=${P_VAL} ---"
    for DIM in "${TAMANHOS[@]}"; do
        M1="${DIR_MATRIZES}/m1_${DIM}.txt"
        M2="${DIR_MATRIZES}/m2_${DIM}.txt"
        SOMA=0

        for ((r=1; r<=REPETICOES; r++)); do
            BASE="${DIR_RESULTADOS}/proc_${DIM}_P${P_VAL}_r${r}"
            ./paralelo_processos "$M1" "$M2" "$P_VAL" "$BASE" > /dev/null
            T=$(maior_tempo "$BASE" "$P_VAL")
            SOMA=$(awk -v s="$SOMA" -v t="$T" 'BEGIN { printf "%.9f", s+t }')
            echo "${DIM},processos,${P_VAL},${r},${T}" >> "$RELATORIO"
            log "  proc ${DIM}x${DIM} P=${P_VAL} rep ${r}/${REPETICOES}: ${T}s"
        done

        MEDIA=$(awk -v s="$SOMA" -v n="$REPETICOES" 'BEGIN { printf "%.9f", s/n }')
        log "  >> Media processos ${DIM}x${DIM} P=${P_VAL}: ${MEDIA}s"
    done
done

# PASSO 6 – Resumo das médias no terminal
log "=== PASSO 6: Resumo das medias ==="
echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║               RESUMO – TEMPOS MÉDIOS (segundos)             ║"
echo "╠══════════════╦══════════════╦═══════════╦════════════════════╣"
echo "║   Tamanho    ║  Algoritmo   ║  T ou P   ║   Tempo Médio (s)  ║"
echo "╠══════════════╬══════════════╬═══════════╬════════════════════╣"

awk -F',' '
NR > 1 {
    key = $1 "," $2 "," $3
    soma[key] += $5
    cnt[key]++
}
END {
    for (k in soma) {
        split(k, a, ",")
        printf "║ %12s ║ %12s ║ %9s ║ %18.6f ║\n", a[1]"x"a[1], a[2], a[3], soma[k]/cnt[k]
    }
}
' "$RELATORIO" | sort -t'|' -k1

echo "╚══════════════╩══════════════╩═══════════╩════════════════════╝"
echo ""
log "Relatório completo salvo em: $RELATORIO"
log "=== ETAPA 2 CONCLUÍDA ==="
