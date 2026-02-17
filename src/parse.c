#include "parse.h"
#include "lex.h"
#include "variable.h"
#include <stdlib.h>
#include <string.h>

static Node* parse_logor(Context* ctx);
static Node* parse_logand(Context* ctx);
static Node* convert_array_to_ptr(Node* node);
static Node* parse_unary_no_array_conv(Context* ctx);
static EnumConst* find_enum_const(Context* ctx, Token* tok);
static Typedef* find_typedef(Context* ctx, Token* tok);

Node* new_node(NodeKind kind, Node* lhs, Node* rhs) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->next = NULL;
    node->lhs = lhs;
    node->rhs = rhs;
    node->init = NULL;
    node->cond = NULL;
    node->val = 0;

    return node;
}

Node* new_node_num(int val) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    node->type = new_type_int();
    return node;
}

Node* new_node_ident(Context* ctx, Token* tok) {
    EnumConst* ec = find_enum_const(ctx, tok);
    if (ec) {
        return new_node_num(ec->val);
    }

    Node* node = calloc(1, sizeof(Node));
    LVar* lvar = find_lvar(ctx, tok);
    if (lvar) {
        node->kind = ND_LVAR;
        node->val = lvar->offset;
        node->type = lvar->type;
        return node;
    }

    LVar* gvar = find_gvar(ctx, tok);
    if (gvar) {
        node->kind = ND_GVAR;
        node->tok = tok;
        node->type = gvar->type;
        return node;
    }

    // Undeclared variable - error
    fprintf(stderr, "Error: undeclared variable '");
    fwrite(tok->str, 1, tok->len, stderr);
    fprintf(stderr, "'\n");
    exit(1);
}

void free_ast(Node* ast) {
    if (ast == NULL)
        return;
    // Don't free ast->lhs for ND_CALL (it's a Token*, not a Node*)
    if (ast->kind != ND_CALL) {
        free_ast(ast->lhs);
    }
    free_ast(ast->rhs);
    free_ast(ast->next);
    free_ast(ast->cond);
    free_ast(ast->init);
    free(ast);
}

// parse_type = "int" | "void" | type "*"
Type* parse_type(Context* ctx);

// Helper to create a new char type
Type* new_type_char(void) {
    Type* t = calloc(1, sizeof(Type));
    t->ty = CHAR;
    t->ptr_to = NULL;
    return t;
}

// Helper to create a new int type
Type* new_type_int(void) {
    Type* t = calloc(1, sizeof(Type));
    t->ty = INT;
    t->ptr_to = NULL;
    return t;
}

// Helper to create a pointer type
Type* new_type_ptr(Type* base) {
    Type* t = calloc(1, sizeof(Type));
    t->ty = PTR;
    t->ptr_to = base;
    return t;
}

// Helper to create an array type
Type* new_type_array(Type* base, size_t size) {
    Type* t = calloc(1, sizeof(Type));
    t->ty = PTR;
    t->ptr_to = base;
    t->array_size = size;
    return t;
}

static EnumConst* find_enum_const(Context* ctx, Token* tok) {
    for (EnumConst* ec = ctx->enum_consts; ec; ec = ec->next) {
        if (ec->len == tok->len && strncmp(ec->name, tok->str, ec->len) == 0)
            return ec;
    }
    return NULL;
}

static Typedef* find_typedef(Context* ctx, Token* tok) {
    for (Typedef* td = ctx->typedefs; td; td = td->next) {
        if (td->len == tok->len && strncmp(td->name, tok->str, td->len) == 0)
            return td;
    }
    return NULL;
}

// Try to parse a type, returns NULL if not a type
Type* try_parse_type(Context* ctx) {
    // Skip common qualifiers
    while (consume(ctx, "const") || consume(ctx, "static") ||
           consume(ctx, "extern"))
        ;

    // Parse base type
    Type* base = NULL;
    if (consume(ctx, "int")) {
        base = new_type_int();
    } else if (consume(ctx, "char")) {
        base = new_type_char();
    } else if (consume(ctx, "void")) {
        base = calloc(1, sizeof(Type));
        base->ty = VOID;
    } else if (consume(ctx, "long")) {
        base = new_type_int(); // long as int
    } else if (consume(ctx, "bool") ||
               (ctx->current_token->kind == TK_IDENT &&
                ctx->current_token->len == 4 &&
                strncmp(ctx->current_token->str, "bool", 4) == 0)) {
        consume(ctx, "bool");
        base = new_type_int();
    } else if (consume(ctx, "size_t")) {
        base = new_type_int();
    } else if (consume(ctx, "enum")) {
        consume_ident(ctx); // Ignore tag
        if (consume(ctx, "{")) {
            int val = 0;
            while (!consume(ctx, "}")) {
                Token* tok = consume_ident(ctx);
                if (!tok)
                    break;
                if (consume(ctx, "=")) {
                    val = expect_number(ctx);
                }
                EnumConst* ec = calloc(1, sizeof(EnumConst));
                ec->name = tok->str;
                ec->len = tok->len;
                ec->val = val++;
                ec->next = ctx->enum_consts;
                ctx->enum_consts = ec;
                consume(ctx, ",");
            }
        }
        base = new_type_int();
    } else if (consume(ctx, "struct")) {
        expect(ctx, "{");
        base = calloc(1, sizeof(Type));
        base->ty = STRUCT;
        Member head = {0};
        Member* cur = &head;
        int index = 0;
        while (!consume(ctx, "}")) {
            Type* mty = parse_type(ctx);
            Token* mtok = consume_ident(ctx);
            if (!mtok) {
                fprintf(stderr, "Expected member name\n");
                exit(1);
            }
            // Check for array in struct members if needed, but for now simple
            if (consume(ctx, "[")) {
                int size = expect_number(ctx);
                expect(ctx, "]");
                mty = new_type_array(mty, (size_t)size);
            }
            expect(ctx, ";");

            Member* m = calloc(1, sizeof(Member));
            m->type = mty;
            m->name = mtok->str;
            m->len = mtok->len;
            m->index = index++;
            cur->next = m;
            cur = m;
        }
        base->members = head.next;
    } else if (ctx->current_token->kind == TK_IDENT) {
        Typedef* td = find_typedef(ctx, ctx->current_token);
        if (td) {
            ctx->current_token = ctx->current_token->next;
            base = td->type;
        } else {
            return NULL;
        }
    } else {
        return NULL; // Not a type
    }

    // Parse pointers
    while (consume(ctx, "*")) {
        base = new_type_ptr(base);
    }

    return base;
}

Type* parse_type(Context* ctx) {
    Type* base = try_parse_type(ctx);
    if (!base) {
        fprintf(stderr, "Expected type (int or void)\n");
        exit(1);
    }
    return base;
}

// params = ty ident ("," ty ident)*
Node* parse_params(Context* ctx) {
    Type* ty = parse_type(ctx);
    Token* tok = consume_ident(ctx);
    if (!tok) {
        fprintf(stderr, "Expected parameter name\n");
        exit(1);
    }

    if (consume(ctx, "[")) {
        if (!consume(ctx, "]")) {
            expect_number(ctx);
            expect(ctx, "]");
        }
        ty = new_type_ptr(ty);
    }

    // Add parameter to locals before creating node
    LVar* lvar = add_lvar(ctx, tok, ty);
    Node* head = new_node_ident(ctx, tok);
    head->type = ty; // Store type in node
    head->val = lvar->offset;
    Node* tail = head;

    while (consume(ctx, ",")) {
        ty = parse_type(ctx);
        tok = consume_ident(ctx);
        if (!tok) {
            fprintf(stderr, "Expected parameter name\n");
            exit(1);
        }

        if (consume(ctx, "[")) {
            if (!consume(ctx, "]")) {
                expect_number(ctx);
                expect(ctx, "]");
            }
            ty = new_type_ptr(ty);
        }

        // Add parameter to locals before creating node
        lvar = add_lvar(ctx, tok, ty);
        tail->next = new_node_ident(ctx, tok);
        tail->next->type = ty; // Store type in node
        tail->next->val = lvar->offset;
        tail = tail->next;
    }

    return head;
}

// function = ty ident "(" params? ")" "{" stmt* "}"
Node* parse_function(Context* ctx) {
    Type* return_ty = parse_type(ctx);
    Token* tok = consume_ident(ctx);
    if (!tok) {
        fprintf(stderr, "Expected function name\n");
        exit(1);
    }

    // Reset locals for each function
    ctx->locals = NULL;

    expect(ctx, "(");

    // Parse parameters
    Node* func_params = NULL;
    if (!consume(ctx, ")")) {
        func_params = parse_params(ctx);
        expect(ctx, ")");
    }

    expect(ctx, "{");

    // Create function node with return type
    Node* node = new_node(ND_FUNCTION, NULL, NULL);
    node->tok = tok;
    node->type = return_ty;  // Store return type
    node->rhs = func_params; // Store parameters in rhs

    // Parse function body (statements)
    Node* head = NULL;
    Node* tail = NULL;
    while (!consume(ctx, "}")) {
        Node* stmt_node = parse_stmt(ctx);
        if (head == NULL) {
            head = stmt_node;
            tail = stmt_node;
        } else {
            tail->next = stmt_node;
            tail = stmt_node;
        }
    }
    node->lhs = head;
    node->locals = ctx->locals; // Store local variables

    return node;
}

// global_var = ty ident ("[" num "]")? ";"
Node* parse_global_var(Context* ctx) {
    Type* ty = parse_type(ctx);
    Token* tok = consume_ident(ctx);
    if (!tok) {
        fprintf(stderr, "Expected variable name\n");
        exit(1);
    }

    // Check for array definition
    if (consume(ctx, "[")) {
        int size = expect_number(ctx);
        expect(ctx, "]");
        ty = new_type_array(ty, size);
    }

    expect(ctx, ";");

    // Add to globals
    LVar* lvar = calloc(1, sizeof(LVar));
    lvar->name = tok->str;
    lvar->len = tok->len;
    lvar->type = ty;
    lvar->next = ctx->globals;
    ctx->globals = lvar;

    // Create global variable node
    Node* node = new_node(ND_GVAR, NULL, NULL);
    node->tok = tok;
    node->type = ty;
    return node;
}

void parse_program(Context* ctx) {
    int i = 0;
    while (!at_eof(ctx)) {
        if (consume(ctx, "typedef")) {
            Type* ty = parse_type(ctx);
            Token* tok = consume_ident(ctx);
            if (!tok) {
                fprintf(stderr, "Expected identifier after typedef\n");
                exit(1);
            }
            expect(ctx, ";");
            Typedef* td = calloc(1, sizeof(Typedef));
            td->name = tok->str;
            td->len = tok->len;
            td->type = ty;
            td->next = ctx->typedefs;
            ctx->typedefs = td;
            continue;
        }

        // Try to parse type first to distinguish function from global var
        Type* ty = try_parse_type(ctx);
        if (!ty) {
            fprintf(stderr, "Expected type or function definition\n");
            exit(1);
        }

        Token* tok = consume_ident(ctx);
        if (!tok) {
            if (consume(ctx, ";")) {
                // Standalone type definition: e.g. enum { A }; or struct { int
                // x; };
                continue;
            }
            fprintf(stderr, "Expected identifier\n");
            exit(1);
        }

        // Check if this is a function definition (followed by "(")
        if (consume(ctx, "(")) {
            // Reconstruct the parse state for parse_function
            ctx->locals = NULL;

            // Parse parameters
            Node* func_params = NULL;
            if (!consume(ctx, ")")) {
                func_params = parse_params(ctx);
                expect(ctx, ")");
            }

            if (consume(ctx, ";")) {
                // Prototype
                Node* node = new_node(ND_FUNCTION, NULL, NULL);
                node->tok = tok;
                node->type = ty;
                node->rhs = func_params;
                ctx->code[i++] = node;
                continue;
            }

            expect(ctx, "{");

            // Create function node with return type
            Node* node = new_node(ND_FUNCTION, NULL, NULL);
            node->tok = tok;
            node->type = ty;
            node->rhs = func_params;

            // Parse function body (statements)
            Node* head = NULL;
            Node* tail = NULL;
            while (!consume(ctx, "}")) {
                Node* stmt_node = parse_stmt(ctx);
                if (head == NULL) {
                    head = stmt_node;
                    tail = stmt_node;
                } else {
                    tail->next = stmt_node;
                    tail = stmt_node;
                }
            }
            node->lhs = head;
            node->locals = ctx->locals;

            ctx->code[i++] = node;
        } else {
            // This is a global variable
            // Put back the "[" or ";" token
            // Since we've already consumed the ident, we need to check for "["
            // or ";"
            if (consume(ctx, "[")) {
                int size = expect_number(ctx);
                expect(ctx, "]");
                ty = new_type_array(ty, size);
            }

            // Add to globals
            LVar* lvar = calloc(1, sizeof(LVar));
            lvar->name = tok->str;
            lvar->len = tok->len;
            lvar->type = ty;
            lvar->next = ctx->globals;
            ctx->globals = lvar;

            // Create global variable node
            Node* node = new_node(ND_GVAR, NULL, NULL);
            node->tok = tok;
            node->type = ty;

            if (consume(ctx, "=")) {
                node->init = parse_expr(ctx);
            }
            expect(ctx, ";");

            ctx->code[i++] = node;
        }
    }
    ctx->node_count = i;
}

Node* parse_stmt(Context* ctx) {
    Node* node;

    // Check for type declaration: e.g. int x; struct { ... } s;
    {
        Type* ty = try_parse_type(ctx);
        if (ty) {
            Token* tok = consume_ident(ctx);
            if (!tok) {
                fprintf(stderr, "Expected variable name after type\n");
                exit(1);
            }

            // Check for array definitions (e.g., int a[10], char x[3])
            if (consume(ctx, "[")) {
                int size = expect_number(ctx);
                expect(ctx, "]");
                ty = new_type_array(ty, size);
            }

            // Add variable to locals
            LVar* lvar = add_lvar(ctx, tok, ty);
            Node* decl = new_node(ND_DECL, NULL, NULL);
            decl->tok = tok;
            decl->val = lvar->offset;
            decl->type = ty;

            if (consume(ctx, "=")) {
                decl->init = parse_expr(ctx);
            }
            expect(ctx, ";");

            return decl;
        }
    }

    if (consume(ctx, "return")) {
        node = new_node(ND_RETURN, convert_array_to_ptr(parse_expr(ctx)), NULL);
    } else if (consume(ctx, "if")) {
        expect(ctx, "(");
        Node* cond = convert_array_to_ptr(parse_expr(ctx));
        expect(ctx, ")");
        Node* then = parse_stmt(ctx);
        Node* els = NULL;
        if (consume(ctx, "else")) {
            els = parse_stmt(ctx);
        }
        node = new_node(ND_IF, then, els);
        node->cond = cond;
        return node;
    } else if (consume(ctx, "while")) {
        expect(ctx, "(");
        Node* cond = convert_array_to_ptr(parse_expr(ctx));
        expect(ctx, ")");
        Node* body = parse_stmt(ctx);
        node = new_node(ND_WHILE, body, NULL);
        node->cond = cond;
        return node;
    } else if (consume(ctx, "for")) {
        expect(ctx, "(");
        Node* init = NULL;
        if (!consume(ctx, ";")) {
            init = convert_array_to_ptr(parse_expr(ctx));
            expect(ctx, ";");
        }
        Node* cond = NULL;
        if (!consume(ctx, ";")) {
            cond = convert_array_to_ptr(parse_expr(ctx));
            expect(ctx, ";");
        }
        Node* inc = NULL;
        if (!consume(ctx, ")")) {
            inc = convert_array_to_ptr(parse_expr(ctx));
            expect(ctx, ")");
        }
        Node* body = parse_stmt(ctx);
        node = new_node(ND_FOR, body, inc);
        node->init = init;
        node->cond = cond;
        return node;
    } else if (consume(ctx, "switch")) {
        expect(ctx, "(");
        Node* cond = convert_array_to_ptr(parse_expr(ctx));
        expect(ctx, ")");

        Node* node = new_node(ND_SWITCH, NULL, NULL);
        node->cond = cond;

        Node* old_switch = ctx->current_switch;
        ctx->current_switch = node;

        node->lhs = parse_stmt(ctx);

        ctx->current_switch = old_switch;
        return node;
    } else if (consume(ctx, "case")) {
        if (!ctx->current_switch) {
            fprintf(stderr, "case outside switch\n");
            exit(1);
        }
        int val = expect_number(ctx);
        expect(ctx, ":");
        Node* node = new_node(ND_CASE, NULL, NULL);
        node->case_val = val;
        node->next_case = ctx->current_switch->cases;
        ctx->current_switch->cases = node;
        return node;
    } else if (consume(ctx, "default")) {
        if (!ctx->current_switch) {
            fprintf(stderr, "default outside switch\n");
            exit(1);
        }
        expect(ctx, ":");
        Node* node = new_node(ND_CASE, NULL, NULL);
        node->is_default = true;
        node->next_case = ctx->current_switch->cases;
        ctx->current_switch->cases = node;
        return node;
    } else if (consume(ctx, "break")) {
        expect(ctx, ";");
        return new_node(ND_BREAK, NULL, NULL);
    } else if (consume(ctx, "{")) {
        // Block statement: consume statements until }
        node = parse_stmt(ctx);
        Node* head = node;
        while (!consume(ctx, "}")) {
            node->next = parse_stmt(ctx);
            node = node->next;
        }
        return new_node(ND_BLOCK, head, NULL);
    } else {
        node = parse_expr(ctx);
    }
    expect(ctx, ";");

    return node;
}

Node* parse_expr(Context* ctx) { return parse_assign(ctx); }

// logor = logand ("||" logand)*
static Node* parse_logor(Context* ctx) {
    Node* node = parse_logand(ctx);
    for (;;) {
        if (consume(ctx, "||")) {
            Node* rhs = parse_logand(ctx);
            node = new_node(ND_LOGOR, node, rhs);
            node->type = new_type_int();
        } else {
            return node;
        }
    }
}

// logand = equality ("&&" equality)*
static Node* parse_logand(Context* ctx) {
    Node* node = parse_equality(ctx);
    for (;;) {
        if (consume(ctx, "&&")) {
            Node* rhs = parse_equality(ctx);
            node = new_node(ND_LOGAND, node, rhs);
            node->type = new_type_int();
        } else {
            return node;
        }
    }
}

Node* parse_assign(Context* ctx) {
    Node* node = parse_logor(ctx);
    if (consume(ctx, "=")) {
        node =
            new_node(ND_ASSIGN, node, convert_array_to_ptr(parse_assign(ctx)));
        if (node->lhs) {
            node->type = node->lhs->type;
        }
    }
    return node;
}

Node* parse_equality(Context* ctx) {
    Node* node = parse_relational(ctx);
    while (1) {
        if (consume(ctx, "==")) {
            node = new_node(ND_EQ, convert_array_to_ptr(node),
                            convert_array_to_ptr(parse_relational(ctx)));
            node->type = new_type_int();
        } else if (consume(ctx, "!=")) {
            node = new_node(ND_NE, convert_array_to_ptr(node),
                            convert_array_to_ptr(parse_relational(ctx)));
            node->type = new_type_int();
        } else {
            return node;
        }
    }
}

Node* parse_relational(Context* ctx) {
    Node* node = parse_add(ctx);
    while (1) {
        if (consume(ctx, "<=")) {
            node = new_node(ND_LE, convert_array_to_ptr(node),
                            convert_array_to_ptr(parse_add(ctx)));
            node->type = new_type_int();
        } else if (consume(ctx, ">=")) {
            node = new_node(ND_GE, convert_array_to_ptr(node),
                            convert_array_to_ptr(parse_add(ctx)));
            node->type = new_type_int();
        } else if (consume(ctx, "<")) {
            node = new_node(ND_LT, convert_array_to_ptr(node),
                            convert_array_to_ptr(parse_add(ctx)));
            node->type = new_type_int();
        } else if (consume(ctx, ">")) {
            node = new_node(ND_GT, convert_array_to_ptr(node),
                            convert_array_to_ptr(parse_add(ctx)));
            node->type = new_type_int();
        } else {
            return node;
        }
    }
}

Node* parse_add(Context* ctx) {
    Node* node = parse_mul(ctx);
    while (1) {
        if (consume(ctx, "+")) {
            Node* rhs = convert_array_to_ptr(parse_mul(ctx));
            node = convert_array_to_ptr(node);
            Node* newNode = new_node(ND_ADD, node, rhs);
            if (node->type && node->type->ty == PTR) {
                newNode->type = node->type;
            } else if (rhs->type && rhs->type->ty == PTR) {
                newNode->type = rhs->type;
            } else {
                newNode->type = new_type_int();
            }
            node = newNode;
        } else if (consume(ctx, "-")) {
            Node* rhs = convert_array_to_ptr(parse_mul(ctx));
            node = convert_array_to_ptr(node);
            Node* newNode = new_node(ND_SUB, node, rhs);
            if (node->type && node->type->ty == PTR) {
                newNode->type = node->type;
            } else {
                newNode->type = new_type_int();
            }
            node = newNode;
        } else {
            return node;
        }
    }
}

Node* parse_mul(Context* ctx) {
    Node* node = parse_unary(ctx);
    while (1) {
        if (consume(ctx, "*")) {
            node = new_node(ND_MUL, convert_array_to_ptr(node),
                            convert_array_to_ptr(parse_unary(ctx)));
            node->type = new_type_int();
        } else if (consume(ctx, "/")) {
            node = new_node(ND_DIV, convert_array_to_ptr(node),
                            convert_array_to_ptr(parse_unary(ctx)));
            node->type = new_type_int();
        } else {
            return node;
        }
    }
}

static int type_size(Type* ty) {
    if (ty == NULL)
        return 4;
    if (ty->ty == CHAR)
        return 1;
    if (ty->ty == INT)
        return 4;
    if (ty->ty == STRUCT) {
        int size = 0;
        for (Member* m = ty->members; m; m = m->next) {
            size += type_size(m->type);
        }
        return size;
    }
    if (ty->array_size > 0)
        return type_size(ty->ptr_to) * ty->array_size;
    if (ty->ty == PTR)
        return 8;
    return 4;
}

static Member* find_member(Type* ty, Token* tok) {
    for (Member* m = ty->members; m; m = m->next) {
        if (m->len == tok->len && memcmp(m->name, tok->str, tok->len) == 0) {
            return m;
        }
    }
    return NULL;
}

static Node* parse_postfix(Context* ctx) {
    Node* node = parse_primary(ctx);

    for (;;) {
        if (consume(ctx, "[")) {
            // x[y] -> *(x+y)
            Node* index = parse_expr(ctx);
            expect(ctx, "]");
            Node* base = convert_array_to_ptr(node);
            Node* add_node = new_node(ND_ADD, base, index);
            if (base->type && base->type->ty == PTR) {
                add_node->type = base->type;
            } else if (index->type && index->type->ty == PTR) {
                add_node->type = index->type;
            } else {
                add_node->type = new_type_int();
            }
            node = new_node(ND_DEREF, add_node, NULL);
            if (add_node->type && add_node->type->ty == PTR) {
                node->type = add_node->type->ptr_to;
            } else {
                node->type = new_type_int();
            }
            continue;
        }

        if (consume(ctx, ".")) {
            Token* tok = consume_ident(ctx);
            if (!tok) {
                fprintf(stderr, "Expected member name after '.'\n");
                exit(1);
            }
            if (!node->type || node->type->ty != STRUCT) {
                fprintf(stderr, "Left side of '.' is not a struct\n");
                exit(1);
            }
            Member* m = find_member(node->type, tok);
            if (!m) {
                fprintf(stderr, "Member '%.*s' not found\n", tok->len,
                        tok->str);
                exit(1);
            }
            Node* n = new_node(ND_MEMBER, node, NULL);
            n->type = m->type;
            n->member = m;
            node = n;
            continue;
        }

        if (consume(ctx, "->")) {
            Token* tok = consume_ident(ctx);
            if (!tok) {
                fprintf(stderr, "Expected member name after '->'\n");
                exit(1);
            }
            node = convert_array_to_ptr(node);
            if (!node->type || node->type->ty != PTR ||
                node->type->ptr_to->ty != STRUCT) {
                fprintf(stderr,
                        "Left side of '->' is not a pointer to struct\n");
                exit(1);
            }
            // p->m is (*p).m
            Node* deref = new_node(ND_DEREF, node, NULL);
            deref->type = node->type->ptr_to;

            Member* m = find_member(deref->type, tok);
            if (!m) {
                fprintf(stderr, "Member '%.*s' not found\n", tok->len,
                        tok->str);
                exit(1);
            }
            Node* n = new_node(ND_MEMBER, deref, NULL);
            n->type = m->type;
            n->member = m;
            node = n;
            continue;
        }

        if (consume(ctx, "++")) {
            Node* n = new_node(ND_POST_INC, node, NULL);
            n->type = node->type;
            node = n;
            continue;
        }

        if (consume(ctx, "--")) {
            Node* n = new_node(ND_POST_DEC, node, NULL);
            n->type = node->type;
            node = n;
            continue;
        }

        return node;
    }
}

static Node* convert_array_to_ptr(Node* node) {
    if (node->type && node->type->array_size > 0) {
        Node* n = new_node(ND_ARRAY_TO_PTR, node, NULL);
        n->type = new_type_ptr(node->type->ptr_to);
        return n;
    }
    return node;
}

static bool is_type(Context* ctx) {
    Token* tok = ctx->current_token;
    if (tok->kind == TK_RESERVED) {
        char* keywords[] = {"int",    "char",   "void",    "long",  "bool",
                            "size_t", "enum",   "struct",  "const", "static",
                            "extern", "signed", "unsigned"};
        for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
            if ((size_t)tok->len == strlen(keywords[i]) &&
                strncmp(tok->str, keywords[i], tok->len) == 0)
                return true;
        }
    }
    if (tok->kind == TK_IDENT && find_typedef(ctx, tok))
        return true;
    return false;
}

Node* parse_unary(Context* ctx) {
    if (ctx->current_token->kind == TK_RESERVED &&
        ctx->current_token->len == 1 && ctx->current_token->str[0] == '(') {
        Token* next = ctx->current_token->next;
        // Peek if it's a type
        Token* old_tok = ctx->current_token;
        ctx->current_token = next;
        bool it = is_type(ctx);
        ctx->current_token = old_tok;

        if (it) {
            consume(ctx, "(");
            Type* ty = parse_type(ctx);
            expect(ctx, ")");
            Node* node = new_node(ND_CAST, parse_unary(ctx), NULL);
            node->type = ty;
            return node;
        }
    }

    if (consume(ctx, "+")) {
        return parse_unary(ctx);
    }
    if (consume(ctx, "-")) {
        Node* node = new_node(ND_SUB, new_node_num(0), parse_unary(ctx));
        node->type = new_type_int();
        return node;
    }
    if (consume(ctx, "sizeof")) {
        if (consume(ctx, "(")) {
            Type* ty = try_parse_type(ctx);
            if (ty) {
                expect(ctx, ")");
                return new_node_num(type_size(ty));
            }
            Node* node = parse_expr(ctx);
            expect(ctx, ")");
            Type* ty_res = node->type;
            if (node->kind == ND_ARRAY_TO_PTR)
                ty_res = node->lhs->type;
            return new_node_num(type_size(ty_res));
        } else {
            Node* node = parse_unary(ctx);
            Type* ty_res = node->type;
            if (node->kind == ND_ARRAY_TO_PTR)
                ty_res = node->lhs->type;
            return new_node_num(type_size(ty_res));
        }
    }
    if (consume(ctx, "++")) {
        Node* node = parse_unary(ctx);
        Node* res = new_node(ND_PRE_INC, node, NULL);
        res->type = node->type;
        return res;
    }
    if (consume(ctx, "--")) {
        Node* node = parse_unary(ctx);
        Node* res = new_node(ND_PRE_DEC, node, NULL);
        res->type = node->type;
        return res;
    }
    if (consume(ctx, "*")) {
        Node* operand = convert_array_to_ptr(parse_unary(ctx));
        Node* node = new_node(ND_DEREF, operand, NULL);
        if (operand->type && operand->type->ty == PTR) {
            node->type = operand->type->ptr_to;
        } else {
            node->type = new_type_int();
        }
        return node;
    }
    if (consume(ctx, "&")) {
        Node* operand = parse_unary_no_array_conv(ctx);
        Node* node = new_node(ND_ADDR, operand, NULL);
        node->type =
            new_type_ptr(operand->type ? operand->type : new_type_int());
        return node;
    }
    if (consume(ctx, "!")) {
        Node* operand = convert_array_to_ptr(parse_unary(ctx));
        Node* node = new_node(ND_NOT, operand, NULL);
        node->type = new_type_int();
        return node;
    }
    return parse_postfix(ctx);
}

static Node* parse_unary_no_array_conv(Context* ctx) {
    if (consume(ctx, "+")) {
        return parse_postfix(ctx);
    } else if (consume(ctx, "-")) {
        Node* node = new_node(ND_SUB, new_node_num(0), parse_postfix(ctx));
        node->type = new_type_int();
        return node;
    } else if (consume(ctx, "sizeof")) {
        // ... nested sizeof ...
        if (consume(ctx, "(")) {
            Type* ty = try_parse_type(ctx);
            if (ty) {
                expect(ctx, ")");
                return new_node_num(type_size(ty));
            }
            Node* node = parse_expr(ctx);
            expect(ctx, ")");
            return new_node_num(type_size(node->type));
        } else {
            Node* node = parse_unary_no_array_conv(ctx);
            return new_node_num(type_size(node->type));
        }
    } else if (consume(ctx, "*")) {
        Node* operand = parse_unary(ctx); // Deref DOES convert array to ptr
        Node* node = new_node(ND_DEREF, operand, NULL);
        if (operand->type && operand->type->ty == PTR) {
            node->type = operand->type->ptr_to;
        } else {
            node->type = new_type_int();
        }
        return node;
    } else if (consume(ctx, "&")) {
        Node* operand = parse_unary_no_array_conv(ctx); // &array is allowed
        Node* node = new_node(ND_ADDR, operand, NULL);
        node->type =
            new_type_ptr(operand->type ? operand->type : new_type_int());
        return node;
    } else {
        return parse_postfix(ctx);
    }
}

Node* parse_primary(Context* ctx) {
    if (consume(ctx, "NULL") || consume(ctx, "false")) {
        return new_node_num(0);
    }
    if (consume(ctx, "true")) {
        return new_node_num(1);
    }

    // "(" expr ")"
    if (consume(ctx, "(")) {
        Node* node = parse_expr(ctx);
        expect(ctx, ")");
        return node;
    }

    // String literal
    if (ctx->current_token->kind == TK_STR) {
        Token* tok = ctx->current_token;
        ctx->current_token = ctx->current_token->next;
        // Add string to context's string vector
        int idx = ctx->string_count++;
        ctx->strings[idx] = tok->str;
        ctx->string_lens[idx] = tok->len;
        // Create ND_STR node
        Node* node = calloc(1, sizeof(Node));
        node->kind = ND_STR;
        node->val = idx; // index into strings vector
        // Type is char* (pointer to char)
        node->type = new_type_ptr(new_type_char());
        return node;
    }

    // function call or ident
    Token* tok = consume_ident(ctx);

    if (tok) {
        if (consume(ctx, "(")) {
            Node* node_args = NULL;
            if (!consume(ctx, ")")) {
                // args
                node_args = parse_args(ctx);
                expect(ctx, ")");
            }
            // function call
            Node* node_func = new_node(ND_CALL, node_args, NULL);
            node_func->tok = tok;
            node_func->type = new_type_int();
            return node_func;
        }
        // ident or array subscript
        Node* ident_node = new_node_ident(ctx, tok);

        // Check for array subscript: ident[expr]
        if (consume(ctx, "[")) {
            // x[y] -> *(x+y)
            Node* index = parse_expr(ctx);
            expect(ctx, "]");

            // For arrays, ident_node is the array variable.
            // We need to get its address, then add the index.
            // convert_array_to_ptr handles this by wrapping in ND_ADDR
            Node* base = convert_array_to_ptr(ident_node);

            // Create addition node (base+index)
            Node* add_node = new_node(ND_ADD, base, index);
            // Type inference for pointer addition
            if (base->type && base->type->ty == PTR) {
                add_node->type = base->type;
            } else if (index->type && index->type->ty == PTR) {
                add_node->type = index->type;
            } else {
                add_node->type = new_type_int();
            }
            // Create dereference node *(...)
            Node* deref_node = new_node(ND_DEREF, add_node, NULL);
            if (add_node->type && add_node->type->ty == PTR) {
                deref_node->type = add_node->type->ptr_to;
            } else {
                deref_node->type = new_type_int();
            }
            return deref_node;
        }
        return ident_node;
    }

    // Check for number followed by subscript (e.g., 0[a])
    if (ctx->current_token->kind == TK_NUM) {
        Node* num_node = new_node_num(expect_number(ctx));
        // Check for [ after number
        if (consume(ctx, "[")) {
            // 0[y] -> *(0+y)
            Node* index = parse_expr(ctx);
            expect(ctx, "]");
            // Create addition node (0+index)
            Node* add_node = new_node(ND_ADD, num_node, index);
            // Type inference
            if (index->type && index->type->ty == PTR) {
                add_node->type = index->type;
            } else {
                add_node->type = new_type_int();
            }
            // Create dereference node *(...)
            Node* deref_node = new_node(ND_DEREF, add_node, NULL);
            if (add_node->type && add_node->type->ty == PTR) {
                deref_node->type = add_node->type->ptr_to;
            } else {
                deref_node->type = new_type_int();
            }
            return deref_node;
        }
        return num_node;
    }

    // number (without subscript)
    return new_node_num(expect_number(ctx));
}

Node* parse_args(Context* ctx) {
    Node* node = convert_array_to_ptr(parse_expr(ctx));
    Node* head = node;
    while (consume(ctx, ",")) {
        node->next = convert_array_to_ptr(parse_expr(ctx));
        node = node->next;
    }
    return head;
}