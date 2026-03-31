# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Academic project for a High-Performance Computing course (Computação de Alto Desempenho), 8th semester. Written in **C**, documenting **performance experiments** that explore cache behavior, instruction-level parallelism (ILP), compiler optimizations, and algorithmic efficiency. Reports are written in Portuguese.

## Build Commands

No Makefile or build system — compile directly with GCC:

```bash
# Basic compilation
gcc -o atv-01/main.exe atv-01/main.c

# Activity 2 requires multiple optimization levels for comparison
gcc -O0 -o atv-02/main_O0.exe atv-02/main.c
gcc -O2 -o atv-02/main_O2.exe atv-02/main.c
gcc -O3 -o atv-02/main_O3.exe atv-02/main.c
```

VS Code task (in `helloworld/.vscode/tasks.json`) uses:
```
gcc.exe -fdiagnostics-color=always -g ${file} -o ${fileDirname}\${fileBasenameNoExtension}.exe
```

Results are collected by running the compiled binaries and recording timing output manually.

## Architecture & Structure

Each `atv-XX/` directory is a self-contained experiment with:
- `main.c` — the C source implementing the experiment
- `relatorio.md` — the written report (theory, results, analysis)
- Compiled `.exe` binaries (Windows, committed to repo)

### Experiments

| Dir | Concept | Key technique |
|-----|---------|---------------|
| `atv-01` | Cache locality | Row-major vs. column-major matrix traversal; 5000×5000 matrices show ~3× difference |
| `atv-02` | ILP & vectorization | Three loop variants: independent, sequential-dependency, multiple-accumulators; tested at -O0/-O2/-O3 |
| `atv-03` | Algorithmic convergence | Leibniz series for π; O(1/n) convergence from 100 to 1B iterations |
| `atv-04` | (in progress) | Source only, no report yet |

### Report structure (`relatorio.md`)

Each report follows: theoretical background → code explanation → results table → analysis → appendix with full source listing.

## Development Notes

- Compiler optimization flags are part of the experiment design in `atv-02` — don't apply `-O2`/`-O3` globally without intent
- Timing is measured inside the C programs using `clock()` or similar; results are platform-dependent (Windows)
- Code comments and variable names may be in Portuguese
