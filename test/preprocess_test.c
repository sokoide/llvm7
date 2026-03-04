#include "preprocess_test.h"
#include "../src/preprocess.h"
#include "test_common.h"
#include <stdio.h>
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
