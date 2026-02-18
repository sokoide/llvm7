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

# アセンブルとリンク (システムにより異なります)
llc test.ll -o test.s
clang test.s -o test
./test

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

### 注意：`#include` があるソースは事前にプリプロセスする

`llvm7` 本体にはプリプロセッサがないため、`#include` や `#define` を含むファイル（例：`demo/example10.c`）をそのまま実行すると `lex error: Invalid character '#'` で止まり、最悪セグフォルトに至ります。
`clang -E` や `gcc -E` でソースを `.i` に展開した後、その `.i` を入力してください。たとえば：

```bash
clang -Iselfhost/include -E -P demo/example10.c -o demo/example10.i
./build/llvm7 demo/example10.i -o demo/example10.ll
llc demo/example10.ll -o demo/example10.s
clang demo/example10.s -o demo/example10
./demo/example10
```

展開後の `.i` では `stdio.h` などの宣言も文字列として含まれるので、`selfhost/include` にある最小限のヘッダで処理できます。

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
clang -Iselfhost/include -E -P demo/stdio.c -o demo/stdio.i
./build/llvm7 demo/stdio.i -o demo/stdio.ll
llc demo/stdio.ll -o demo/stdio.s
clang demo/stdio.s -o demo/stdio -lc
./demo/stdio
```

`printf` の宣言が足りない場合は `selfhost/include/stdio.h` を必要な関数で拡張してください。

```

## サポートされている C の機能

- `int`/`char` 型のスカラーと、ポインタ・配列・構造体・列挙型の定義・代入・読み出し。
- グローバル変数とローカル変数。ローカルはスタックに `alloca` し、初期化式は `=` または配列リテラルで記述可。
- 制御構文: `if`、`else`、`while`、`for`、`return`、`break`、`continue`。入れ子構造とブロック `{}` に対応。
- 関数定義と呼び出し。引数は先頭にリスト状に集まる。可変長引数 (`...`) と外部関数の宣言・呼び出しも一部サポート。
- ポインタ演算 (`+`/`-`) や `*`/`&` 演算子、配列のインデックス、構造体のメンバ (`.`/`->`)。
- `typedef` による型エイリアス、`struct`/`enum` タグ、列挙子の解決。
- 文字列リテラル・文字リテラルのエスケープ処理、行コメント `//` とブロックコメント `/* ... */`。
- 単純なプリプロセッサなし。ただし `clang -E` で preprocessing した `.i` ファイルをそのまま入力できます。

## サポートされていない C の機能

- `union`、ビットフィールド、複雑なプリプロセッサディレクティブ（`#ifdef`/`#elif` を含むマクロ展開）は未実装。またプリプロセッサ本体を持たないため、`#include`/`#define` を含むソースは事前に `clang -E` で展開してください。
- `long double` や浮動小数点 (`float`/`double`) 型はサポートしていません。
- `goto`、構造体の未名前付きメンバ、関数ポインタの複雑なキャストや呼び出しなど高度な制御構造は不完全です。
- 標準ライブラリのヘッダ全体は含まれず、必要なら `selfhost/include` 以下のミニマルな宣言を使って手動で補ってください。
