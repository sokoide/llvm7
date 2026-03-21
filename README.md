# LLVM7 Compiler

C言語で書かれた、LLVM IR を出力する C コンパイラです。
セルフホスト (Self-hosting) を達成しており、自分のソースコードをコンパイルして動作するバイナリを生成できます。

## 特徴

- **C Compiler Frontend**: C言語のサブセットをサポートし、LLVM IR (.ll) を生成します。
- **Self-Hosting**: `llvm7` コンパイラ自身を用いて、`llvm7` のソースコードをコンパイル可能です (Stage 2)。
- **LLVM Backend**: 生成された LLVM IR は `llc` や `clang` を用いてネイティブバイナリに変換・リンクされます。

## 必要要件

- C コンパイラ (clang 推奨)
- LLVM (llvm-config コマンドが使えること)
- Make

## ビルド方法

### Stage 1: ホストコンパイラによるビルド

システム標準のコンパイラ (clang/gcc) を使って `llvm7` をビルドします。

```bash
make
```

これにより `build/llvm7` が生成されます。

### Stage 2: セルフホストビルド

Stage 1 で生成された `build/llvm7` を使って、コンパイラ自身のソースコードをコンパイルします。

```bash
make selfhost
```

これにより以下の処理が行われます。

1. `src/*.c` を `build/llvm7` でコンパイルし、LLVM IR (`selfhost/build/*.ll`) を生成
2. 生成された IR をリンクして `build/llvm7_selfhost` を生成

## 実行方法

```bash
./build/llvm7 input.c [-o output.ll]
```

生成した LLVM IR は以下のように実行ファイルに変換できます。

```bash
# LLVM IR の生成
./build/llvm7 test.c -o test.ll

# リンク（推奨: IR を直接 clang に渡す）
clang test.ll -o test
./test

# アセンブル経由で確認したい場合
llc test.ll -o test.s
clang test.s -o test
./test
```

### デモコードをIR→バイナリ化して試す

`demo/example01.c` などのサンプルは `build/llvm7` で直接 IR に変換し、`llc` + `clang` でバイナリ化できます。以下が典型的なワークフローです。

```bash
# IR を生成（出力ファイルは `demo/example01.ll`）
./build/llvm7 demo/example01.c -o demo/example01.ll

# llc でアセンブル、clang でリンク
llc demo/example01.ll -o demo/example01.s
clang demo/example01.s -o demo/example01

# 実行例（終了コード 4 が期待値）
./demo/example01
echo $?
```

`example01`〜`example09` には期待される終了コードがあるため、`make selfhost_demo_check` を使えばセルフホストバイナリで IR 生成から実行→コード一致まで自動的に確認できます。

### 注意：外部プリプロセッサは不要

`llvm7` 本体に簡易プリプロセッサを実装しているため、`#include` や `#define` を含む `.c` をそのまま入力できます。
たとえば：

```bash
./build/llvm7 demo/example10.c -o demo/example10.ll
llc demo/example10.ll -o demo/example10.s
clang demo/example10.s -o demo/example10
./demo/example10
```

### stdio の例

セルフホスト環境は `stdio.h` や `printf` 宣言を `selfhost/include/stdio.h` などに用意しているので、`demo/stdio.c` のように `printf` を呼ぶコードも試せます。

```c
#include <stdio.h>

int main() {
    printf("answer=%d\n", 11);
    return 11;
}
```

これをコンパイルして実行するには：

```bash
./build/llvm7 demo/stdio.c -o demo/stdio.ll
llc demo/stdio.ll -o demo/stdio.s
clang demo/stdio.s -o demo/stdio -lc
./demo/stdio
```

`printf` の宣言が足りない場合は `selfhost/include/stdio.h` を必要な関数で拡張してください。

## サポートされている C の機能

- `int`/`char`/`float`/`double`/`long`/`void` と、`unsigned`/`signed`、`const`/`volatile`/`restrict`/`inline`/`register`/`_Bool` などのキーワード認識。
- ポインタ・配列・構造体・`union`・列挙型の定義・代入・読み出し。
- グローバル変数とローカル変数。ローカルはスタックに `alloca` し、初期化式は `=` または配列リテラルで記述可。
- 制御構文: `if`、`else`、`while`、`for`、`return`、`break`、`continue`。入れ子構造とブロック `{}` に対応。
- `goto` とラベル文（`label: stmt`）。
- 関数定義と呼び出し。引数は先頭にリスト状に集まる。可変長引数 (`...`) と外部関数の宣言・呼び出しも一部サポート。
- ポインタ演算 (`+`/`-`) や `*`/`&` 演算子、配列のインデックス、構造体/`union` のメンバ (`.`/`->`)。
- `typedef` による型エイリアス、`struct`/`enum` タグ、列挙子の解決。
- 関数ポインタの基本ケース（ローカル宣言 `int (*fp)(int);`、代入、間接呼び出し `fp(...)`）。
- 文字列リテラル・文字リテラルのエスケープ処理、行コメント `//` とブロックコメント `/* ... */`。
- プリプロセッサ（`#include`、`#define`、関数形式マクロ、`#undef`、`#ifdef`/`#ifndef`/`#if`/`#elif`/`#else`/`#endif`、`#pragma` 無視、`#error`）。可変長マクロ引数（`...`）と `__VA_ARGS__` もサポート。
- ビットフィールド宣言（現在の実装ではビット単位パッキングは未対応で、独立メンバとして扱います）。
- 指定イニシャライザ（配列の `[index] = value`、構造体の `.member = value`）。
- 複合リテラル（スカラー/構造体/union/配列、`(int[]){...}` の要素数推論を含む）。
- `long double`（現状は `double` と同等精度で処理）。
- `_Complex`（現状は実数部のみを `float`/`double` として処理）。
- 可変長配列 VLA（現状はローカル1次元宣言 `int a[n];` の基本利用と `sizeof(a)` の実行時評価をサポート、初期化子は未対応）。

## C99未実装

- 可変長配列 (VLA) の高度ケース（多次元、初期化子）。
- `switch` の一部高度ケース、関数ポインタの複雑な宣言・呼び出し（多段ネスト、可変引数絡みなど）。
- プリプロセッサの高度機能（`##`、`#`、再帰展開の完全互換など）。
- ビットフィールドの厳密なABI互換パッキング/符号拡張。
- 標準ライブラリのヘッダ全体は含まれず、必要なら `selfhost/include` 以下のミニマルな宣言を使って手動で補ってください。
