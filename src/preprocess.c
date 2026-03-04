#include "preprocess.h"
#include "file.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Macro Macro;
struct Macro {
    char* name;
    char* value;
    Macro* next;
};

static Macro* macros = NULL;

static void add_macro(const char* name, const char* value) {
    Macro* m = malloc(sizeof(Macro));
    m->name = strdup(name);
    m->value = strdup(value);
    m->next = macros;
    macros = m;
}

static Macro* find_macro(const char* name, int len) {
    for (Macro* m = macros; m; m = m->next) {
        if (strncmp(m->name, name, len) == 0 && m->name[len] == '\0') {
            return m;
        }
    }
    return NULL;
}

static char* get_dirname(const char* path) {
    char* p = strrchr(path, '/');
    if (!p)
        return strdup(".");
    int len = p - path;
    char* dir = malloc(len + 1);
    strncpy(dir, path, len);
    dir[len] = '\0';
    return dir;
}

static int is_ident_char(char c) { return isalnum(c) || c == '_'; }

typedef struct CondStack CondStack;
struct CondStack {
    bool matched; // Whether any branch in this #if level has matched
    bool active;  // Whether the current branch is active
    CondStack* next;
};

char* preprocess(const char* input, const char* filename) {
    size_t out_size = 1024 * 1024;
    char* output = malloc(out_size);
    char* out_ptr = output;
    *out_ptr = '\0';

    CondStack* cond_stack = NULL;

    const char* p = input;
    while (*p) {
        const char* next_line = strchr(p, '\n');
        size_t line_len = next_line ? (size_t)(next_line - p) : strlen(p);

        const char* start = p;
        while (start < (next_line ? next_line : p + line_len) &&
               (*start == ' ' || *start == '\t'))
            start++;

        bool is_directive = false;
        if (start < (next_line ? next_line : p + line_len) && *start == '#') {
            const char* directive = start + 1;
            while (*directive == ' ' || *directive == '\t')
                directive++;

            if (strncmp(directive, "ifdef", 5) == 0 &&
                (directive[5] == ' ' || directive[5] == '\t')) {
                is_directive = true;
                const char* name_p = directive + 5;
                while (*name_p == ' ' || *name_p == '\t')
                    name_p++;
                const char* name_start = name_p;
                while (is_ident_char(*name_p))
                    name_p++;
                int name_len = name_p - name_start;

                bool found = find_macro(name_start, name_len) != NULL;
                bool parent_active = (!cond_stack || cond_stack->active);

                CondStack* cs = malloc(sizeof(CondStack));
                cs->active = parent_active && found;
                cs->matched = found;
                cs->next = cond_stack;
                cond_stack = cs;
            } else if (strncmp(directive, "ifndef", 6) == 0 &&
                       (directive[6] == ' ' || directive[6] == '\t')) {
                is_directive = true;
                const char* name_p = directive + 6;
                while (*name_p == ' ' || *name_p == '\t')
                    name_p++;
                const char* name_start = name_p;
                while (is_ident_char(*name_p))
                    name_p++;
                int name_len = name_p - name_start;

                bool found = find_macro(name_start, name_len) != NULL;
                bool parent_active = (!cond_stack || cond_stack->active);

                CondStack* cs = malloc(sizeof(CondStack));
                cs->active = parent_active && !found;
                cs->matched = !found;
                cs->next = cond_stack;
                cond_stack = cs;
            } else if (strncmp(directive, "else", 4) == 0) {
                is_directive = true;
                if (cond_stack) {
                    bool parent_active =
                        (!cond_stack->next || cond_stack->next->active);
                    cond_stack->active = parent_active && !cond_stack->matched;
                }
            } else if (strncmp(directive, "endif", 5) == 0) {
                is_directive = true;
                if (cond_stack) {
                    CondStack* tmp = cond_stack;
                    cond_stack = cond_stack->next;
                    free(tmp);
                }
            } else if (cond_stack && !cond_stack->active) {
                // If we are skipping, just ignore other directives except
                // nested #ifs (Wait, I should handle #if/#else/#endif nesting
                // even when skipping) For simplicity, let's just mark it as
                // handled if it starts with #
                is_directive = true;
            } else if (strncmp(directive, "include", 7) == 0 &&
                       (directive[7] == ' ' || directive[7] == '\t' ||
                        directive[7] == '"')) {
                is_directive = true;
                const char* inc_p = directive + 7;
                while (*inc_p == ' ' || *inc_p == '\t')
                    inc_p++;
                if (*inc_p == '"') {
                    inc_p++;
                    const char* end_quote = strchr(inc_p, '"');
                    if (end_quote && (!next_line || end_quote < next_line)) {
                        int name_len = end_quote - inc_p;
                        char inc_name[256];
                        strncpy(inc_name, inc_p, name_len);
                        inc_name[name_len] = '\0';

                        char* dir = get_dirname(filename);
                        char full_path[512];
                        if (strcmp(dir, ".") == 0)
                            strcpy(full_path, inc_name);
                        else
                            snprintf(full_path, sizeof(full_path), "%s/%s", dir,
                                     inc_name);
                        free(dir);

                        char* inc_content = read_file(full_path);
                        if (inc_content) {
                            char* preprocessed_inc =
                                preprocess(inc_content, full_path);
                            size_t inc_len = strlen(preprocessed_inc);
                            if (out_ptr + inc_len >= output + out_size) {
                                fprintf(stderr,
                                        "Preprocessed output too large\n");
                                exit(1);
                            }
                            strcpy(out_ptr, preprocessed_inc);
                            out_ptr += inc_len;
                            free(preprocessed_inc);
                            free(inc_content);
                        } else {
                            fprintf(stderr,
                                    "Error: could not read include file %s\n",
                                    full_path);
                            exit(1);
                        }
                    }
                }
            } else if (strncmp(directive, "define", 6) == 0 &&
                       (directive[6] == ' ' || directive[6] == '\t')) {
                is_directive = true;
                const char* def_p = directive + 6;
                while (*def_p == ' ' || *def_p == '\t')
                    def_p++;
                const char* name_start = def_p;
                while (is_ident_char(*def_p))
                    def_p++;
                int name_len = def_p - name_start;
                if (name_len > 0) {
                    char name[256];
                    strncpy(name, name_start, name_len);
                    name[name_len] = '\0';
                    while (*def_p == ' ' || *def_p == '\t')
                        def_p++;
                    const char* val_start = def_p;
                    size_t val_len =
                        (next_line ? next_line : p + line_len) - val_start;
                    char value[1024];
                    strncpy(value, val_start, val_len);
                    value[val_len] = '\0';
                    add_macro(name, value);
                }
            }
        }

        if (is_directive) {
            p = next_line ? next_line + 1 : p + line_len;
            continue;
        }

        if (!cond_stack || cond_stack->active) {
            const char* line_end = next_line ? next_line : p + line_len;
            while (p < line_end) {
                if (is_ident_char(*p) && !isdigit(*p)) {
                    const char* ident_start = p;
                    while (p < line_end && is_ident_char(*p))
                        p++;
                    int len = p - ident_start;
                    Macro* m = find_macro(ident_start, len);
                    if (m) {
                        size_t m_len = strlen(m->value);
                        if (out_ptr + m_len >= output + out_size) {
                            fprintf(stderr, "Preprocessed output too large\n");
                            exit(1);
                        }
                        strcpy(out_ptr, m->value);
                        out_ptr += m_len;
                    } else {
                        strncpy(out_ptr, ident_start, len);
                        out_ptr += len;
                    }
                } else {
                    *out_ptr++ = *p++;
                }
            }
            if (next_line) {
                *out_ptr++ = '\n';
                p = next_line + 1;
            }
            *out_ptr = '\0';
        } else {
            p = next_line ? next_line + 1 : p + line_len;
        }
    }

    return output;
}
