#include "preprocess.h"
#include "file.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct Macro Macro;
struct Macro {
    char* name;
    char* value;
    bool is_function;
    bool is_variadic;
    char** params;
    int param_count;
    Macro* next;
};

typedef struct {
    Macro* macros;
    int current_line;
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

static void free_macro_fields(Macro* m) {
    free(m->name);
    free(m->value);
    if (m->params) {
        for (int i = 0; i < m->param_count; i++) {
            free(m->params[i]);
        }
        free(m->params);
    }
}

static void add_or_replace_macro(PreprocessContext* ctx, const char* name,
                                 const char* value, bool is_function,
                                 bool is_variadic, char** params,
                                 int param_count) {
    size_t name_len = strlen(name);
    Macro* old = find_macro(ctx, name, name_len);
    if (old) {
        free(old->value);
        if (old->params) {
            for (int i = 0; i < old->param_count; i++)
                free(old->params[i]);
            free(old->params);
        }
        old->value = strdup(value);
        if (!old->value) {
            perror("strdup");
            exit(1);
        }
        old->is_function = is_function;
        old->is_variadic = is_variadic;
        old->params = params;
        old->param_count = param_count;
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
    m->is_function = is_function;
    m->is_variadic = is_variadic;
    m->params = params;
    m->param_count = param_count;
    m->next = ctx->macros;
    ctx->macros = m;
}

static void undef_macro(PreprocessContext* ctx, const char* name, size_t len) {
    Macro** pp = &ctx->macros;
    while (*pp) {
        Macro* m = *pp;
        if (strncmp(m->name, name, len) == 0 && m->name[len] == '\0') {
            *pp = m->next;
            free_macro_fields(m);
            free(m);
            return;
        }
        pp = &m->next;
    }
}

static void free_macros(Macro* m) {
    while (m) {
        Macro* next = m->next;
        free_macro_fields(m);
        free(m);
        m = next;
    }
}

static char* get_dirname(const char* path) {
    size_t slash_pos = 0;
    bool found = false;
    for (size_t i = 0; path[i] != '\0'; i++) {
        if (path[i] == '/') {
            slash_pos = i;
            found = true;
        }
    }
    if (!found)
        return strdup(".");
    return dup_range(path, slash_pos);
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

static char* trim_copy(const char* s, size_t len) {
    const char* b = s;
    const char* e = s + len;
    while (b < e && (*b == ' ' || *b == '\t'))
        b++;
    while (e > b && (e[-1] == ' ' || e[-1] == '\t'))
        e--;
    return dup_range(b, (size_t)(e - b));
}

static int find_param_index(Macro* m, const char* name, size_t len) {
    for (int i = 0; i < m->param_count; i++) {
        if (strlen(m->params[i]) == len &&
            strncmp(m->params[i], name, len) == 0) {
            return i;
        }
    }
    return -1;
}

static char* expand_macros_in_text(PreprocessContext* ctx, const char* text,
                                   int depth);

static void sb_append_escaped_quoted(StrBuf* out, const char* s) {
    sb_append_c(out, '"');
    for (const char* p = s; *p; p++) {
        if (*p == '\\' || *p == '"')
            sb_append_c(out, '\\');
        sb_append_c(out, *p);
    }
    sb_append_c(out, '"');
}

static void trim_trailing_space(StrBuf* out) {
    while (out->len > 0) {
        char c = out->data[out->len - 1];
        if (c == ' ' || c == '\t') {
            out->len--;
            out->data[out->len] = '\0';
            continue;
        }
        break;
    }
}

static char* expand_function_macro(Macro* m, char** args, int argc) {
    StrBuf out;
    sb_init(&out);
    const char* p = m->value;
    while (*p) {
        if (*p == '#' && p[1] == '#') {
            // Token pasting: remove previous trailing spaces and suppress
            // following spaces.
            trim_trailing_space(&out);
            p += 2;
            while (*p == ' ' || *p == '\t')
                p++;
            continue;
        }
        if (*p == '#') {
            const char* q = p + 1;
            while (*q == ' ' || *q == '\t')
                q++;
            if (is_ident_start(*q)) {
                const char* start = q;
                while (is_ident_char(*q))
                    q++;
                size_t len = (size_t)(q - start);
                int idx = find_param_index(m, start, len);
                if (idx >= 0 && idx < argc) {
                    sb_append_escaped_quoted(&out, args[idx]);
                    p = q;
                    continue;
                }
                // #__VA_ARGS__ stringification
                if (m->is_variadic && len == 11 &&
                    strncmp(start, "__VA_ARGS__", 11) == 0) {
                    StrBuf joined;
                    sb_init(&joined);
                    int fixed_count = m->param_count;
                    for (int i = fixed_count; i < argc; i++) {
                        if (i > fixed_count)
                            sb_append_c(&joined, ',');
                        sb_append_n(&joined, args[i], strlen(args[i]));
                    }
                    sb_append_escaped_quoted(&out, joined.data);
                    free(joined.data);
                    p = q;
                    continue;
                }
            }
        }
        // , ##__VA_ARGS__ GCC extension: suppress preceding comma when empty
        if (*p == ',' && m->is_variadic) {
            const char* q = p + 1;
            while (*q == ' ' || *q == '\t')
                q++;
            if (q[0] == '#' && q[1] == '#') {
                const char* r = q + 2;
                while (*r == ' ' || *r == '\t')
                    r++;
                if (strncmp(r, "__VA_ARGS__", 11) == 0 &&
                    !is_ident_char(r[11])) {
                    if (argc <= m->param_count) {
                        // No variadic args: suppress comma
                        p = r + 11;
                        continue;
                    }
                    // Has variadic args: emit ", args..."
                    sb_append_c(&out, ',');
                    int fixed_count = m->param_count;
                    for (int i = fixed_count; i < argc; i++) {
                        if (i > fixed_count)
                            sb_append_c(&out, ',');
                        sb_append_n(&out, args[i], strlen(args[i]));
                    }
                    p = r + 11;
                    continue;
                }
            }
        }
        if (is_ident_start(*p)) {
            const char* start = p;
            while (is_ident_char(*p))
                p++;
            size_t len = (size_t)(p - start);
            // Check for __VA_ARGS__
            if (m->is_variadic && len == 11 &&
                strncmp(start, "__VA_ARGS__", 11) == 0) {
                // Append variadic args (all args after fixed params)
                int fixed_count = m->param_count;
                for (int i = fixed_count; i < argc; i++) {
                    if (i > fixed_count)
                        sb_append_c(&out, ',');
                    sb_append_n(&out, args[i], strlen(args[i]));
                }
                continue;
            }
            int idx = find_param_index(m, start, len);
            if (idx >= 0 && idx < argc) {
                sb_append_n(&out, args[idx], strlen(args[idx]));
            } else {
                sb_append_n(&out, start, len);
            }
            continue;
        }
        sb_append_c(&out, *p++);
    }
    return out.data;
}

static char* expand_macros_in_text(PreprocessContext* ctx, const char* text,
                                   int depth) {
    if (depth > 16) {
        return strdup(text);
    }

    StrBuf out;
    sb_init(&out);
    const char* p = text;
    while (*p) {
        if (*p == '"' || *p == '\'') {
            char q = *p;
            sb_append_c(&out, *p++);
            bool escaped = false;
            while (*p) {
                sb_append_c(&out, *p);
                if (escaped) {
                    escaped = false;
                } else if (*p == '\\') {
                    escaped = true;
                } else if (*p == q) {
                    p++;
                    break;
                }
                p++;
            }
            continue;
        }

        if (is_ident_start(*p)) {
            const char* id_start = p;
            while (is_ident_char(*p))
                p++;
            size_t id_len = (size_t)(p - id_start);
            if (id_len == 8 && strncmp(id_start, "__LINE__", 8) == 0) {
                char line_buf[32];
                snprintf(line_buf, sizeof(line_buf), "%d", ctx->current_line);
                sb_append_n(&out, line_buf, strlen(line_buf));
                continue;
            }
            Macro* m = find_macro(ctx, id_start, id_len);
            if (m && !m->is_function) {
                char* ex = expand_macros_in_text(ctx, m->value, depth + 1);
                sb_append_n(&out, ex, strlen(ex));
                free(ex);
            } else {
                sb_append_n(&out, id_start, id_len);
            }
            continue;
        }

        sb_append_c(&out, *p++);
    }
    return out.data;
}

static char** parse_macro_args(const char** sp, const char* end,
                               int* out_argc) {
    const char* p = *sp;
    if (p >= end || *p != '(') {
        *out_argc = 0;
        return NULL;
    }
    p++; // skip '('

    int cap = 4;
    int argc = 0;
    char** args = malloc(sizeof(char*) * cap);
    if (!args) {
        perror("malloc");
        exit(1);
    }

    while (1) {
        p = skip_spaces(p, end);
        if (p >= end) {
            break;
        }
        if (*p == ')') {
            p++;
            break;
        }

        const char* arg_start = p;
        int depth = 0;
        while (p < end) {
            if (*p == '(') {
                depth++;
                p++;
                continue;
            }
            if (*p == ')') {
                if (depth == 0)
                    break;
                depth--;
                p++;
                continue;
            }
            if (*p == ',' && depth == 0)
                break;
            p++;
        }

        if (argc == cap) {
            cap *= 2;
            char** np = realloc(args, sizeof(char*) * cap);
            if (!np) {
                perror("realloc");
                exit(1);
            }
            args = np;
        }
        args[argc++] = trim_copy(arg_start, (size_t)(p - arg_start));

        p = skip_spaces(p, end);
        if (p < end && *p == ',') {
            p++;
            continue;
        }
        if (p < end && *p == ')') {
            p++;
            break;
        }
    }

    *sp = p;
    *out_argc = argc;
    return args;
}

static char* preprocess_internal(const char* input, const char* filename,
                                 PreprocessContext* ctx) {
    StrBuf out;
    sb_init(&out);

    ctx->current_line = 1;

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
                    if (name_len == 8 &&
                        strncmp(name_start, "__LINE__", 8) == 0) {
                        expr_val = ctx->current_line;
                    } else {
                        Macro* m = find_macro(ctx, name_start, name_len);
                        if (m)
                            expr_val = (int)strtol(m->value, NULL, 10);
                        else
                            expr_val = 0;
                    }
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
                        size_t inc_len = 0;
                        const char* t = inc_start;
                        while (t < d) {
                            inc_len++;
                            t++;
                        }
                        char* inc_name = dup_range(inc_start, inc_len);
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
                    bool is_function = false;
                    bool is_variadic = false;
                    char** params = NULL;
                    int param_count = 0;

                    // Function-like macro: no whitespace between name and '('
                    if (d < line_end && *d == '(') {
                        is_function = true;
                        d++;
                        int pcap = 4;
                        params = malloc(sizeof(char*) * pcap);
                        if (!params) {
                            perror("malloc");
                            exit(1);
                        }
                        while (1) {
                            d = skip_spaces(d, line_end);
                            if (d < line_end && *d == ')') {
                                d++;
                                break;
                            }
                            // Check for ... (variadic)
                            if ((line_end - d) >= 3 && d[0] == '.' &&
                                d[1] == '.' && d[2] == '.') {
                                is_variadic = true;
                                d += 3;
                                d = skip_spaces(d, line_end);
                                if (d < line_end && *d == ',') {
                                    d++;
                                    continue;
                                }
                                if (d < line_end && *d == ')') {
                                    d++;
                                    break;
                                }
                                fprintf(stderr,
                                        "variadic macro ... must be last\n");
                                exit(1);
                            }
                            const char* pstart = d;
                            if (!(d < line_end && is_ident_start(*d))) {
                                fprintf(stderr, "invalid macro parameter\n");
                                exit(1);
                            }
                            while (d < line_end && is_ident_char(*d))
                                d++;
                            if (param_count == pcap) {
                                pcap *= 2;
                                char** np =
                                    realloc(params, sizeof(char*) * pcap);
                                if (!np) {
                                    perror("realloc");
                                    exit(1);
                                }
                                params = np;
                            }
                            params[param_count++] =
                                dup_range(pstart, (size_t)(d - pstart));
                            d = skip_spaces(d, line_end);
                            if (d < line_end && *d == ',') {
                                d++;
                                continue;
                            }
                            if (d < line_end && *d == ')') {
                                d++;
                                break;
                            }
                            fprintf(stderr, "invalid macro parameter list\n");
                            exit(1);
                        }
                    } else {
                        while (d < line_end && (*d == ' ' || *d == '\t'))
                            d++;
                    }

                    char* value = trim_copy(d, (size_t)(line_end - d));
                    add_or_replace_macro(ctx, name, value, is_function,
                                         is_variadic, params, param_count);
                    free(name);
                    free(value);
                }
            } else if (kw_len == 5 && strncmp(kw_start, "undef", 5) == 0) {
                is_directive = true;
                while (d < line_end && (*d == ' ' || *d == '\t'))
                    d++;
                const char* name_start = d;
                while (d < line_end && is_ident_char(*d))
                    d++;
                if (d > name_start)
                    undef_macro(ctx, name_start, (size_t)(d - name_start));
            } else if (kw_len == 6 && strncmp(kw_start, "pragma", 6) == 0) {
                // Ignore pragmas for now.
                is_directive = true;
            } else if (kw_len == 5 && strncmp(kw_start, "error", 5) == 0) {
                is_directive = true;
                const char* msg = skip_spaces(d, line_end);
                fprintf(stderr, "#error: %.*s\n", (int)(line_end - msg), msg);
                exit(1);
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
                    if (id_len == 8 && strncmp(id_start, "__LINE__", 8) == 0) {
                        char line_buf[32];
                        snprintf(line_buf, sizeof(line_buf), "%d",
                                 ctx->current_line);
                        sb_append_n(&out, line_buf, strlen(line_buf));
                        continue;
                    }
                    Macro* m = find_macro(ctx, id_start, id_len);
                    if (m) {
                        if (m->is_function) {
                            const char* call = s;
                            call = skip_spaces(call, line_end);
                            if (call < line_end && *call == '(') {
                                int argc = 0;
                                char** args =
                                    parse_macro_args(&call, line_end, &argc);
                                // For variadic macros, accept argc >=
                                // param_count For non-variadic, require argc ==
                                // param_count
                                bool arg_count_ok =
                                    m->is_variadic ? (argc >= m->param_count)
                                                   : (argc == m->param_count);
                                if (arg_count_ok) {
                                    char* ex =
                                        expand_function_macro(m, args, argc);
                                    char* re =
                                        expand_macros_in_text(ctx, ex, 0);
                                    sb_append_n(&out, re, strlen(re));
                                    free(re);
                                    free(ex);
                                    s = call;
                                } else {
                                    sb_append_n(&out, id_start, id_len);
                                }
                                for (int i = 0; i < argc; i++)
                                    free(args[i]);
                                free(args);
                            } else {
                                sb_append_n(&out, id_start, id_len);
                            }
                        } else {
                            char* ex = expand_macros_in_text(ctx, m->value, 0);
                            sb_append_n(&out, ex, strlen(ex));
                            free(ex);
                        }
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
            ctx->current_line++;
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
    add_or_replace_macro(&ctx, "__clang__", "1", false, false, NULL, 0);
#ifdef __APPLE__
    add_or_replace_macro(&ctx, "__APPLE__", "1", false, false, NULL, 0);
#endif
    add_or_replace_macro(&ctx, "__STDC_VERSION__", "199901L", false, false,
                         NULL, 0);

    StrBuf file_val;
    sb_init(&file_val);
    sb_append_escaped_quoted(&file_val, filename);
    add_or_replace_macro(&ctx, "__FILE__", file_val.data, false, false, NULL,
                         0);
    free(file_val.data);

    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char date_buf[32];
    strftime(date_buf, sizeof(date_buf), "\"%b %d %Y\"", tm_info);
    add_or_replace_macro(&ctx, "__DATE__", date_buf, false, false, NULL, 0);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "\"%H:%M:%S\"", tm_info);
    add_or_replace_macro(&ctx, "__TIME__", time_buf, false, false, NULL, 0);

    char* out = preprocess_internal(input, filename, &ctx);
    free_macros(ctx.macros);
    return out;
}
