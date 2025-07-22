
# Timestamp Scheduler

Um simulador em C++ para controle de concorrência utilizando o **Protocolo de Ordenação por Timestamp**.

Este programa lê uma lista de objetos de banco de dados, transações e escalonamentos a partir de um arquivo de entrada estruturado. Em seguida, simula a execução com base em timestamps, identifica conflitos e registra operações por objeto.

---

## Funcionalidades

- Leitura de objetos, transações e timestamps a partir de um arquivo
- Processamento de operações `read`, `write` e `commit`
- Detecção de conflitos com base em ordenação por timestamp
- Saída com o status de cada escalonamento
- Geração de arquivos de log individuais por objeto

---

## Estrutura do Projeto

Timestamp-Scheduler/
├── CMakeLists.txt # Arquivo de configuração do CMake
├── main.cpp # Código-fonte principal
├── in.txt # Arquivo de entrada (exemplo abaixo)
├── out.txt # Arquivo de saída (gerado após execução)
├── obj_logs/ # Diretório com logs por objeto (gerado automaticamente)
├── build/ # Diretório de build do CMake
└── README.md # Este arquivo

## Requisitos

- **CMake** 3.12 ou superior  
- Compilador compatível com **C++17** (GCC 7+, Clang 5+, MSVC 2017+)

---


## Compilação

From root 
```bash
mkdir -p build
cd build

cmake -S . -B build

cmake --build build
```
Run: 
```bash
./build/Timestamp-Scheduler
```

## Formato de entrada

# Objects; 
A,B,C; 
# Transactions;
t1,t2;
# Timestamps;
5,10;

E_1 - r1(A), w1(B), r2(C), c1, c2
E_2 - w2(B), r1(A), c1, c2

Detalhes:

    Objetos: nomes separados por vírgulas

    Transações: t1, t2, etc.

    Timestamps: um valor por transação, na mesma ordem

    Escalonamentos:

        r1(A) → leitura do objeto A pela transação 1

        w2(B) → escrita no objeto B pela transação 2

        c1 → commit da transação 1
## Saída

out.txt

Cada linha informa o resultado do escalonamento:
```
E_1-OK
E_2-ROLLBACK-1
```

obj_logs/

Arquivos .txt para cada objeto, indicando as operações realizadas.

Exemplo: obj_logs/A.txt
```
E_1, Read, 0
E_2, Write, 1
```

## Exemplo

Entrada
```
# Objetos;
X, Y, Z;
# Transações;
t1, t2, T3;
# Timestamps;
5, 10, 3;
# Escalonamentos
E_1-r1(X) R2(Y) w2(Y) r3(Y) w1(X) c1
E_2-w2(X) R1(Y) w3(X) r2(Z) w1(Z) c1
E_3-r3(X) W3(Y) c1 r1(X) w1(Y) c2 r2(Y) w2(Z) c3
```

Saída
```
=== INFORMAÇÕES CARREGADAS === 
Objetos: X, Y, Z
Transações: t1, t2, t3
Timestamps: 5, 10, 3
Mapeamento Transação -> Timestamp:
  t1 -> 5
  t2 -> 10
  t3 -> 3
=== PROCESSANDO E_1 ===
Escalonamento: E_1-r1(X) R2(Y) w2(Y) r3(Y) w1(X) c1
Momento 0: r1(X) [TS=5] -> Verificando: TS(t1)=5 vs TS_write(X)=-1 -> OK, TS_read(X) = 5
Momento 1: r2(Y) [TS=10] -> Verificando: TS(t2)=10 vs TS_write(Y)=-1 -> OK, TS_read(Y) = 10
Momento 2: w2(Y) [TS=10] -> Verificando: TS(t2)=10 vs TS_read(Y)=10 e TS_write(Y)=-1 -> OK, TS_write(Y) = 10
Momento 3: r3(Y) [TS=3] -> Verificando: TS(t3)=3 vs TS_write(Y)=10 -> CONFLITO! Rollback necessário
RESULTADO: E_1-ROLLBACK-3
==================================================
=== PROCESSANDO E_2 ===
Escalonamento: E_2-w2(X) R1(Y) w3(X) r2(Z) w1(Z) c1
Momento 0: w2(X) [TS=10] -> Verificando: TS(t2)=10 vs TS_read(X)=-1 e TS_write(X)=-1 -> OK, TS_write(X) = 10
Momento 1: r1(Y) [TS=5] -> Verificando: TS(t1)=5 vs TS_write(Y)=-1 -> OK, TS_read(Y) = 5
Momento 2: w3(X) [TS=3] -> Verificando: TS(t3)=3 vs TS_read(X)=-1 e TS_write(X)=10 -> CONFLITO! Rollback necessário
RESULTADO: E_2-ROLLBACK-2
==================================================
=== PROCESSANDO E_3 ===
Escalonamento: E_3-r3(X) W3(Y) c1 r1(X) w1(Y) c2 r2(Y) w2(Z) c3
Momento 0: r3(X) [TS=3] -> Verificando: TS(t3)=3 vs TS_write(X)=-1 -> OK, TS_read(X) = 3
Momento 1: w3(Y) [TS=3] -> Verificando: TS(t3)=3 vs TS_read(Y)=-1 e TS_write(Y)=-1 -> OK, TS_write(Y) = 3
Momento 2: c1 (commit)
Momento 3: r1(X) [TS=5] -> Verificando: TS(t1)=5 vs TS_write(X)=-1 -> OK, TS_read(X) = 5
Momento 4: w1(Y) [TS=5] -> Verificando: TS(t1)=5 vs TS_read(Y)=-1 e TS_write(Y)=3 -> OK, TS_write(Y) = 5
Momento 5: c2 (commit)
Momento 6: r2(Y) [TS=10] -> Verificando: TS(t2)=10 vs TS_write(Y)=5 -> OK, TS_read(Y) = 10
Momento 7: w2(Z) [TS=10] -> Verificando: TS(t2)=10 vs TS_read(Z)=-1 e TS_write(Z)=-1 -> OK, TS_write(Z) = 10
Momento 8: c3 (commit)
--- Estado da Estrutura de Timestamps para E_3 ---
Objeto    TS-Read     TS-Write
----------------------------------
X         5           NULL
Y         10          5
Z         NULL        10
RESULTADO: E_3-OK
==================================================
=== PROCESSAMENTO CONCLUÍDO ===
Arquivo de saída gerado: out.txt
Arquivos de log dos objetos:
  - X.txt
  - Y.txt
  - Z.txt
Verifique os arquivos gerados para os resultados completos.                         
```
out.txt:
```
E_1-ROLLBACK-3
E_2-ROLLBACK-2
E_3-OK
```

obj_logs/X.txt:
```
E_1, Read, 0
E_2, Write, 0
E_2, Write, 2
E_3, Read, 0
E_3, Read, 3
```
