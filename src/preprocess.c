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

typedef struct {
    Macro* macros;
} PreprocessContext;

typedef struct CondStack CondStack;
struct CondStack {
    bool matched;
    bool active;
    CondStack* next;
};

typedef struct {
    char* data;
    size_t len;
    size_t cap;
} StrBuf;

static char* dup_range(const char* begin, size_t len) {
    char* s = malloc(len + 1);
    if (!s) {
        perror("malloc");
        exit(1);
    }
    memcpy(s, begin, len);
    s[len] = '\0';
    return s;
}

static void sb_init(StrBuf* sb) {
    sb->cap = 4096;
    sb->len = 0;
    sb->data = malloc(sb->cap);
    if (!sb->data) {
        perror("malloc");
        exit(1);
    }
    sb->data[0] = '\0';
}

static void sb_ensure(StrBuf* sb, size_t add_len) {
    size_t needed = sb->len + add_len + 1;
    if (needed <= sb->cap)
        return;
    while (sb->cap < needed)
        sb->cap *= 2;
    char* p = realloc(sb->data, sb->cap);
    if (!p) {
        perror("realloc");
        exit(1);
    }
    sb->data = p;
}

static void sb_append_n(StrBuf* sb, const char* s, size_t n) {
    sb_ensure(sb, n);
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
}

static void sb_append_c(StrBuf* sb, char c) {
    sb_ensure(sb, 1);
    sb->data[sb->len++] = c;
    sb->data[sb->len] = '\0';
}

static bool is_ident_start(char c) {
    return (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_');
}

static bool is_ident_char(char c) {
    return is_ident_start(c) || ('0' <= c && c <= '9');
}

static const char* skip_spaces(const char* p, const char* end) {
    while (p < end && (*p == ' ' || *p == '\t'))
        p++;
    return p;
}

static Macro* find_macro(PreprocessContext* ctx, const char* name, size_t len) {
    for (Macro* m = ctx->macros; m; m = m->next) {
        if (strncmp(m->name, name, len) == 0 && m->name[len] == '\0')
            return m;
    }
    return NULL;
}

static void add_or_replace_macro(PreprocessContext* ctx, const char* name,
                                 const char* value) {
    size_t name_len = strlen(name);
    Macro* old = find_macro(ctx, name, name_len);
    if (old) {
        free(old->value);
        old->value = strdup(value);
        if (!old->value) {
            perror("strdup");
            exit(1);
        }
        return;
    }

    Macro* m = calloc(1, sizeof(Macro));
    if (!m) {
        perror("calloc");
        exit(1);
    }
    m->name = strdup(name);
    m->value = strdup(value);
    if (!m->name || !m->value) {
        perror("strdup");
        exit(1);
    }
    m->next = ctx->macros;
    ctx->macros = m;
}

static void free_macros(Macro* m) {
    while (m) {
        Macro* next = m->next;
        free(m->name);
        free(m->value);
        free(m);
        m = next;
    }
}

static char* get_dirname(const char* path) {
    const char* p = strrchr(path, '/');
    if (!p)
        return strdup(".");
    return dup_range(path, (size_t)(p - path));
}

static char* try_read_include(const char* dir, const char* inc_name) {
    StrBuf path;
    sb_init(&path);
    if (strcmp(dir, ".") == 0) {
        sb_append_n(&path, inc_name, strlen(inc_name));
    } else {
        sb_append_n(&path, dir, strlen(dir));
        sb_append_c(&path, '/');
        sb_append_n(&path, inc_name, strlen(inc_name));
    }

    char* content = read_file(path.data);
    free(path.data);
    return content;
}

static char* preprocess_internal(const char* input, const char* filename,
                                 PreprocessContext* ctx) {
    StrBuf out;
    sb_init(&out);

    CondStack* cond_stack = NULL;
    bool in_block_comment = false;

    const char* p = input;
    while (*p) {
        const char* line_start = p;
        const char* line_end = strchr(p, '\n');
        if (!line_end)
            line_end = p + strlen(p);

        const char* scan = line_start;
        while (scan < line_end && (*scan == ' ' || *scan == '\t'))
            scan++;

        bool is_directive = false;
        if (scan < line_end && *scan == '#') {
            const char* d = scan + 1;
            while (d < line_end && (*d == ' ' || *d == '\t'))
                d++;

            const char* kw_start = d;
            while (d < line_end && isalpha((unsigned char)*d))
                d++;
            size_t kw_len = (size_t)(d - kw_start);

            bool parent_active = (!cond_stack || cond_stack->active);

            const char* expr_begin = skip_spaces(d, line_end);
            int expr_val = 0;
            if (expr_begin < line_end) {
                const char* e = expr_begin;
                bool negate = false;
                if (*e == '!') {
                    negate = true;
                    e++;
                    e = skip_spaces(e, line_end);
                }
                if ((line_end - e) >= 7 && strncmp(e, "defined", 7) == 0) {
                    e += 7;
                    e = skip_spaces(e, line_end);
                    if (e < line_end && *e == '(')
                        e++;
                    e = skip_spaces(e, line_end);
                    const char* name_start = e;
                    while (e < line_end && is_ident_char(*e))
                        e++;
                    size_t name_len = (size_t)(e - name_start);
                    expr_val = find_macro(ctx, name_start, name_len) ? 1 : 0;
                } else if (is_ident_start(*e)) {
                    const char* name_start = e;
                    while (e < line_end && is_ident_char(*e))
                        e++;
                    size_t name_len = (size_t)(e - name_start);
                    Macro* m = find_macro(ctx, name_start, name_len);
                    if (m)
                        expr_val = (int)strtol(m->value, NULL, 10);
                    else
                        expr_val = 0;
                } else {
                    expr_val = (int)strtol(e, NULL, 10);
                }
                if (negate)
                    expr_val = !expr_val;
            }

            if (kw_len == 5 && strncmp(kw_start, "ifdef", 5) == 0) {
                is_directive = true;
                while (d < line_end && (*d == ' ' || *d == '\t'))
                    d++;
                const char* name_start = d;
                while (d < line_end && is_ident_char(*d))
                    d++;
                size_t name_len = (size_t)(d - name_start);
                bool found = find_macro(ctx, name_start, name_len) != NULL;

                CondStack* cs = calloc(1, sizeof(CondStack));
                cs->active = parent_active && found;
                cs->matched = found;
                cs->next = cond_stack;
                cond_stack = cs;
            } else if (kw_len == 6 && strncmp(kw_start, "ifndef", 6) == 0) {
                is_directive = true;
                while (d < line_end && (*d == ' ' || *d == '\t'))
                    d++;
                const char* name_start = d;
                while (d < line_end && is_ident_char(*d))
                    d++;
                size_t name_len = (size_t)(d - name_start);
                bool found = find_macro(ctx, name_start, name_len) != NULL;

                CondStack* cs = calloc(1, sizeof(CondStack));
                cs->active = parent_active && !found;
                cs->matched = !found;
                cs->next = cond_stack;
                cond_stack = cs;
            } else if (kw_len == 2 && strncmp(kw_start, "if", 2) == 0) {
                is_directive = true;
                CondStack* cs = calloc(1, sizeof(CondStack));
                cs->active = parent_active && (expr_val != 0);
                cs->matched = (expr_val != 0);
                cs->next = cond_stack;
                cond_stack = cs;
            } else if (kw_len == 4 && strncmp(kw_start, "elif", 4) == 0) {
                is_directive = true;
                if (cond_stack) {
                    bool grand_active =
                        (!cond_stack->next || cond_stack->next->active);
                    bool take = (!cond_stack->matched) && (expr_val != 0);
                    cond_stack->active = grand_active && take;
                    if (take)
                        cond_stack->matched = true;
                }
            } else if (kw_len == 4 && strncmp(kw_start, "else", 4) == 0) {
                is_directive = true;
                if (cond_stack) {
                    bool grand_active =
                        (!cond_stack->next || cond_stack->next->active);
                    cond_stack->active = grand_active && !cond_stack->matched;
                    cond_stack->matched = true;
                }
            } else if (kw_len == 5 && strncmp(kw_start, "endif", 5) == 0) {
                is_directive = true;
                if (cond_stack) {
                    CondStack* tmp = cond_stack;
                    cond_stack = cond_stack->next;
                    free(tmp);
                }
            } else if (!parent_active) {
                is_directive = true;
            } else if (kw_len == 7 && strncmp(kw_start, "include", 7) == 0) {
                is_directive = true;
                while (d < line_end && (*d == ' ' || *d == '\t'))
                    d++;

                if (d < line_end && (*d == '"' || *d == '<')) {
                    char closing = (*d == '"') ? '"' : '>';
                    d++;
                    const char* inc_start = d;
                    while (d < line_end && *d != closing)
                        d++;
                    if (d < line_end && *d == closing) {
                        char* inc_name =
                            dup_range(inc_start, (size_t)(d - inc_start));
                        char* inc_content = NULL;

                        if (closing == '"') {
                            char* dir = get_dirname(filename);
                            inc_content = try_read_include(dir, inc_name);
                            free(dir);
                        }
                        if (!inc_content) {
                            inc_content =
                                try_read_include("selfhost/include", inc_name);
                        }
                        if (!inc_content) {
                            inc_content = try_read_include(".", inc_name);
                        }
                        if (!inc_content) {
                            fprintf(stderr,
                                    "Error: could not read include file %s\n",
                                    inc_name);
                            free(inc_name);
                            exit(1);
                        }

                        char* expanded =
                            preprocess_internal(inc_content, filename, ctx);
                        sb_append_n(&out, expanded, strlen(expanded));
                        free(expanded);
                        free(inc_content);
                        free(inc_name);
                    }
                }
            } else if (kw_len == 6 && strncmp(kw_start, "define", 6) == 0) {
                is_directive = true;
                while (d < line_end && (*d == ' ' || *d == '\t'))
                    d++;
                const char* name_start = d;
                if (d < line_end && is_ident_start(*d)) {
                    while (d < line_end && is_ident_char(*d))
                        d++;
                    char* name =
                        dup_range(name_start, (size_t)(d - name_start));
                    while (d < line_end && (*d == ' ' || *d == '\t'))
                        d++;
                    char* value = dup_range(d, (size_t)(line_end - d));
                    add_or_replace_macro(ctx, name, value);
                    free(name);
                    free(value);
                }
            }
        }

        if (!is_directive && (!cond_stack || cond_stack->active)) {
            const char* s = line_start;
            while (s < line_end) {
                if (in_block_comment) {
                    sb_append_c(&out, *s);
                    if (*s == '*' && (s + 1) < line_end && s[1] == '/') {
                        sb_append_c(&out, '/');
                        s += 2;
                        in_block_comment = false;
                    } else {
                        s++;
                    }
                    continue;
                }

                if (*s == '/' && (s + 1) < line_end && s[1] == '*') {
                    sb_append_c(&out, '/');
                    sb_append_c(&out, '*');
                    s += 2;
                    in_block_comment = true;
                    continue;
                }
                if (*s == '/' && (s + 1) < line_end && s[1] == '/') {
                    sb_append_n(&out, s, (size_t)(line_end - s));
                    s = line_end;
                    continue;
                }
                if (*s == '"' || *s == '\'') {
                    char quote = *s;
                    sb_append_c(&out, *s++);
                    bool escaped = false;
                    while (s < line_end) {
                        sb_append_c(&out, *s);
                        if (escaped) {
                            escaped = false;
                        } else if (*s == '\\') {
                            escaped = true;
                        } else if (*s == quote) {
                            s++;
                            break;
                        }
                        s++;
                    }
                    continue;
                }

                if (is_ident_start(*s)) {
                    const char* id_start = s;
                    while (s < line_end && is_ident_char(*s))
                        s++;
                    size_t id_len = (size_t)(s - id_start);
                    Macro* m = find_macro(ctx, id_start, id_len);
                    if (m) {
                        sb_append_n(&out, m->value, strlen(m->value));
                    } else {
                        sb_append_n(&out, id_start, id_len);
                    }
                    continue;
                }

                sb_append_c(&out, *s);
                s++;
            }
        }

        if (*line_end == '\n') {
            if (!is_directive && (!cond_stack || cond_stack->active))
                sb_append_c(&out, '\n');
            p = line_end + 1;
        } else {
            p = line_end;
        }
    }

    while (cond_stack) {
        CondStack* next = cond_stack->next;
        free(cond_stack);
        cond_stack = next;
    }

    return out.data;
}

char* preprocess(const char* input, const char* filename) {
    PreprocessContext ctx = {0};
    add_or_replace_macro(&ctx, "__clang__", "1");
#ifdef __APPLE__
    add_or_replace_macro(&ctx, "__APPLE__", "1");
#endif
    char* out = preprocess_internal(input, filename, &ctx);
    free_macros(ctx.macros);
    return out;
}
