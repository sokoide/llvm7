#include "preprocess_test.h"
#include "../src/preprocess.h"
#include "test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* test_preprocess_noop() {
    const char* input = "int main() { return 0; }";
    char* output = preprocess(input, "test.c");

    mu_assert("Output should not be NULL", output != NULL);
    mu_assert("Output should match input for no-op",
              strcmp(input, output) == 0);

    // free(output); // Assuming preprocess returns a new string
    return NULL;
}

char* test_preprocess_include() {
    const char* input =
        "#include \"dummy.h\"\nint main() { return dummy_val; }";
    char* output = preprocess(input, "main.c");

    mu_assert("Output should contain dummy.h content",
              strstr(output, "int dummy_val = 42;") != NULL);
    mu_assert("Output should contain original content",
              strstr(output, "int main() { return dummy_val; }") != NULL);

    return NULL;
}

char* test_preprocess_define() {
    const char* input = "#define FOO 42\nint x = FOO;";
    char* output = preprocess(input, "test.c");

    mu_assert("Output should contain replaced value",
              strstr(output, "int x = 42;") != NULL);
    mu_assert("Output should not contain FOO",
              strstr(output, "FOO") == NULL ||
                  strstr(output, "#define") != NULL);

    return NULL;
}

char* test_preprocess_ifdef() {
    const char* input =
        "#define FOO\n#ifdef FOO\nint x = 1;\n#else\nint x = "
        "2;\n#endif\n#ifdef BAR\nint y = 1;\n#else\nint y = 2;\n#endif";
    char* output = preprocess(input, "test.c");

    mu_assert("Output should contain x = 1",
              strstr(output, "int x = 1;") != NULL);
    mu_assert("Output should NOT contain x = 2",
              strstr(output, "int x = 2;") == NULL);
    mu_assert("Output should NOT contain y = 1",
              strstr(output, "int y = 1;") == NULL);
    mu_assert("Output should contain y = 2",
              strstr(output, "int y = 2;") != NULL);

    return NULL;
}

char* test_preprocess_no_expand_in_literals_or_comments() {
    const char* input = "#define FOO 42\n"
                        "int x = FOO;\n"
                        "char* s = \"FOO\";\n"
                        "char c = 'F';\n"
                        "// FOO should stay in comment\n"
                        "/* FOO should stay in block comment */\n";
    char* output = preprocess(input, "test.c");

    mu_assert("FOO in code should be expanded",
              strstr(output, "int x = 42;") != NULL);
    mu_assert("FOO in string should NOT be expanded",
              strstr(output, "char* s = \"FOO\";") != NULL);
    mu_assert("FOO in line comment should NOT be expanded",
              strstr(output, "// FOO should stay in comment") != NULL);
    mu_assert("FOO in block comment should NOT be expanded",
              strstr(output, "/* FOO should stay in block comment */") != NULL);

    free(output);
    return NULL;
}

char* test_preprocess_macro_scope_is_per_call() {
    char* out1 = preprocess("#define A 1\nint x = A;\n", "a.c");
    mu_assert("first call should expand A", strstr(out1, "int x = 1;") != NULL);
    free(out1);

    char* out2 = preprocess("int y = A;\n", "b.c");
    mu_assert("macro A should not leak to second call",
              strstr(out2, "int y = A;") != NULL);
    free(out2);
    return NULL;
}

char* test_preprocess_long_define_value() {
    char value[1501];
    for (int i = 0; i < 1500; i++)
        value[i] = 'x';
    value[1500] = '\0';

    char input[1700];
    snprintf(input, sizeof(input), "#define BIG %s\nchar* s = BIG;\n", value);
    char* output = preprocess(input, "long.c");

    mu_assert("long macro value should be expanded",
              strstr(output, value) != NULL);
    free(output);
    return NULL;
}

char* test_preprocess_if_elif_expr() {
    const char* input = "#define A 0\n"
                        "#if A\n"
                        "int x = 1;\n"
                        "#elif defined(A)\n"
                        "int x = 2;\n"
                        "#else\n"
                        "int x = 3;\n"
                        "#endif\n";
    char* output = preprocess(input, "expr.c");
    mu_assert("elif branch should be selected",
              strstr(output, "int x = 2;") != NULL);
    mu_assert("if branch should be removed",
              strstr(output, "int x = 1;") == NULL);
    mu_assert("else branch should be removed",
              strstr(output, "int x = 3;") == NULL);
    free(output);
    return NULL;
}

char* test_preprocess_function_macro_and_undef() {
    const char* input = "#define ADD(x,y) ((x)+(y))\n"
                        "int z = ADD(2, 3);\n"
                        "#undef ADD\n"
                        "int k = ADD;\n";
    char* output = preprocess(input, "fn.c");
    mu_assert("function-like macro should expand",
              strstr(output, "int z = ((2)+(3));") != NULL);
    mu_assert("undef should remove macro binding",
              strstr(output, "int k = ADD;") != NULL);
    free(output);
    return NULL;
}

char* test_preprocess_pragma_ignored() {
    const char* input = "#pragma once\nint x = 1;\n";
    char* output = preprocess(input, "p.c");
    mu_assert("pragma line should not remain",
              strstr(output, "#pragma") == NULL);
    mu_assert("normal code should remain",
              strstr(output, "int x = 1;") != NULL);
    free(output);
    return NULL;
}

char* test_preprocess_stringification() {
    const char* input = "#define STR(x) #x\n"
                        "char* s = STR(hello world);\n";
    char* output = preprocess(input, "str.c");
    mu_assert("stringification should quote argument",
              strstr(output, "char* s = \"hello world\";") != NULL);
    free(output);
    return NULL;
}

char* test_preprocess_token_pasting() {
    const char* input = "#define CAT(a,b) a##b\n"
                        "int foobar = 7;\n"
                        "int x = CAT(foo,bar);\n";
    char* output = preprocess(input, "paste.c");
    mu_assert("token pasting should concatenate identifiers",
              strstr(output, "int x = foobar;") != NULL);
    free(output);
    return NULL;
}

char* test_preprocess_recursive_macro_expansion() {
    const char* input = "#define A B\n"
                        "#define B 1\n"
                        "int x = A;\n";
    char* output = preprocess(input, "recur.c");
    mu_assert("recursive macro expansion should reach final token",
              strstr(output, "int x = 1;") != NULL);
    free(output);
    return NULL;
}

char* test_preprocess_variadic_macro_basic() {
    const char* input = "#define DEBUG(fmt, ...) fmt, __VA_ARGS__\n"
                        "int x = DEBUG(1, 2, 3);\n";
    char* output = preprocess(input, "var.c");
    // Note: variadic args preserve original spacing from parse_macro_args
    // which trims spaces, so "2, 3" becomes "2,3"
    mu_assert("variadic macro should expand __VA_ARGS__",
              strstr(output, "int x = 1, 2,3;") != NULL);
    free(output);
    return NULL;
}

char* test_preprocess_variadic_macro_single_arg() {
    // Case: only format argument, no variadic args
    // In C99, __VA_ARGS__ expands to nothing when empty
    const char* input = "#define LOG(fmt, ...) fmt\n"
                        "int x = LOG(42);\n";
    char* output = preprocess(input, "var2.c");
    mu_assert("variadic macro with no extra args should work",
              strstr(output, "int x = 42;") != NULL);
    free(output);
    return NULL;
}

char* test_preprocess_variadic_macro_in_stringify() {
    const char* input = "#define PRINT(fmt, ...) printf(fmt, __VA_ARGS__)\n"
                        "PRINT(\"%d %d\", a, b);\n";
    char* output = preprocess(input, "var3.c");
    // Note: variadic args preserve original spacing from parse_macro_args
    // which trims spaces, so "a, b" becomes "a,b"
    mu_assert("variadic macro with printf-style call should expand",
              strstr(output, "printf(\"%d %d\", a,b);") != NULL);
    free(output);
    return NULL;
}

char* test_preprocess_variadic_macro_zero_fixed_params() {
    // #define VA_ONLY(...) with no fixed params
    const char* input = "#define VA_ONLY(...) __VA_ARGS__\n"
                        "int x = VA_ONLY(a, b, c);\n";
    char* output = preprocess(input, "var4.c");
    mu_assert("variadic-only macro should expand all args",
              strstr(output, "int x = a,b,c;") != NULL);
    free(output);
    return NULL;
}

char* test_preprocess_variadic_macro_stringify_va_args() {
    // #__VA_ARGS__ should stringify all variadic args as one string
    const char* input = "#define TOSTR(fmt, ...) #__VA_ARGS__\n"
                        "const char* s = TOSTR(fmt, 1, 2);\n";
    char* output = preprocess(input, "var5.c");
    mu_assert("stringify of __VA_ARGS__ should produce quoted string",
              strstr(output, "const char* s = \"1,2\";") != NULL);
    free(output);
    return NULL;
}

char* test_preprocess_variadic_macro_gnu_comma_suppression() {
    // , ##__VA_ARGS__ GCC extension: suppress comma when no variadic args
    const char* input = "#define LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)\n"
                        "LOG(\"hello\");\n"
                        "LOG(\"val=%d\", 42);\n";
    char* output = preprocess(input, "var6.c");
    mu_assert("no variadic args: comma should be suppressed",
              strstr(output, "printf(\"hello\");") != NULL);
    mu_assert("with variadic args: comma and args should appear",
              strstr(output, "printf(\"val=%d\",42);") != NULL);
    free(output);
    return NULL;
}
