# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

C コンパイラ (llvm7)。C ソース → LLVM IR (.ll) を生成。セルフホスト達成済み（自分自身をコンパイル可能）。

## Build & Run Commands

```bash
make                  # Stage 1: build/llvm7 を生成
make run              # build/llvm7 で demo/example02.c をコンパイル
make run DEMO=path    # 任意のデモファイルを指定
make test             # test/ 下のユニットテスト全実行
make selfhost         # Stage 2: 自分自身でソースをコンパイル → build/llvm7_selfhost
make selfhost_verify  # Stage 1 と Stage 2 の出力 IR をクイック比較（1ファイル）
make selfhost_test    # セルフホストバイナリで demo/*.c をコンパイル＆実行検証
make bootstrap_compare  # 全ソースコードの IR 出力を比較（Stage 1 vs Stage 2）
make bootstrap_check    # ユニットテスト全実行 + 全ソースの IR 比較（完全性検証）
make format           # clang-format 実行
make clean            # ビルド成果物を削除
```

直接実行:
```bash
./build/llvm7 input.c -o output.ll
clang output.ll -o output `llvm-config --ldflags --libs --system-libs` -lc
```

## Requirements

- clang, LLVM (llvm-config が利用可能であること), Make

## Architecture

コンパイルパイプライン: `ソース → preprocess → tokenize → parse → codegen → LLVM IR`

各フェーズは `Context` 構造体（`common.h`）で状態を共有。

### Source Modules (src/)

| File | Role |
|------|------|
| `main.c` | エントリポイント。パイプライン全体を呼び出し |
| `preprocess.c/.h` | プリプロセッサ（#include, #define, #ifdef 等）。`selfhost/include/` のヘッダを使用 |
| `lex.c/.h` | トークナイザ。ソース → `Token` リスト |
| `parse.c/.h` | 再帰下降パーサ。Token → AST (`Node`)。文法規則は `parse.h` のコメント参照 |
| `codegen.c/.h` | LLVM C API 経由で AST → LLVM IR を生成 |
| `variable.c/.h` | ローカル/グローバル変数の管理（スコープ付き） |
| `file.c/.h` | ファイル読み込みユーティリティ |
| `stdio.c` | セルフホスト用 stdio 宣言 |
| `common.h` | 全モジュール共通の型定義（Token, Node, Type, Context 等） |

### Key Data Structures (common.h)

- `Token`: トークン種別、値、位置情報のリンクリスト
- `Node`: AST ノード。`NodeKind` で種別管理。二項演算は lhs/rhs、制御文は cond/init/lhs(then)/rhs(else) を使用
- `Type`: INT/CHAR/VOID/PTR/STRUCT/UNION/LONG/DOUBLE/FLOAT/BOOL。ポインタ先は `ptr_to`、構造体メンバは `Member` リスト
- `Context`: コンパイル全体の状態。トークン、AST、変数スコープ、typedef、enum、struct tag、switch ネスト等を保持

### Tests (test/)

`mu_assert` ベースの独自テストフレームワーク。`test/main.c` に全テストを登録。
`test/test_common.c` にヘルパー、各 `*_test.c` に lex/parse/codegen/preprocess/file のテスト。
テストは `src/main.c` を除外してビルド。

### Self-hosting (selfhost/)

- `include/`: セルフホスト用の最小限のヘッダ（stdio.h, stdlib.h, stdarg.h 等）
- `bootstrap/`: tc1（Stage 1）/ tc2（Stage 2）の出力を比較するブートストラップ検証用ディレクトリ
- `build/`: セルフホストビルド中間生成物

## Coding Conventions

- C99、4スペースインデント、ブレス同行情報
- snake_case 関数、CamelCase 構造体型（Type, Node）
- コミットメッセージ: `fix:`/`feat:` プレフィックス
