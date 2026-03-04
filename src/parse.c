#include "parse.h"
#include "lex.h"
#include "variable.h"
#include <stdlib.h>
#include <string.h>

#define LLVM7_INT_MAX 2147483647

static Node* parse_logor(Context* ctx);
static Node* parse_conditional(Context* ctx);
static Node* parse_logand(Context* ctx);
static Node* parse_bit_or(Context* ctx);
static Node* parse_bit_xor(Context* ctx);
static Node* parse_bit_and(Context* ctx);
static Node* parse_shift(Context* ctx);
static Node* convert_array_to_ptr(Node* node);
static Node* parse_unary_no_array_conv(Context* ctx);
static EnumConst* find_enum_const(Context* ctx, Token* tok);
static Typedef* find_typedef(Context* ctx, Token* tok);
static Node* clone_ast(Node* node);
static Node* parse_sizeof_expr_node(Context* ctx, Node* node);
static int type_size(Type* ty);
static Node* find_defined_function(Context* ctx, Token* tok);

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
    node->uval = (unsigned long long)(unsigned int)val;
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

    Node* fn = find_defined_function(ctx, tok);
    if (fn) {
        node->kind = ND_FUNCNAME;
        node->tok = tok;
        node->type = new_type_ptr(fn->type ? fn->type : new_type_int());
        return node;
    }

    // Undeclared variable - error
    fprintf(stderr, "Error: undeclared variable '");
    fwrite(tok->str, 1, tok->len, stderr);
    fprintf(stderr, "'\n");
    exit(1);
}

static Node* find_defined_function(Context* ctx, Token* tok) {
    for (int i = 0; i < MAX_NODES; i++) {
        Node* n = ctx->code[i];
        if (!n || n->kind != ND_FUNCTION || !n->tok)
            continue;
        if (n->tok->len == tok->len &&
            strncmp(n->tok->str, tok->str, tok->len) == 0)
            return n;
    }
    return NULL;
}

void free_ast(Node* ast) {
    if (ast == NULL)
        return;
    free_ast(ast->lhs);
    free_ast(ast->rhs);
    free_ast(ast->next);
    free_ast(ast->cond);
    free_ast(ast->init);
    free(ast);
}

static Node* clone_ast(Node* node) {
    if (node == NULL) {
        return NULL;
    }

    Node* cloned = calloc(1, sizeof(Node));
    *cloned = *node;
    cloned->lhs = clone_ast(node->lhs);
    cloned->rhs = clone_ast(node->rhs);
    cloned->next = clone_ast(node->next);
    cloned->cond = clone_ast(node->cond);
    cloned->init = clone_ast(node->init);
    return cloned;
}

static Node* parse_sizeof_expr_node(Context* ctx, Node* node) {
    if (!node) {
        return new_node_num(0);
    }

    if (node->kind == ND_ARRAY_TO_PTR)
        node = node->lhs;

    if (node->kind == ND_LVAR && node->type && node->type->ty == PTR &&
        node->type->array_size == 0 && node->type->ptr_to && node->val >= 0 &&
        node->val < MAX_NODES && ctx->vla_size_exprs[node->val]) {
        Node* count = clone_ast(ctx->vla_size_exprs[node->val]);
        Node* elem_size = new_node_num(type_size(node->type->ptr_to));
        Node* mul = new_node(ND_MUL, count, elem_size);
        mul->type = get_common_type(count->type, elem_size->type);
        return mul;
    }

    Type* ty_res = node->type;
    if (node->kind == ND_ARRAY_TO_PTR && node->lhs)
        ty_res = node->lhs->type;
    return new_node_num(type_size(ty_res));
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

// Helper to create a new double type
Type* new_type_double(void) {
    Type* t = calloc(1, sizeof(Type));
    t->ty = DOUBLE;
    t->ptr_to = NULL;
    return t;
}

// Helper to create a new float type
Type* new_type_float(void) {
    Type* t = calloc(1, sizeof(Type));
    t->ty = FLOAT;
    t->ptr_to = NULL;
    return t;
}

Node* new_node_fnum(double fval, Type* ty) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = ND_FNUM;
    node->fval = fval;
    node->type = ty;
    return node;
}

static int get_type_rank(Type* ty) {
    if (ty->ty == DOUBLE)
        return 100;
    if (ty->ty == FLOAT)
        return 90;
    if (ty->ty == LONG)
        return 80;
    if (ty->ty == INT)
        return 70;
    if (ty->ty == CHAR)
        return 60;
    return 0;
}

Type* get_common_type(Type* ty1, Type* ty2) {
    if (ty1->ty == PTR)
        return ty1;
    if (ty2->ty == PTR)
        return ty2;

    int r1 = get_type_rank(ty1);
    int r2 = get_type_rank(ty2);

    if (r1 > r2)
        return ty1;
    if (r2 > r1)
        return ty2;

    // Ranks are equal (same base type), check signedness
    if (ty1->is_unsigned || ty2->is_unsigned) {
        Type* res = calloc(1, sizeof(Type));
        *res = *ty1; // Copy type
        res->is_unsigned = true;
        return res;
    }

    return ty1;
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

static int expect_const_int(Context* ctx) {
    if (ctx->current_token->kind == TK_NUM) {
        return expect_number(ctx);
    }
    Token* tok = consume_ident(ctx);
    if (tok) {
        EnumConst* ec = find_enum_const(ctx, tok);
        if (ec)
            return ec->val;
        fprintf(stderr,
                "Expected constant integer, but got identifier: '%.*s'\n",
                tok->len, tok->str);
        exit(1);
    }
    fprintf(stderr, "Expected constant integer, token kind=%d near: '%.40s'\n",
            ctx->current_token->kind, ctx->current_token->str);

    return expect_number(ctx);
}

static StructTag* find_tag(Context* ctx, Token* tok) {
    for (StructTag* tag = ctx->struct_tags; tag; tag = tag->next) {
        if (tag->len == tok->len && strncmp(tag->name, tok->str, tag->len) == 0)
            return tag;
    }
    return NULL;
}

// Try to parse a type, returns NULL if not a type
Type* try_parse_type(Context* ctx) {
    // Skip common qualifiers
    bool is_unsigned = false;
    bool has_qualifier = false;
    while (1) {
        if (consume(ctx, "const") || consume(ctx, "static") ||
            consume(ctx, "extern") || consume(ctx, "signed") ||
            consume(ctx, "inline") || consume(ctx, "restrict") ||
            consume(ctx, "volatile") || consume(ctx, "register")) {
            has_qualifier = true;
            continue;
        }
        if (consume(ctx, "unsigned")) {
            is_unsigned = true;
            has_qualifier = true;
            continue;
        }
        break;
    }

    // Parse base type
    Type* base = NULL;
    if (consume(ctx, "int")) {
        base = new_type_int();
    } else if (consume(ctx, "char")) {
        base = new_type_char();
    } else if (consume(ctx, "float")) {
        consume(ctx, "_Complex");
        base = new_type_float();
    } else if (consume(ctx, "double")) {
        consume(ctx, "_Complex");
        base = new_type_double();
    } else if (consume(ctx, "void")) {
        base = calloc(1, sizeof(Type));
        base->ty = VOID;
    } else if (consume(ctx, "long")) {
        if (consume(ctx, "double")) {
            consume(ctx, "_Complex");
            // Treat long double as double in current backend.
            base = new_type_double();
        } else {
            consume(ctx, "long"); // Support 'long long'
            base = calloc(1, sizeof(Type));
            base->ty = LONG;
        }
    } else if (consume(ctx, "_Complex")) {
        if (consume(ctx, "float")) {
            base = new_type_float();
        } else {
            consume(ctx, "long");
            consume(ctx, "double");
            base = new_type_double();
        }
    } else if (consume(ctx, "_Bool") || consume(ctx, "bool") ||
               (ctx->current_token->kind == TK_IDENT &&
                ctx->current_token->len == 4 &&
                strncmp(ctx->current_token->str, "bool", 4) == 0)) {
        consume(ctx, "bool");
        base = new_type_int();
    } else if (consume(ctx, "size_t")) {
        base = calloc(1, sizeof(Type));
        base->ty = LONG;
        base->is_unsigned = true;
    } else if (consume(ctx, "enum")) {
        // ... (keep rest of enum logic)
        consume_ident(ctx); // Ignore tag
        if (consume(ctx, "{")) {
            int val = 0;
            while (!consume(ctx, "}")) {
                Token* tok = consume_ident(ctx);
                if (!tok)
                    break;
                if (consume(ctx, "=")) {
                    val = expect_const_int(ctx);
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
    } else if (ctx->current_token->kind == TK_RESERVED &&
               ((ctx->current_token->len == 6 &&
                 strncmp(ctx->current_token->str, "struct", 6) == 0) ||
                (ctx->current_token->len == 5 &&
                 strncmp(ctx->current_token->str, "union", 5) == 0))) {
        bool is_union = (ctx->current_token->len == 5 &&
                         strncmp(ctx->current_token->str, "union", 5) == 0);
        ctx->current_token = ctx->current_token->next;
        Token* tag = consume_ident(ctx);
        Type* str_type = NULL;

        if (tag) {
            StructTag* st = find_tag(ctx, tag);
            if (st) {
                str_type = st->type;
            } else {
                str_type = calloc(1, sizeof(Type));
                str_type->ty = is_union ? UNION : STRUCT;
                StructTag* nst = calloc(1, sizeof(StructTag));
                nst->name = tag->str;
                nst->len = tag->len;
                nst->type = str_type;
                nst->next = ctx->struct_tags;
                ctx->struct_tags = nst;
            }
        } else {
            // Anonymous struct
            str_type = calloc(1, sizeof(Type));
            str_type->ty = is_union ? UNION : STRUCT;
        }

        if (consume(ctx, "{")) {
            Member head;
            memset(&head, 0, sizeof(Member));
            Member* cur = &head;
            int index = 0;
            while (!consume(ctx, "}")) {
                Type* mty = parse_type(ctx);
                Token* mtok = consume_ident(ctx);
                if (!mtok) {
                    fprintf(stderr, "Expected member name\\n");
                    exit(1);
                }
                // Check for array in struct members if needed, but for now
                // simple
                if (consume(ctx, "[")) {
                    int size = 0;
                    if (!consume(ctx, "]")) {
                        size = expect_const_int(ctx);
                        expect(ctx, "]");
                    }
                    mty = new_type_array(mty, (size_t)size);
                }
                Member* m = calloc(1, sizeof(Member));
                m->type = mty;
                m->name = mtok->str;
                m->len = mtok->len;
                m->index = index++;
                if (consume(ctx, ":")) {
                    int bw = expect_const_int(ctx);
                    if (bw <= 0 || bw > 32) {
                        fprintf(stderr, "bitfield width must be 1..32\n");
                        exit(1);
                    }
                    m->is_bitfield = true;
                    m->bit_width = bw;
                    // Current backend models bitfields as independent integer
                    // members. Keep sequential struct index and record metadata
                    // for future packed-bitfield support.
                    m->bit_offset = 0;
                    if (is_union) {
                        m->index = 0;
                    }
                } else if (is_union) {
                    m->index = 0;
                }
                expect(ctx, ";");
                cur->next = m;
                cur = m;
            }
            str_type->members = head.next;
        } else if (!tag) {
            fprintf(stderr, "Expected '{' after struct\\n");
            exit(1);
        }

        base = str_type;
    } else if (ctx->current_token->kind == TK_IDENT) {
        Typedef* td = find_typedef(ctx, ctx->current_token);
        if (td) {
            ctx->current_token = ctx->current_token->next;
            base = td->type;
        }
    }

    if (base == NULL && has_qualifier) {
        base = new_type_int();
    }

    if (base == NULL) {
        return NULL;
    }

    if (is_unsigned) {
        base->is_unsigned = true;
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
Node* parse_params(Context* ctx, bool* is_vararg) {
    *is_vararg = false;
    Type* ty = parse_type(ctx);
    Token* tok = consume_ident(ctx);

    if (!tok) {
        // Handle (void)
        if (ty->ty == VOID && ty->ptr_to == NULL) {
            // Check if next is ')'
            if (ctx->current_token->kind == TK_RESERVED &&
                strncmp(ctx->current_token->str, ")", 1) == 0) {
                return NULL;
            }
        }
        // Arguments without name (prototype)
        tok = calloc(1, sizeof(Token));
        tok->kind = TK_IDENT;
        tok->str = "";
        tok->len = 0;
    }

    if (consume(ctx, "[")) {
        if (!consume(ctx, "]")) {
            expect_const_int(ctx);
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
        // Check for ellipsis (variadic arguments)
        if (consume(ctx, "...")) {
            *is_vararg = true;
            return head;
        }

        ty = parse_type(ctx);
        tok = consume_ident(ctx);
        if (!tok) {
            // Arguments without name
            tok = calloc(1, sizeof(Token));
            tok->kind = TK_IDENT;
            tok->str = "";
            tok->len = 0;
        }

        if (consume(ctx, "[")) {
            if (!consume(ctx, "]")) {
                expect_const_int(ctx);
                expect(ctx, "]");
            }
            ty = new_type_ptr(ty);
        }

        lvar = add_lvar(ctx, tok, ty);
        Node* node = new_node_ident(ctx, tok);
        node->type = ty;
        node->val = lvar->offset;
        tail->next = node;
        tail = node;
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
    reset_scope(ctx);

    expect(ctx, "(");

    // Parse parameters
    Node* func_params = NULL;
    bool is_vararg = false;
    if (!consume(ctx, ")")) {
        func_params = parse_params(ctx, &is_vararg);
        expect(ctx, ")");
    }

    expect(ctx, "{");

    // Create function node with return type
    Node* node = new_node(ND_FUNCTION, NULL, NULL);
    node->tok = tok;
    node->type = return_ty;  // Store return type
    node->rhs = func_params; // Store parameters in rhs
    node->is_vararg = is_vararg;

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
        int size = expect_const_int(ctx);
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

Node* parse_expr(Context* ctx);

static int count_members(Type* ty) {
    int n = 0;
    Member* m = ty ? ty->members : NULL;
    for (; m; m = m->next)
        n++;
    return n;
}

static Member* find_member_by_tok(Type* ty, Token* tok) {
    for (Member* m = ty ? ty->members : NULL; m; m = m->next) {
        if (m->len == tok->len && strncmp(m->name, tok->str, tok->len) == 0)
            return m;
    }
    return NULL;
}

static Node* new_zero_init_node(void) {
    Node* z = new_node_num(0);
    return z;
}

static Node* parse_initializer(Context* ctx, Type* target_type) {
    expect(ctx, "{");
    Node* node = new_node(ND_INIT, NULL, NULL);
    Node** slots = NULL;
    int slot_count = 0;
    int slot_cap = 0;
    int next_index = 0;
    bool first = true;

    bool is_array =
        (target_type && target_type->ty == PTR && target_type->array_size > 0);
    bool is_struct = (target_type && target_type->ty == STRUCT);
    bool is_union = (target_type && target_type->ty == UNION);
    int fixed_count = -1;
    if (is_array) {
        fixed_count = (int)target_type->array_size;
    } else if (is_struct) {
        fixed_count = count_members(target_type);
    } else if (is_union) {
        fixed_count = 1;
    }

    while (!consume(ctx, "}")) {
        if (!first)
            expect(ctx, ",");
        first = false;
        if (consume(ctx, "}"))
            break; // trailing comma

        int index = next_index;
        bool has_designator = false;
        Member* designated_member = NULL;
        if (consume(ctx, "[")) {
            index = expect_const_int(ctx);
            expect(ctx, "]");
            expect(ctx, "=");
            has_designator = true;
        } else if (consume(ctx, ".")) {
            Token* mtok = consume_ident(ctx);
            if (!mtok) {
                fprintf(stderr,
                        "Expected member name after '.' in initializer\n");
                exit(1);
            }
            if (!is_struct && !is_union) {
                fprintf(stderr,
                        "'.member' designator requires struct/union type\n");
                exit(1);
            }
            designated_member = find_member_by_tok(target_type, mtok);
            if (!designated_member) {
                fprintf(stderr, "Unknown struct member in initializer\n");
                exit(1);
            }
            index = is_union ? 0 : designated_member->index;
            expect(ctx, "=");
            has_designator = true;
        }

        if (index < 0) {
            fprintf(stderr, "Invalid designator index\n");
            exit(1);
        }
        if (fixed_count >= 0 && index >= fixed_count) {
            fprintf(stderr, "Initializer index out of range\n");
            exit(1);
        }

        if (index >= slot_cap) {
            int new_cap = slot_cap ? slot_cap : 8;
            while (new_cap <= index)
                new_cap *= 2;
            Node** new_slots = realloc(slots, sizeof(Node*) * new_cap);
            if (!new_slots) {
                perror("realloc");
                exit(1);
            }
            for (int i = slot_cap; i < new_cap; i++)
                new_slots[i] = NULL;
            slots = new_slots;
            slot_cap = new_cap;
        }
        if (index + 1 > slot_count)
            slot_count = index + 1;

        Node* val = parse_expr(ctx);
        if (is_union) {
            Member* target_member = designated_member;
            if (!target_member) {
                target_member = target_type->members;
            }
            if (target_member) {
                Node* cast = new_node(ND_CAST, val, NULL);
                cast->type = target_member->type;
                val = cast;
            }
        }

        slots[index] = val;
        next_index = index + 1;
        if (!has_designator)
            next_index = index + 1;
    }

    int final_count = slot_count;
    if (fixed_count > final_count)
        final_count = fixed_count;

    Node head;
    head.next = NULL;
    Node* cur = &head;
    for (int i = 0; i < final_count; i++) {
        Node* n = NULL;
        if (i < slot_cap)
            n = slots[i];
        if (!n)
            n = new_zero_init_node();
        cur->next = n;
        cur = n;
        cur->next = NULL;
    }
    free(slots);

    node->lhs = head.next; // Use lhs for list body
    return node;
}

static Node* parse_block_stmt(Context* ctx) {
    enter_scope(ctx);
    Node* head = calloc(1, sizeof(Node));
    Node* cur = head;
    while (!consume(ctx, "}")) {
        if (at_eof(ctx)) {
            fprintf(stderr, "Unexpected EOF while parsing block\n");
            exit(1);
        }
        cur->next = parse_stmt(ctx);
        cur = cur->next;
    }
    leave_scope(ctx);
    return new_node(ND_BLOCK, head->next, NULL);
}

void parse_program(Context* ctx) {
    int i = 0;
    while (!at_eof(ctx)) {
        // Check for extern declaration
        bool is_extern = consume(ctx, "extern");

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
            reset_scope(ctx);

            // Parse parameters
            Node* func_params = NULL;
            bool is_vararg = false;
            if (!consume(ctx, ")")) {
                func_params = parse_params(ctx, &is_vararg);
                expect(ctx, ")");
            }

            if (consume(ctx, ";")) {
                // Prototype (extern or regular)
                Node* proto_node = new_node(ND_FUNCTION, NULL, NULL);
                proto_node->tok = tok;
                proto_node->type = ty;
                proto_node->rhs = func_params;
                proto_node->is_extern = is_extern;
                proto_node->is_vararg = is_vararg;
                ctx->code[i++] = proto_node;
                continue;
            }

            // extern function must be a prototype
            if (is_extern) {
                fprintf(stderr, "extern function must be a prototype\n");
                exit(1);
            }

            expect(ctx, "{");

            // Create function node with return type
            Node* func_node = new_node(ND_FUNCTION, NULL, NULL);
            func_node->tok = tok;
            func_node->type = ty;
            func_node->rhs = func_params;

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
            func_node->lhs = head;
            func_node->locals = ctx->locals;

            ctx->code[i++] = func_node;
        } else {
            // This is a global variable
            // Put back the "[" or ";" token
            // Since we've already consumed the ident, we need to check for "["
            // or ";"
            if (consume(ctx, "[")) {
                int size = 0;
                if (!consume(ctx, "]")) {
                    size = expect_const_int(ctx);
                    expect(ctx, "]");
                }
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
            Node* gvar_node = new_node(ND_GVAR, NULL, NULL);
            gvar_node->tok = tok;
            gvar_node->type = ty;
            gvar_node->is_extern = is_extern;

            // extern declarations cannot have initializers
            if (!is_extern && consume(ctx, "=")) {
                int array_size_before = gvar_node->type->array_size;
                // Check if next token is '{'
                Token* t = ctx->current_token;
                if (t->kind == TK_RESERVED && t->len == 1 && t->str[0] == '{') {
                    gvar_node->init = parse_initializer(ctx, ty);
                    gvar_node->init->type = ty; // Set type for codegen
                } else {
                    gvar_node->init = parse_expr(ctx);
                }

                if (array_size_before == 0 && gvar_node->type->ty == PTR &&
                    gvar_node->init && gvar_node->init->kind == ND_INIT &&
                    gvar_node->init->lhs) {
                    int count = 0;
                    for (Node* init_node = gvar_node->init->lhs; init_node;
                         init_node = init_node->next) {
                        count++;
                    }
                    gvar_node->type->array_size = count;
                }
            }
            expect(ctx, ";");

            ctx->code[i++] = gvar_node;
            continue; // Ensure we move to next token!
        }
    }
    ctx->node_count = i;
}

Node* parse_declaration(Context* ctx, Type* ty) {
    Token* tok = consume_ident(ctx);
    bool is_func_ptr_decl = false;
    if (!tok && consume(ctx, "(")) {
        if (!consume(ctx, "*")) {
            fprintf(stderr, "Expected '*' in parenthesized declarator\n");
            exit(1);
        }
        tok = consume_ident(ctx);
        if (!tok) {
            fprintf(stderr,
                    "Expected variable name in function pointer declarator\n");
            exit(1);
        }
        expect(ctx, ")");
        expect(ctx, "(");
        if (!consume(ctx, ")")) {
            while (1) {
                Type* pty = try_parse_type(ctx);
                if (!pty) {
                    fprintf(stderr, "Expected parameter type in function "
                                    "pointer declarator\n");
                    exit(1);
                }
                consume_ident(ctx);
                if (consume(ctx, ")"))
                    break;
                expect(ctx, ",");
            }
        }
        ty = new_type_ptr(ty);
        is_func_ptr_decl = true;
    }
    if (!tok) {
        fprintf(stderr, "Expected variable name after type\n");
        exit(1);
    }

    Node* vla_size = NULL;
    // Check for array definitions (e.g., int a[10], char x[3], int y[], int
    // vla[n])
    if (!is_func_ptr_decl && consume(ctx, "[")) {
        if (!consume(ctx, "]")) {
            vla_size = parse_expr(ctx);
            expect(ctx, "]");
            if (vla_size && vla_size->kind == ND_NUM) {
                int size = vla_size->val;
                ty = new_type_array(ty, size);
                vla_size = NULL;
            } else {
                // Runtime-sized local array is represented as pointer variable.
                ty = new_type_ptr(ty);
            }
        } else {
            ty = new_type_array(ty, 0);
        }
    }

    // Add variable to locals
    LVar* lvar = add_lvar(ctx, tok, ty);
    Node* node = new_node(ND_DECL, NULL, NULL);
    node->tok = tok;
    node->val = lvar->offset;
    node->type = ty;
    if (vla_size) {
        node->is_vla = true;
        node->rhs = vla_size;
        if (lvar->offset >= 0 && lvar->offset < MAX_NODES) {
            ctx->vla_size_exprs[lvar->offset] = clone_ast(vla_size);
        }
    }

    if (consume(ctx, "=")) {
        if (node->is_vla) {
            fprintf(stderr, "VLA initializer is not supported\n");
            exit(1);
        }
        if (ctx->current_token->kind == TK_RESERVED &&
            ctx->current_token->len == 1 && ctx->current_token->str[0] == '{') {
            node->init = parse_initializer(ctx, ty);
            // If array size was 0, count elements
            if (node->type->ty == PTR && node->type->array_size == 0) {
                int count = 0;
                for (Node* init_node = node->init->lhs; init_node;
                     init_node = init_node->next)
                    count++;
                node->type->array_size = count;
            }
        } else {
            node->init = parse_expr(ctx);
        }
    }
    expect(ctx, ";");

    return node;
}

Node* parse_stmt(Context* ctx) {
    Node* stmt_node;

    // label: statement
    if (ctx->current_token->kind == TK_IDENT && ctx->current_token->next &&
        ctx->current_token->next->kind == TK_RESERVED &&
        ctx->current_token->next->len == 1 &&
        ctx->current_token->next->str[0] == ':') {
        Token* label_tok = ctx->current_token;
        ctx->current_token = ctx->current_token->next->next; // consume ident:
        Node* body = parse_stmt(ctx);
        Node* label_node = new_node(ND_LABEL, body, NULL);
        label_node->tok = label_tok;
        return label_node;
    }

    // Check for type declaration: e.g. int x; struct { ... } s;
    {
        Type* ty = try_parse_type(ctx);
        if (ty) {
            return parse_declaration(ctx, ty);
        }
    }

    if (consume(ctx, ";")) {
        return new_node(ND_BLOCK, NULL, NULL);
    }

    if (consume(ctx, "return")) {
        Node* retval = NULL;
        Token* t = ctx->current_token;
        if (!(t->kind == TK_RESERVED && t->len == 1 && t->str[0] == ';')) {
            retval = convert_array_to_ptr(parse_expr(ctx));
        }
        stmt_node = new_node(ND_RETURN, retval, NULL);
    } else if (consume(ctx, "if")) {
        expect(ctx, "(");
        Node* cond = convert_array_to_ptr(parse_expr(ctx));
        expect(ctx, ")");
        Node* then = parse_stmt(ctx);
        Node* els = NULL;
        if (consume(ctx, "else")) {
            els = parse_stmt(ctx);
        }
        Node* if_node = new_node(ND_IF, then, els);
        if_node->cond = cond;
        return if_node;
    } else if (consume(ctx, "while")) {
        expect(ctx, "(");
        Node* cond = convert_array_to_ptr(parse_expr(ctx));
        expect(ctx, ")");
        Node* body = parse_stmt(ctx);
        Node* while_node = new_node(ND_WHILE, body, NULL);
        while_node->cond = cond;
        return while_node;
    } else if (consume(ctx, "do")) {
        Node* body = parse_stmt(ctx);
        expect(ctx, "while");
        expect(ctx, "(");
        Node* cond = convert_array_to_ptr(parse_expr(ctx));
        expect(ctx, ")");
        expect(ctx, ";");
        Node* do_while_node = new_node(ND_WHILE, body, NULL);
        do_while_node->cond = cond;
        do_while_node->is_do_while = true;
        return do_while_node;
    } else if (consume(ctx, "for")) {
        expect(ctx, "(");
        Node* for_node = new_node(ND_FOR, NULL, NULL);
        enter_scope(ctx);

        Type* ty = try_parse_type(ctx);
        if (ty) {
            for_node->init = parse_declaration(ctx, ty);
        } else {
            if (!consume(ctx, ";")) {
                for_node->init = convert_array_to_ptr(parse_expr(ctx));
                expect(ctx, ";");
            }
        }

        if (!consume(ctx, ";")) {
            for_node->cond = convert_array_to_ptr(parse_expr(ctx));
            expect(ctx, ";");
        }
        if (!consume(ctx, ")")) {
            for_node->rhs = convert_array_to_ptr(parse_expr(ctx));
            expect(ctx, ")");
        }
        for_node->lhs = parse_stmt(ctx);
        leave_scope(ctx);
        return for_node;
    } else if (consume(ctx, "switch")) {
        expect(ctx, "(");
        Node* cond = convert_array_to_ptr(parse_expr(ctx));
        expect(ctx, ")");

        Node* switch_node = new_node(ND_SWITCH, NULL, NULL);
        switch_node->cond = cond;

        Node* old_switch = ctx->current_switch;
        ctx->current_switch = switch_node;

        switch_node->lhs = parse_stmt(ctx);

        ctx->current_switch = old_switch;
        return switch_node;
    } else if (consume(ctx, "case")) {
        if (!ctx->current_switch) {
            fprintf(stderr, "case outside switch\n");
            exit(1);
        }
        int val = expect_const_int(ctx);
        expect(ctx, ":");

        Node* case_node = new_node(ND_CASE, NULL, NULL);
        case_node->val = val;
        case_node->case_val = val;
        case_node->next_case = ctx->current_switch->cases;
        ctx->current_switch->cases = case_node;
        return case_node;
    } else if (consume(ctx, "default")) {
        if (!ctx->current_switch) {
            fprintf(stderr, "default outside switch\n");
            exit(1);
        }
        expect(ctx, ":");
        Node* default_node = new_node(ND_CASE, NULL, NULL);
        default_node->is_default = true;
        default_node->next_case = ctx->current_switch->cases;
        ctx->current_switch->cases = default_node;
        return default_node;
    } else if (consume(ctx, "break")) {
        expect(ctx, ";");
        return new_node(ND_BREAK, NULL, NULL);
    } else if (consume(ctx, "continue")) {
        expect(ctx, ";");
        return new_node(ND_CONTINUE, NULL, NULL);
    } else if (consume(ctx, "goto")) {
        Token* label_tok = consume_ident(ctx);
        if (!label_tok) {
            fprintf(stderr, "Expected label name after goto\n");
            exit(1);
        }
        expect(ctx, ";");
        Node* n = new_node(ND_GOTO, NULL, NULL);
        n->tok = label_tok;
        return n;
    } else if (consume(ctx, "{")) {
        return parse_block_stmt(ctx);
    } else {
        stmt_node = parse_expr(ctx);
    }
    expect(ctx, ";");

    return stmt_node;
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
    Node* node = parse_bit_or(ctx);
    for (;;) {
        if (consume(ctx, "&&")) {
            Node* rhs = parse_bit_or(ctx);
            node = new_node(ND_LOGAND, node, rhs);
            node->type = new_type_int();
        } else {
            return node;
        }
    }
}

static Node* parse_conditional(Context* ctx) {
    Node* node = parse_logor(ctx);
    if (consume(ctx, "?")) {
        Node* ternary = new_node(ND_COND, NULL, NULL);
        ternary->cond = node;
        ternary->lhs = parse_expr(ctx);
        expect(ctx, ":");
        ternary->rhs = parse_conditional(ctx);
        // Type inference for ternary
        ternary->type = get_common_type(ternary->lhs->type, ternary->rhs->type);
        return ternary;
    }
    return node;
}

Node* parse_assign(Context* ctx) {
    Node* node = parse_conditional(ctx);
    if (consume(ctx, "=")) {
        node =
            new_node(ND_ASSIGN, node, convert_array_to_ptr(parse_assign(ctx)));
    } else if (consume(ctx, "+=")) {
        Node* lhs = node;
        Node* lhs_copy = clone_ast(lhs);
        node = new_node(ND_ASSIGN, lhs,
                        new_node(ND_ADD, lhs_copy,
                                 convert_array_to_ptr(parse_assign(ctx))));
    } else if (consume(ctx, "-=")) {
        Node* lhs = node;
        Node* lhs_copy = clone_ast(lhs);
        node = new_node(ND_ASSIGN, lhs,
                        new_node(ND_SUB, lhs_copy,
                                 convert_array_to_ptr(parse_assign(ctx))));
    } else if (consume(ctx, "*=")) {
        Node* lhs = node;
        Node* lhs_copy = clone_ast(lhs);
        node = new_node(ND_ASSIGN, lhs,
                        new_node(ND_MUL, lhs_copy,
                                 convert_array_to_ptr(parse_assign(ctx))));
    } else if (consume(ctx, "/=")) {
        Node* lhs = node;
        Node* lhs_copy = clone_ast(lhs);
        node = new_node(ND_ASSIGN, lhs,
                        new_node(ND_DIV, lhs_copy,
                                 convert_array_to_ptr(parse_assign(ctx))));
    } else if (consume(ctx, "&=")) {
        Node* lhs = node;
        Node* lhs_copy = clone_ast(lhs);
        node = new_node(ND_ASSIGN, lhs,
                        new_node(ND_BITAND, lhs_copy,
                                 convert_array_to_ptr(parse_assign(ctx))));
    } else if (consume(ctx, "|=")) {
        Node* lhs = node;
        Node* lhs_copy = clone_ast(lhs);
        node = new_node(ND_ASSIGN, lhs,
                        new_node(ND_BITOR, lhs_copy,
                                 convert_array_to_ptr(parse_assign(ctx))));
    } else if (consume(ctx, "^=")) {
        Node* lhs = node;
        Node* lhs_copy = clone_ast(lhs);
        node = new_node(ND_ASSIGN, lhs,
                        new_node(ND_BITXOR, lhs_copy,
                                 convert_array_to_ptr(parse_assign(ctx))));
    } else if (consume(ctx, "<<=")) {
        Node* lhs = node;
        Node* lhs_copy = clone_ast(lhs);
        node = new_node(ND_ASSIGN, lhs,
                        new_node(ND_SHL, lhs_copy,
                                 convert_array_to_ptr(parse_assign(ctx))));
    } else if (consume(ctx, ">>=")) {
        Node* lhs = node;
        Node* lhs_copy = clone_ast(lhs);
        node = new_node(ND_ASSIGN, lhs,
                        new_node(ND_SHR, lhs_copy,
                                 convert_array_to_ptr(parse_assign(ctx))));
    }

    if (node && node->kind == ND_ASSIGN) {
        if (node->lhs) {
            node->type = node->lhs->type;
        }
    }
    return node;
}

static Node* parse_bit_or(Context* ctx) {
    Node* node = parse_bit_xor(ctx);
    while (consume(ctx, "|")) {
        Node* lhs = convert_array_to_ptr(node);
        Node* rhs = convert_array_to_ptr(parse_bit_xor(ctx));
        node = new_node(ND_BITOR, lhs, rhs);
        node->type = get_common_type(lhs->type, rhs->type);
    }
    return node;
}

static Node* parse_bit_xor(Context* ctx) {
    Node* node = parse_bit_and(ctx);
    while (consume(ctx, "^")) {
        Node* lhs = convert_array_to_ptr(node);
        Node* rhs = convert_array_to_ptr(parse_bit_and(ctx));
        node = new_node(ND_BITXOR, lhs, rhs);
        node->type = get_common_type(lhs->type, rhs->type);
    }
    return node;
}

static Node* parse_bit_and(Context* ctx) {
    Node* node = parse_equality(ctx);
    while (consume(ctx, "&")) {
        Node* lhs = convert_array_to_ptr(node);
        Node* rhs = convert_array_to_ptr(parse_equality(ctx));
        node = new_node(ND_BITAND, lhs, rhs);
        node->type = get_common_type(lhs->type, rhs->type);
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
    Node* node = parse_shift(ctx);
    while (1) {
        if (consume(ctx, "<=")) {
            node = new_node(ND_LE, convert_array_to_ptr(node),
                            convert_array_to_ptr(parse_shift(ctx)));
            node->type = new_type_int();
        } else if (consume(ctx, ">=")) {
            node = new_node(ND_GE, convert_array_to_ptr(node),
                            convert_array_to_ptr(parse_shift(ctx)));
            node->type = new_type_int();
        } else if (consume(ctx, "<")) {
            node = new_node(ND_LT, convert_array_to_ptr(node),
                            convert_array_to_ptr(parse_shift(ctx)));
            node->type = new_type_int();
        } else if (consume(ctx, ">")) {
            node = new_node(ND_GT, convert_array_to_ptr(node),
                            convert_array_to_ptr(parse_shift(ctx)));
            node->type = new_type_int();
        } else {
            return node;
        }
    }
}

static Node* parse_shift(Context* ctx) {
    Node* node = parse_add(ctx);
    for (;;) {
        if (consume(ctx, "<<")) {
            Node* lhs = convert_array_to_ptr(node);
            Node* rhs = convert_array_to_ptr(parse_add(ctx));
            node = new_node(ND_SHL, lhs, rhs);
            node->type = lhs->type;
        } else if (consume(ctx, ">>")) {
            Node* lhs = convert_array_to_ptr(node);
            Node* rhs = convert_array_to_ptr(parse_add(ctx));
            node = new_node(ND_SHR, lhs, rhs);
            node->type = lhs->type;
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
                newNode->type = get_common_type(node->type, rhs->type);
            }
            node = newNode;
        } else if (consume(ctx, "-")) {
            Node* rhs = convert_array_to_ptr(parse_mul(ctx));
            node = convert_array_to_ptr(node);
            Node* newNode = new_node(ND_SUB, node, rhs);
            if (node->type && node->type->ty == PTR) {
                newNode->type = node->type;
            } else {
                newNode->type = get_common_type(node->type, rhs->type);
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
            Node* lhs = convert_array_to_ptr(node);
            Node* rhs = convert_array_to_ptr(parse_unary(ctx));
            node = new_node(ND_MUL, lhs, rhs);
            node->type = get_common_type(lhs->type, rhs->type);
        } else if (consume(ctx, "/")) {
            Node* lhs = convert_array_to_ptr(node);
            Node* rhs = convert_array_to_ptr(parse_unary(ctx));
            node = new_node(ND_DIV, lhs, rhs);
            node->type = get_common_type(lhs->type, rhs->type);
        } else if (consume(ctx, "%")) {
            Node* lhs = convert_array_to_ptr(node);
            Node* rhs = convert_array_to_ptr(parse_unary(ctx));
            node = new_node(ND_MOD, lhs, rhs);
            node->type = get_common_type(lhs->type, rhs->type);
        } else {
            return node;
        }
    }
}

static int type_align(Type* ty) {
    if (ty == NULL)
        return 4;
    if (ty->ty == CHAR)
        return 1;
    if (ty->ty == INT)
        return 4;
    if (ty->ty == FLOAT)
        return 4;
    if (ty->ty == DOUBLE)
        return 8;
    if (ty->ty == LONG)
        return 8;
    if (ty->ty == PTR)
        return 8;
    if (ty->ty == STRUCT) {
        int max_align = 1;
        for (Member* m = ty->members; m; m = m->next) {
            int a = type_align(m->type);
            if (a > max_align)
                max_align = a;
        }
        return max_align;
    }
    if (ty->ty == UNION) {
        int max_align = 1;
        for (Member* m = ty->members; m; m = m->next) {
            int a = type_align(m->type);
            if (a > max_align)
                max_align = a;
        }
        return max_align;
    }
    if (ty->array_size > 0)
        return type_align(ty->ptr_to);
    return 4;
}

static int type_size(Type* ty) {
    if (ty == NULL)
        return 4;
    if (ty->array_size > 0)
        return type_size(ty->ptr_to) * ty->array_size;

    if (ty->ty == CHAR)
        return 1;
    if (ty->ty == INT)
        return 4;
    if (ty->ty == FLOAT)
        return 4;
    if (ty->ty == DOUBLE)
        return 8;
    if (ty->ty == LONG)
        return 8;
    if (ty->ty == PTR)
        return 8;
    if (ty->ty == STRUCT) {
        int size = 0;
        int max_align = 1;
        for (Member* m = ty->members; m; m = m->next) {
            int a = type_align(m->type);
            if (a > max_align)
                max_align = a;
            // Align current size to member alignment
            size = ((size + a - 1) / a) * a;
            size += type_size(m->type);
        }
        // Final size is multiple of max_align
        size = ((size + max_align - 1) / max_align) * max_align;
        return size;
    }
    if (ty->ty == UNION) {
        int max_size = 0;
        int max_align = 1;
        for (Member* m = ty->members; m; m = m->next) {
            int s = type_size(m->type);
            int a = type_align(m->type);
            if (s > max_size)
                max_size = s;
            if (a > max_align)
                max_align = a;
        }
        max_size = ((max_size + max_align - 1) / max_align) * max_align;
        return max_size;
    }
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
        if (consume(ctx, "(")) {
            Node* node_args = NULL;
            if (!consume(ctx, ")")) {
                node_args = parse_args(ctx);
                expect(ctx, ")");
            }
            Node* call = new_node(ND_CALL, node_args, node);
            call->type = new_type_int();
            node = call;
            continue;
        }

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
            if (!node->type ||
                (node->type->ty != STRUCT && node->type->ty != UNION)) {
                fprintf(stderr, "Left side of '.' is not a struct/union\n");
                exit(1);
            }
            Member* m = find_member(node->type, tok);
            if (!m) {
                fprintf(stderr, "Member not found\n");
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
                (node->type->ptr_to->ty != STRUCT &&
                 node->type->ptr_to->ty != UNION)) {
                fprintf(stderr,
                        "Left side of '->' is not a pointer to struct/union\n");
                exit(1);
            }
            // p->m is (*p).m
            Node* deref = new_node(ND_DEREF, node, NULL);
            deref->type = node->type->ptr_to;

            Member* m = find_member(deref->type, tok);
            if (!m) {
                fprintf(stderr, "Member not found\n");
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

static char* type_keywords[22] = {
    "int",      "char",     "void",     "long",    "bool",  "_Bool",
    "size_t",   "enum",     "struct",   "union",   "const", "static",
    "extern",   "signed",   "unsigned", "double",  "float", "inline",
    "restrict", "volatile", "register", "_Complex"};

static bool is_type(Context* ctx) {
    Token* tok = ctx->current_token;
    if (tok->kind == TK_RESERVED) {
        int num_type_kw = 22;
        for (int i = 0; i < num_type_kw; i++) {
            if ((size_t)tok->len == strlen(type_keywords[i]) &&
                strncmp(tok->str, type_keywords[i], tok->len) == 0)
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
            while (consume(ctx, "[")) {
                int size = 0;
                if (!consume(ctx, "]")) {
                    size = expect_const_int(ctx);
                    expect(ctx, "]");
                }
                ty = new_type_array(ty, (size_t)size);
            }
            expect(ctx, ")");
            if (ctx->current_token->kind == TK_RESERVED &&
                ctx->current_token->len == 1 &&
                ctx->current_token->str[0] == '{') {
                Node* init = parse_initializer(ctx, ty);
                if (ty->ty == PTR && ty->array_size == 0 && init &&
                    init->kind == ND_INIT) {
                    int cnt = 0;
                    for (Node* n = init->lhs; n; n = n->next)
                        cnt++;
                    ty->array_size = (size_t)cnt;
                }
                if ((ty->ty == STRUCT || ty->ty == UNION) ||
                    (ty->ty == PTR && ty->array_size > 0)) {
                    Node* lit = new_node(ND_COMPOUND, NULL, NULL);
                    lit->type = ty;
                    lit->init = init;
                    return lit;
                }
                Node* elem = init->lhs;
                if (!elem) {
                    elem = new_node_num(0);
                }
                Node* cast = new_node(ND_CAST, elem, NULL);
                cast->type = ty;
                return cast;
            }
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
            return parse_sizeof_expr_node(ctx, node);
        } else {
            Node* node = parse_unary(ctx);
            return parse_sizeof_expr_node(ctx, node);
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
        Type* ptr_to = new_type_int();
        if (operand->type) {
            ptr_to = operand->type;
        }
        node->type = new_type_ptr(ptr_to);
        return node;
    }
    if (consume(ctx, "!")) {
        Node* operand = convert_array_to_ptr(parse_unary(ctx));
        Node* node = new_node(ND_NOT, operand, NULL);
        node->type = new_type_int();
        return node;
    }
    if (consume(ctx, "~")) {
        Node* operand = convert_array_to_ptr(parse_unary(ctx));
        Node* node = new_node(ND_BITNOT, operand, NULL);
        node->type = operand->type;
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
            return parse_sizeof_expr_node(ctx, node);
        } else {
            Node* node = parse_unary_no_array_conv(ctx);
            return parse_sizeof_expr_node(ctx, node);
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
        Type* ptr_to = new_type_int();
        if (operand->type) {
            ptr_to = operand->type;
        }
        node->type = new_type_ptr(ptr_to);
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
        int total_len = 0;
        Token* t = ctx->current_token;
        while (t && t->kind == TK_STR) {
            total_len += t->len;
            t = t->next;
        }

        char* merged = calloc((size_t)total_len + 1, 1);
        if (!merged) {
            perror("calloc");
            exit(1);
        }
        int off = 0;
        while (ctx->current_token->kind == TK_STR) {
            Token* tok = ctx->current_token;
            memcpy(merged + off, tok->str, (size_t)tok->len);
            off += tok->len;
            ctx->current_token = ctx->current_token->next;
        }

        // Add string to context's string vector
        int idx = ctx->string_count++;
        ctx->strings[idx] = merged;
        ctx->string_lens[idx] = total_len;
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
    if (!tok && !at_eof(ctx)) {
    }

    if (tok) {
        bool has_lvar = find_lvar(ctx, tok) != NULL;
        bool has_gvar = find_gvar(ctx, tok) != NULL;
        if (!has_lvar && !has_gvar && consume(ctx, "(")) {
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
        Token* num_tok = ctx->current_token;
        Node* num_node;
        if (num_tok->is_float) {
            bool is_single = false;
            if (num_tok->len > 0 && (num_tok->str[num_tok->len - 1] == 'f' ||
                                     num_tok->str[num_tok->len - 1] == 'F')) {
                is_single = true;
            }
            num_node =
                new_node_fnum(num_tok->fval,
                              is_single ? new_type_float() : new_type_double());
            ctx->current_token = ctx->current_token->next;
        } else {
            int ival = (num_tok->uval <= (unsigned long long)LLVM7_INT_MAX)
                           ? (int)num_tok->uval
                           : LLVM7_INT_MAX;
            num_node = new_node_num(ival);
            num_node->uval = num_tok->uval;
            if (num_tok->is_unsigned) {
                num_node->type->is_unsigned = true;
            }
            ctx->current_token = ctx->current_token->next;
        }

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
    return new_node_num(expect_const_int(ctx));
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
