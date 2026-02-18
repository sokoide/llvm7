# Repository Guidelines

## Project Structure & Module Organization
- `src/` holds the compiler implementation (`lex.c`, `parse.c`, `codegen.c`, etc.).
- `demo/` contains small C programs that exercise supported features and serve as manual regressions.
- `test/` lives beside a custom `Makefile` that builds `build/test_runner`; tests compile `src/*.c` alongside dedicated helper files.
- `selfhost/` stores headers for the bootstrap compiler, temporary build artifacts, and the demo-check harness.
- Generated outputs (`build/`, `selfhost/bootstrap/`, `selfhost/build/`, `selfhost/demo_check/`) are ignored by `.gitignore` and should not be committed.

## Build, Test, and Development Commands
- `make` – stage-1 build using system `clang`; produces `build/llvm7`.
- `make selfhost` – uses `build/llvm7` to preprocess `src/*.c`, emit IR, and build `build/llvm7_selfhost`.
- `make bootstrap_compare` – runs the full bootstrap pipeline (tc1 vs tc2 IR) and diffs outputs.
- `make selfhost_demo_check` – preprocesses `demo/*.c`, compiles via the self-hosted compiler, links, and verifies exit codes for known demos.
- `make test` – runs `test/Makefile`, building the `test_runner` that executes all lexer/parse/codegen unit tests.

## Coding Style & Naming Conventions
- Follow conventional C style: 4-space indentation, braces on the same line, descriptive variable names (`node`, `ctx`, `builder`).
- Use uppercase for `#include` identifiers (`#include "parse.h"`), keep functions in `snake_case`, types in `CamelCase` when referring to structs (`Type`, `Node`).
- Keep compiler helpers modular (lex, parse, codegen) and avoid mixing UI/output logic with parsing.
- No automated formatter; rely on manual alignment and clarity. Watch for consistent `ctx->` usage when threading state.

## Testing Guidelines
- Tests live under `test/` and use the micro-unit helper suite (`mu_assert`). Each test returns `NULL` on success.
- Name new tests `test_<scope>_<behavior>` for clarity (e.g., `test_generate_switch_case_after_return_case`).
- Run `make test` whenever modifying parsing/codegen; target line coverage is functional rather than numeric.
- Add regression data to `test/common` helpers when new fixtures or execution contexts are required.

## Commit & Pull Request Guidelines
- Commit messages should be concise, prefixed if needed (“fix:”/“feat:” etc.), and mention the key change (e.g., `fix: sync tc2 string linkage`).
- Pull requests should describe the bootstrap impact, any new demo/test targets touched, and reference relevant issues or bootstrap logs.
- Include command outputs when reporting regressions (e.g., `make bootstrap_compare` diffs, `make test` results).

## Additional Notes
- Manual preprocessing is required for `#include`/`#define`: expand with `clang -E` before passing to `build/llvm7`. Documented in `README.md`.
- Keep `selfhost/include/*.h` aligned with LLVM C APIs and host headers; missing prototypes can crash tc2 generation.
