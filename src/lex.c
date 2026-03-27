#include "lex.h"
#include "parse.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LLVM7_INT_MAX 2147483647

static int is_alnum(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') || c == '_';
}

static char decode_escape_char(const char** pp) {
    const char* p = *pp;
    if (*p == 'n') {
        *pp = p + 1;
        return '\n';
    }
    if (*p == 'r') {
        *pp = p + 1;
        return '\r';
    }
    if (*p == 't') {
        *pp = p + 1;
        return '\t';
    }
    if (*p == '0') {
        *pp = p + 1;
        return '\0';
    }
    if (*p == '\\' || *p == '"' || *p == '\'') {
        *pp = p + 1;
        return *p;
    }
    if (*p == '\0') {
        return '\0';
    }
    *pp = p + 1;
    return *p;
}

// Keyword table (selfhost-compatible: no anonymous struct)
char* kw_str[41] = {
    "return",   "if",       "else",     "while",    "for",      "int",
    "char",     "void",     "sizeof",   "struct",   "typedef",  "enum",
    "static",   "extern",   "const",    "long",     "bool",     "size_t",
    "NULL",     "true",     "false",    "switch",   "case",     "default",
    "break",    "continue", "unsigned", "signed",   "double",   "float",
    "do",       "goto",     "union",    "inline",   "restrict", "volatile",
    "register", "_Bool",    "_Complex", "__func__", "_Pragma"};
int kw_len[41] = {6, 2, 4, 5, 3, 3, 4, 4, 6, 6, 7, 4, 6, 6, 5, 4, 4, 6, 4, 4, 5,
                  6, 4, 7, 5, 8, 8, 6, 6, 5, 2, 4, 5, 6, 8, 8, 8, 5, 8, 8, 7};
int NUM_KEYWORDS = 41;

char* three_char_ops[3] = {"...", "<<=", ">>="};
int NUM_THREE_CHAR_OPS = 3;

static char* two_char_ops[] = {
    "==", "!=", "<=", ">=", "&&", "||", "->", "++", "--",
    "+=", "-=", "*=", "/=", "<<", ">>", "&=", "|=", "^="};

static int NUM_TWO_CHAR_OPS = sizeof(two_char_ops) / sizeof(two_char_ops[0]);

static bool lex_debug = false;

void lex_get_line_col(const char* source, const char* pos, int* line,
                      int* col) {
    int ln = 1;
    int cl = 1;
    for (const char* p = source; p < pos && *p; p++) {
        if (*p == '\n') {
            ln++;
            cl = 1;
        } else {
            cl++;
        }
    }
    *line = ln;
    *col = cl;
}

static void print_token(TokenKind kind, const char* str, int len, int val) {
    const char* kind_name = "UNKNOWN";
    switch (kind) {
    case TK_RESERVED:
        kind_name = "RESERVED";
        break;
    case TK_IDENT:
        kind_name = "IDENT";
        break;
    case TK_NUM:
        kind_name = "NUM";
        break;
    case TK_STR:
        kind_name = "STR";
        break;
    case TK_EOF:
        kind_name = "EOF";
        break;
    }
    fprintf(stderr, "token: kind=%s len=%d str='%.20s' val=%d\n", kind_name,
            len, str, val);
}

Token* new_token(TokenKind kind, Token* cur, const char* str, int len) {
    Token* tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->val = 0;
    tok->uval = 0;
    tok->fval = 0;
    tok->str = str;
    tok->len = len;
    tok->line = 0;
    tok->col = 0;
    cur->next = tok;
    if (lex_debug) {
        print_token(kind, str, len, tok->val);
    }
    return tok;
}

static Token* new_token_at(TokenKind kind, Token* cur, const char* str, int len,
                           const char* source, const char* pos) {
    Token* tok = new_token(kind, cur, str, len);
    lex_get_line_col(source, pos, &tok->line, &tok->col);
    return tok;
}

/**
 * Tokenize a string into a linked list of tokens
 *
 * @param[in] p The input string to be tokenized
 * @return Pointer to the head of the token linked list
 */
Token* tokenize(const char* p) {
    const char* source = p;
    // Initialize head and tail pointers for the token linked list
    Token head;
    head.next = NULL;
    Token* cur = &head;
    lex_debug = getenv("DEBUG_TOKENS") != NULL;

    // Iterate through the input string until null terminator
    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Skip line comments
        if (strncmp(p, "//", 2) == 0) {
            p += 2;
            while (*p && *p != '\n')
                p++;
            continue;
        }

        // Skip block comments
        if (strncmp(p, "/*", 2) == 0) {
            const char* q = strstr(p + 2, "*/");
            if (!q) {
                int line = 0;
                int col = 0;
                lex_get_line_col(source, p, &line, &col);
                fprintf(stderr, "lex error:%d:%d: unterminated block comment\n",
                        line, col);
                return NULL;
            }
            p = q + 2;
            continue;
        }

        // Check for keywords
        bool keyword_matched = false;
        for (int i = 0; i < NUM_KEYWORDS; i++) {
            if (strncmp(p, kw_str[i], kw_len[i]) == 0 &&
                !is_alnum(p[kw_len[i]])) {
                cur = new_token_at(TK_RESERVED, cur, p, kw_len[i], source, p);
                p += kw_len[i];
                keyword_matched = true;
                break;
            }
        }
        if (keyword_matched) {
            continue;
        }

        // Check for three-character operators (ellipsis)
        bool three_char_matched = false;
        for (int i = 0; i < NUM_THREE_CHAR_OPS; i++) {
            if (strncmp(p, three_char_ops[i], 3) == 0) {
                cur = new_token_at(TK_RESERVED, cur, p, 3, source, p);
                p += 3;
                three_char_matched = true;
                break;
            }
        }
        if (three_char_matched) {
            continue;
        }

        // Check for two-character operators
        bool two_char_matched = false;
        for (int i = 0; i < NUM_TWO_CHAR_OPS; i++) {
            if (strncmp(p, two_char_ops[i], 2) == 0) {
                cur = new_token_at(TK_RESERVED, cur, p, 2, source, p);
                p += 2;
                two_char_matched = true;
                break;
            }
        }
        if (two_char_matched) {
            continue;
        }

        // Check for single-character operators and delimiters
        char* single_char_ops = "+-*/()<>;={},&|[].!:=?%^~\0";
        if (strchr(single_char_ops, *p)) {
            cur = new_token_at(TK_RESERVED, cur, p, 1, source, p);
            p++;
            continue;
        }

        // String literal
        if (*p == '"') {
            p++; // skip opening quote
            size_t cap = strlen(p) + 1;
            char* decoded = calloc(cap, 1);
            size_t len = 0;
            while (*p && *p != '"') {
                if (*p == '\\') {
                    p++;
                    decoded[len++] = decode_escape_char(&p);
                    continue;
                }
                decoded[len++] = *p;
                p++;
            }
            if (*p != '"') {
                int line = 0;
                int col = 0;
                lex_get_line_col(source, p, &line, &col);
                fprintf(stderr,
                        "lex error:%d:%d: unterminated string literal\n", line,
                        col);
                return NULL;
            }
            cur = new_token_at(TK_STR, cur, decoded, (int)len, source, p);
            p++; // skip closing quote
            continue;
        }

        // Character literal
        if (*p == '\'') {
            p++; // skip '
            int val;
            if (*p == '\\') {
                p++;
                val = decode_escape_char(&p);
            } else {
                val = *p++;
            }

            if (*p != '\'') {
                int line = 0;
                int col = 0;
                lex_get_line_col(source, p, &line, &col);
                fprintf(stderr,
                        "lex error:%d:%d: unterminated character literal\n",
                        line, col);
                return NULL;
            }
            p++; // skip closing '
            cur = new_token_at(TK_NUM, cur, p - 2, 1, source, p - 2);
            cur->val = val;
            cur->uval = (unsigned long long)(unsigned int)val;
            cur->len =
                0; // Still use 0 to avoid consume() matching, but wait...
            // Let's use actual length 1 and pointed at the char but it's
            // complex. Actually, the best way is to not use len=0 if it's a
            // valid token. For TK_NUM, expect_number() uses cur->val. So
            // kind=TK_NUM, len=0 is fine IF nobody uses len to match it. Wait,
            // the previous logic was creating a SECOND token!
            continue;
        }

        // Hexadecimal floating-point or integer constant (C99)
        if (strncmp(p, "0x", 2) == 0 || strncmp(p, "0X", 2) == 0) {
            const char* start = p;
            p += 2;

            bool has_dot = false;
            while (isxdigit(*p))
                p++;
            if (*p == '.') {
                has_dot = true;
                p++;
                while (isxdigit(*p))
                    p++;
            }

            // Hex float must have exponent 'p' or 'P'
            if (*p == 'p' || *p == 'P') {
                p++;
                if (*p == '+' || *p == '-')
                    p++;
                while (isdigit(*p))
                    p++;

                // Optional suffix
                if (*p == 'l' || *p == 'L' || *p == 'f' || *p == 'F') {
                    p++;
                }

                cur =
                    new_token_at(TK_NUM, cur, start, p - start, source, start);
                cur->is_float = true;
                cur->fval = strtod(start, NULL);
                continue;
            }

            // Not a hex float. If it had a dot, we backtrack to before the dot
            // to treat it as a hex integer followed by a dot.
            if (has_dot) {
                p = start + 2;
                while (isxdigit(*p))
                    p++;
            }

            // Hex integer
            cur = new_token_at(TK_NUM, cur, start, p - start, source, start);
            cur->is_float = false;
            cur->uval = strtoull(start, NULL, 16);
            cur->fval = (double)cur->uval;
            if (cur->uval <= (unsigned long long)LLVM7_INT_MAX) {
                cur->val = (int)cur->uval;
            } else {
                cur->val = LLVM7_INT_MAX;
            }

            // Optional integer suffixes
            while (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L') {
                if (*p == 'u' || *p == 'U')
                    cur->is_unsigned = true;
                p++;
            }
            cur->len = (int)(p - start);
            continue;
        }

        // Number
        if (isdigit(*p) || (*p == '.' && isdigit(p[1]))) {
            const char* start = p;
            bool is_float = false;
            if (*p == '.') {
                is_float = true;
                p++;
                while (isdigit(*p)) {
                    p++;
                }
            } else {
                while (isdigit(*p)) {
                    p++;
                }
                if (*p == '.') {
                    is_float = true;
                    p++;
                    while (isdigit(*p)) {
                        p++;
                    }
                }
            }
            if (*p == 'e' || *p == 'E') {
                is_float = true;
                p++;
                if (*p == '+' || *p == '-') {
                    p++;
                }
                while (isdigit(*p)) {
                    p++;
                }
            }
            if (*p == 'f' || *p == 'F') {
                is_float = true;
                p++;
            }
            cur = new_token_at(TK_NUM, cur, start, p - start, source, start);
            cur->is_float = is_float;
            if (is_float) {
                cur->fval = strtod(start, NULL);
            } else {
                cur->uval = strtoull(start, NULL, 10);
                cur->fval = (double)cur->uval;
                if (cur->uval <= (unsigned long long)LLVM7_INT_MAX) {
                    cur->val = (int)cur->uval;
                } else {
                    cur->val = LLVM7_INT_MAX;
                }
            }
            if (!is_float && (*p == 'u' || *p == 'U')) {
                cur->is_unsigned = true;
                p++;
            }
            if (!is_float) {
                while (*p == 'l' || *p == 'L') {
                    p++;
                }
            }
            cur->len = (int)(p - start);
            continue;
        }

        // Identifier
        if (('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z') || *p == '_') {
            const char* start = p;
            while (('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z') ||
                   ('0' <= *p && *p <= '9') || *p == '_') {
                p++;
            }
            cur = new_token_at(TK_IDENT, cur, start, p - start, source, start);
            continue;
        }

        // Error
        int line = 0;
        int col = 0;
        lex_get_line_col(source, p, &line, &col);
        fprintf(stderr, "lex error:%d:%d: invalid character '%c'\n", line, col,
                *p);
        return NULL;
    }

    new_token_at(TK_EOF, cur, p, 0, source, p);
    return head.next;
}

/**
 * Free a linked list of tokens
 *
 * @param[in] head Pointer to the head of the token linked list to free
 */
void free_tokens(Token* head) {
    Token* curr = head;
    while (curr != NULL) {
        Token* next = curr->next;
        free(curr);
        curr = next;
    }
}

/**
 * Function to consume a reserved operator token and proceed to the next token
 *
 * @param[in] ctx Context containing current token
 * @param[in] op The character operator to be consumed
 * @return true if the token was consumed successfully, false otherwise
 */
bool consume(Context* ctx, char* op) {
    if (ctx->current_token->kind != TK_RESERVED ||
        (int)strlen(op) != ctx->current_token->len ||
        memcmp(ctx->current_token->str, op, ctx->current_token->len)) {
        return false;
    }
    ctx->current_token = ctx->current_token->next;
    return true;
}

/**
 * Function to consume and return an identifier token
 *
 * @param[in] ctx Context containing current token
 * @return Pointer to the identifier token if the current token is an
 * identifier, NULL otherwise
 */
Token* consume_ident(Context* ctx) {
    // Check if the current token is an identifier
    if (ctx->current_token->kind != TK_IDENT) {
        return NULL;
    }
    // Store current token and advance to the next token
    Token* t = ctx->current_token;
    ctx->current_token = ctx->current_token->next;
    return t;
}

/**
 * Checks if the next token matches the expected operator
 * If it doesn't match, prints an error message and exits the program
 * @param[in] ctx Context containing current token
 * @param[in] op The expected operator string
 */
void expect(Context* ctx, char* op) {
    // Try to consume (match) the expected operator
    if (!consume(ctx, op)) {
        fprintf(stderr, "lex error:%d:%d: expected '%s'\n",
                ctx->current_token->line, ctx->current_token->col, op);
        // Context: fprintf(stderr, "Context: '%.20s'\n",
        // ctx->current_token->str); Exit the program with an error status
        exit(1);
    }
}

int expect_number(Context* ctx) {
    // Check if the current token is a number
    if (ctx->current_token->kind != TK_NUM) {
        fprintf(stderr, "lex error:%d:%d: expected a number\n",
                ctx->current_token->line, ctx->current_token->col);
        // Exit the program with an error status
        exit(1);
    }
    if (ctx->current_token->uval > (unsigned long long)LLVM7_INT_MAX) {
        fprintf(stderr,
                "lex error:%d:%d: integer literal out of range for int\n",
                ctx->current_token->line, ctx->current_token->col);
        exit(1);
    }
    // Return the value of the number token
    int val = ctx->current_token->val;
    ctx->current_token = ctx->current_token->next;
    return val;
}

bool at_eof(Context* ctx) { return ctx->current_token->kind == TK_EOF; }
