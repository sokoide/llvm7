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
