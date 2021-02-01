#include "include/opentac.h"
#include "grammar.tab.h"

extern FILE *yyin;
extern OpentacBuilder *opentac_b;

#define DEFAULT_BUILDER_CAP ((size_t) 32)
#define DEFAULT_DEBUG_CAP ((size_t) 512)
#define DEFAULT_FN_CAP ((size_t) 32)
#define DEFAULT_NAME_TABLE_CAP ((size_t) 32)
#define DEFAULT_PARAMS_CAP ((size_t) 4)

OpentacBuilder *opentac_parse(FILE *file) {
    opentac_assert(file);
    
    yyin = file;
    opentac_b = opentac_builderp();
    if (yyparse()) {
        return NULL;
    }
    return opentac_b;
}

void opentac_builder(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    opentac_builder_with_cap(builder, DEFAULT_BUILDER_CAP);
}

void opentac_builder_with_cap(OpentacBuilder *builder, size_t cap) {
    opentac_assert(builder);
    opentac_assert(cap > 0);
    
    builder->len = 0;
    builder->cap = cap;
    builder->items = malloc(cap * sizeof(OpentacItem *));
    builder->current = builder->items;
    builder->typeset.len = 0;
    builder->typeset.cap = cap;
    builder->typeset.types = malloc(cap * sizeof(OpentacType *));
}

OpentacBuilder *opentac_builderp() {
    return opentac_builderp_with_cap(DEFAULT_BUILDER_CAP);
}

OpentacBuilder *opentac_builderp_with_cap(size_t cap) {
    OpentacBuilder *builder = malloc(sizeof(OpentacBuilder));
    opentac_builder_with_cap(builder, cap);
    return builder;
}

static void opentac_grow_builder(OpentacBuilder *builder, size_t newcap) {
    opentac_assert(builder);
    opentac_assert(newcap >= builder->cap);
    
    builder->cap = newcap;
    builder->items = realloc(builder->items, builder->cap * sizeof(OpentacItem));
    builder->current = builder->items + builder->len;
}

static void opentac_grow_fn(OpentacBuilder *builder, size_t newcap) {
    opentac_assert(builder);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    
    opentac_assert(newcap >= fn->cap);
    
    fn->cap = newcap;
    fn->stmts = realloc(fn->stmts, fn->cap * sizeof(OpentacStmt));
    fn->current = fn->stmts + fn->len;
}

static void opentac_grow_name_table(OpentacFnBuilder *fn, size_t newcap) {
    opentac_assert(fn);
    opentac_assert(newcap >= fn->cap);
    
    fn->name_table.cap = newcap;
    fn->name_table.entries = realloc(fn->name_table.entries, fn->name_table.cap * sizeof(struct OpentacEntry));
}

static void opentac_grow_params(OpentacFnBuilder *fn, size_t newcap) {
    opentac_assert(fn);
    opentac_assert(newcap >= fn->params.cap);
    
    fn->params.cap = newcap;
    fn->params.params = realloc(fn->params.params, fn->params.cap * sizeof(OpentacRefType));
}

static void opentac_grow_typeset(OpentacBuilder *builder, size_t newcap) {
    opentac_assert(builder);
    opentac_assert(newcap >= builder->typeset.cap);
    
    builder->typeset.cap = newcap;
    builder->typeset.types = realloc(builder->typeset.types, builder->typeset.cap * sizeof(OpentacType *));
}

void opentac_build_decl(OpentacBuilder *builder, OpentacString *name, OpentacType *type) {
    opentac_assert(builder);
    
    if ((size_t) (builder->current - builder->items) > builder->cap) {
        opentac_grow_builder(builder, builder->cap * 2);
    }
    
    OpentacItem *item = malloc(sizeof(OpentacItem));
    item->tag = OPENTAC_ITEM_DECL;
    item->decl.name = name;
    item->decl.type = type;
    *builder->current = item;
    ++builder->current;
    ++builder->len;
}

void opentac_build_function(OpentacBuilder *builder, OpentacString *name) {
    opentac_assert(builder);
    
    if ((size_t) (builder->current - builder->items) > builder->cap) {
        opentac_grow_builder(builder, builder->cap * 2);
    }

    size_t cap = DEFAULT_FN_CAP;
    OpentacItem *item = malloc(sizeof(OpentacItem));
    item->tag = OPENTAC_ITEM_FN;
    item->fn.name = name;
    item->fn.param = 0;
    item->fn.reg = 0;
    item->fn.len = 0;
    item->fn.cap = cap;
    item->fn.stmts = malloc(cap * sizeof(OpentacStmt));
    item->fn.current = item->fn.stmts;
    
    cap = DEFAULT_NAME_TABLE_CAP;
    item->fn.name_table.len = 0;
    item->fn.name_table.cap = cap;
    item->fn.name_table.entries = malloc(cap * sizeof(struct OpentacEntry));
    
    cap = DEFAULT_PARAMS_CAP;
    item->fn.params.len = 0;
    item->fn.params.cap = cap;
    item->fn.params.params = malloc(cap * sizeof(OpentacRefType));
    
    item->fn.debug.len = 0;
    item->fn.debug.cap = DEFAULT_DEBUG_CAP;
    item->fn.debug.data = malloc(item->fn.debug.cap);
    
    *builder->current = item;
}

void opentac_finish_function(OpentacBuilder *builder) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    ++builder->current;
    ++builder->len;
}

OpentacItem *opentac_item_ptr(OpentacBuilder *builder) {
    return *builder->current;
}

void opentac_set_item(OpentacBuilder *builder, OpentacItem *item) {
    *builder->current = item;
}

void opentac_build_function_param(OpentacBuilder *builder, OpentacString *name, OpentacRefType *type) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    uint32_t param = --fn->param;
    opentac_fn_bind_int(builder, name, param);
    
    if (fn->params.len == fn->params.cap) {
        opentac_grow_params(fn, fn->params.cap * 2);
    }
    
    fn->params.params[fn->params.len++] = *type;
}

void opentac_builder_insert(OpentacBuilder *builder, size_t index) {
    opentac_assert(builder);
    
    size_t remaining_len = builder->len - index;
    memmove(builder->items + index + 1, builder->items + index, sizeof(OpentacItem) * remaining_len);
    opentac_builder_goto(builder, index);
}

void opentac_builder_goto(OpentacBuilder *builder, size_t index) {
    opentac_assert(builder);
    
    builder->current = builder->items + index;
}

void opentac_builder_goto_end(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    opentac_builder_goto(builder, builder->len);
}

size_t opentac_stmt_offset(OpentacBuilder *builder, OpentacStmt *stmt) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    opentac_assert(stmt);

    OpentacFnBuilder *fn = &(*builder->current)->fn;

    return stmt - fn->stmts;
}

OpentacStmt *opentac_stmt_ptr(OpentacBuilder *builder) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);

    OpentacFnBuilder *fn = &(*builder->current)->fn;

    return fn->current;
}

OpentacStmt *opentac_stmt_at(OpentacBuilder *builder, size_t offset) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);

    OpentacFnBuilder *fn = &(*builder->current)->fn;

    return fn->stmts + offset;
}

OpentacValue opentac_build_alloca(OpentacBuilder *builder, OpentacType **t, uint64_t size, uint64_t align) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if (fn->len == fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    OpentacRegister target = fn->reg++;

    fn->current->tag.opcode = OPENTAC_OP_ALLOCA;
    fn->current->tag.left = OPENTAC_VAL_UI64;
    fn->current->tag.right = OPENTAC_VAL_UI64;
    fn->current->type = t - builder->typeset.types;
    fn->current->left.ui64val = size;
    fn->current->right.ui64val = align;
    fn->current->target = target;
    ++fn->len;
    ++fn->current;

    OpentacValue result;
    result.tag = OPENTAC_VAL_REG;
    result.val.regval = target;

    return result;
}

OpentacValue opentac_build_binary(OpentacBuilder *builder, int op, OpentacType **t, OpentacValue left, OpentacValue right) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if (fn->len == fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    OpentacRegister target = fn->reg++;

    fn->current->tag.opcode = op;
    fn->current->tag.left = left.tag;
    fn->current->tag.right = right.tag;
    fn->current->type = t - builder->typeset.types;
    fn->current->left = left.val;
    fn->current->right = right.val;
    fn->current->target = target;
    ++fn->len;
    ++fn->current;

    OpentacValue result;
    result.tag = OPENTAC_VAL_REG;
    result.val.regval = target;

    return result;
}

OpentacValue opentac_build_unary(OpentacBuilder *builder, int op, OpentacType **t, OpentacValue value) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if (fn->len == fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    OpentacRegister target = fn->reg++;

    fn->current->tag.opcode = op;
    fn->current->tag.left = value.tag;
    fn->current->type = t - builder->typeset.types;
    fn->current->left = value.val;
    fn->current->target = target;
    ++fn->len;
    ++fn->current;

    OpentacValue result;
    result.tag = OPENTAC_VAL_REG;
    result.val.regval = target;

    return result;
}

void opentac_build_index_assign(OpentacBuilder *builder, OpentacType **t, OpentacRegister target, OpentacValue offset, OpentacValue value) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if (fn->len == fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    fn->current->tag.opcode = OPENTAC_OP_INDEX_ASSIGN;
    fn->current->tag.left = offset.tag;
    fn->current->tag.right = value.tag;
    fn->current->type = t - builder->typeset.types;
    fn->current->left = offset.val;
    fn->current->right = value.val;
    fn->current->target = target;
    ++fn->len;
    ++fn->current;
}

OpentacValue opentac_build_assign_index(OpentacBuilder *builder, OpentacType **t, OpentacValue value, OpentacValue offset) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if (fn->len == fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    OpentacRegister target = fn->reg++;

    fn->current->tag.opcode = OPENTAC_OP_ASSIGN_INDEX;
    fn->current->tag.left = value.tag;
    fn->current->tag.right = offset.tag;
    fn->current->type = t - builder->typeset.types;
    fn->current->left = value.val;
    fn->current->right = offset.val;
    fn->current->target = target;
    ++fn->len;
    ++fn->current;

    OpentacValue result;
    result.tag = OPENTAC_VAL_REG;
    result.val.regval = target;

    return result;
}

void opentac_build_param(OpentacBuilder *builder, OpentacType **t, OpentacValue value) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if (fn->len == fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    fn->current->tag.opcode = OPENTAC_OP_PARAM;
    fn->current->tag.left = value.tag;
    fn->current->type = t - builder->typeset.types;
    fn->current->left = value.val;
    ++fn->len;
    ++fn->current;
}

OpentacValue opentac_build_call(OpentacBuilder *builder, OpentacType **t, OpentacValue func, uint64_t nparams) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if (fn->len == fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    OpentacRegister target = fn->reg++;

    fn->current->tag.opcode = OPENTAC_OP_CALL;
    fn->current->tag.left = func.tag;
    fn->current->tag.right = OPENTAC_VAL_UI64;
    fn->current->type = t - builder->typeset.types;
    fn->current->left = func.val;
    fn->current->right.ui64val = nparams;
    fn->current->target = target;
    ++fn->len;
    ++fn->current;

    OpentacValue result;
    result.tag = OPENTAC_VAL_REG;
    result.val.regval = target;

    return result;
}

void opentac_build_return(OpentacBuilder *builder, OpentacType **t, OpentacValue value) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if (fn->len == fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    fn->current->tag.opcode = OPENTAC_OP_RETURN;
    fn->current->tag.left = value.tag;
    fn->current->type = t - builder->typeset.types;
    fn->current->left = value.val;
    ++fn->len;
    ++fn->current;
}

void opentac_build_if_branch(OpentacBuilder *builder, int relop, OpentacValue left, OpentacValue right, OpentacLabel label) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    opentac_assert(relop >= OPENTAC_OP_LT && relop <= OPENTAC_OP_GE);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if (fn->len == fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    fn->current->tag.opcode = OPENTAC_OP_BRANCH | relop;
    fn->current->tag.left = left.tag;
    fn->current->tag.right = right.tag;
    fn->current->left = left.val;
    fn->current->right = right.val;
    fn->current->label = label;
    ++fn->len;
    ++fn->current;
}

void opentac_build_branch(OpentacBuilder *builder, OpentacValue value) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if (fn->len == fn->cap) {
        opentac_grow_fn(builder, fn->cap * 2);
    }
    
    fn->current->tag.opcode = OPENTAC_OP_BRANCH | OPENTAC_OP_NOP;
    fn->current->tag.left = value.tag;
    fn->current->left = value.val;
    ++fn->len;
    ++fn->current;
}

void opentac_fn_insert(OpentacBuilder *builder, size_t index) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    size_t remaining_len = fn->len - index;
    memmove(fn->stmts + index + 1, fn->stmts + index, sizeof(OpentacStmt) * remaining_len);
    opentac_fn_goto(builder, index);
}

void opentac_fn_goto(OpentacBuilder *builder, size_t index) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    fn->current = fn->stmts + index;
}

void opentac_fn_goto_end(OpentacBuilder *builder) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    size_t len = (*builder->current)->fn.len;
    opentac_fn_goto(builder, len);
}

void opentac_fn_bind_int(OpentacBuilder *builder, OpentacString *name, uint32_t val) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if (fn->name_table.len == fn->name_table.cap) {
        opentac_grow_name_table(fn, fn->name_table.cap * 2);
    }
    
    fn->name_table.entries[fn->name_table.len].key = name;
    fn->name_table.entries[fn->name_table.len++].ival = val;
}

void opentac_fn_bind_ptr(OpentacBuilder *builder, OpentacString *name, void *val) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    if (fn->name_table.len == fn->name_table.cap) {
        opentac_grow_name_table(fn, fn->name_table.cap * 2);
    }
    
    fn->name_table.entries[fn->name_table.len].key = name;
    fn->name_table.entries[fn->name_table.len++].pval = val;
}

uint32_t opentac_fn_get_int(OpentacBuilder *builder, OpentacString *name) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    for (size_t i = 0; i < fn->name_table.len; i++) {
        if (strcmp(fn->name_table.entries[i].key->data, name->data) == 0) {
            return fn->name_table.entries[i].ival;
        }
    }

    return -1;
}

void *opentac_fn_get_ptr(OpentacBuilder *builder, OpentacString *name) {
    opentac_assert(builder);
    opentac_assert((*builder->current)->tag == OPENTAC_ITEM_FN);
    
    OpentacFnBuilder *fn = &(*builder->current)->fn;
    for (size_t i = 0; i < fn->name_table.len; i++) {
        if (strcmp(fn->name_table.entries[i].key->data, name->data) == 0) {
            return fn->name_table.entries[i].pval;
        }
    }

    return NULL;
}

void opentac_debug_next(struct OpentacDebugStmt *stmt, struct OpentacDebugData *data, size_t *idx) {
    size_t i = *idx;
    stmt->opcode = ((uint8_t *) data->data)[i++];
    switch (stmt->opcode) {
    case OPENTAC_DBG_SET_FILE:
        memcpy(&stmt->arg0.u32val, ((uint32_t *) ((char *) data->data + i)), sizeof(uint32_t));
        i += sizeof(uint32_t);
        break;
    case OPENTAC_DBG_SET_DIR:
        memcpy(&stmt->arg0.u32val, ((uint32_t *) ((char *) data->data + i)), sizeof(uint32_t));
        i += sizeof(uint32_t);
        break;
    case OPENTAC_DBG_SET_COL:
        memcpy(&stmt->arg0.u16val, ((uint16_t *) ((char *) data->data + i)), sizeof(uint16_t));
        i += sizeof(uint16_t);
        break;
    case OPENTAC_DBG_SET_PC:
        memcpy(&stmt->arg0.u64val, ((uint64_t *) ((char *) data->data + i)), sizeof(uint64_t));
        i += sizeof(uint64_t);
        break;
    case OPENTAC_DBG_SET_LINE:
        memcpy(&stmt->arg0.u32val, ((uint32_t *) ((char *) data->data + i)), sizeof(uint32_t));
        i += sizeof(uint32_t);
        break;
    case OPENTAC_DBG_INC_PC:
        memcpy(&stmt->arg0.u16val, ((uint16_t *) ((char *) data->data + i)), sizeof(uint16_t));
        i += sizeof(uint16_t);
        break;
    case OPENTAC_DBG_INC_LINE:
        memcpy(&stmt->arg0.i16val, ((int16_t *) ((char *) data->data + i)), sizeof(uint16_t));
        i += sizeof(uint16_t);
        break;
    case OPENTAC_DBG_INC:
        memcpy(&stmt->arg0.i8val, ((int8_t *) ((char *) data->data + i)), sizeof(uint8_t));
        i += sizeof(uint8_t);
        memcpy(&stmt->arg1.u8val, ((uint8_t *) ((char *) data->data + i)), sizeof(uint8_t));
        i += sizeof(uint8_t);
        break;
    case OPENTAC_DBG_END:
        break;
    }
    *idx = i;
}

static void opentac_debug_opcode(OpentacBuilder *builder, uint8_t opcode, size_t len, void *args) {
    if ((*builder->current)->fn.debug.len + 1 + len > (*builder->current)->fn.debug.cap) {
        (*builder->current)->fn.debug.cap *= 2;
        (*builder->current)->fn.debug.data = realloc((*builder->current)->fn.debug.data, (*builder->current)->fn.debug.cap);
    }

    memcpy((char *) (*builder->current)->fn.debug.data + (*builder->current)->fn.debug.len, &opcode, 1);
    (*builder->current)->fn.debug.len += 1;
    memcpy((char *) (*builder->current)->fn.debug.data + (*builder->current)->fn.debug.len, args, len);
    (*builder->current)->fn.debug.len += len;
}

void opentac_debug_set_file(OpentacBuilder *builder, uint32_t file) {
    opentac_debug_opcode(builder, OPENTAC_DBG_SET_FILE, sizeof(uint32_t), &file);
}

void opentac_debug_set_dir(OpentacBuilder *builder, uint32_t dir) {
    opentac_debug_opcode(builder, OPENTAC_DBG_SET_DIR, sizeof(uint32_t), &dir);
}

void opentac_debug_set_col(OpentacBuilder *builder, uint16_t col) {
    opentac_debug_opcode(builder, OPENTAC_DBG_SET_COL, sizeof(uint16_t), &col);
}

void opentac_debug_set_pc(OpentacBuilder *builder, uint64_t pc) {
    opentac_debug_opcode(builder, OPENTAC_DBG_SET_PC, sizeof(uint64_t), &pc);
}

void opentac_debug_set_line(OpentacBuilder *builder, uint32_t line) {
    opentac_debug_opcode(builder, OPENTAC_DBG_SET_LINE, sizeof(uint32_t), &line);
}

void opentac_debug_inc_pc(OpentacBuilder *builder, uint16_t inc) {
    opentac_debug_opcode(builder, OPENTAC_DBG_INC_PC, sizeof(uint16_t), &inc);
}

void opentac_debug_inc_line(OpentacBuilder *builder, int16_t inc) {
    opentac_debug_opcode(builder, OPENTAC_DBG_INC_LINE, sizeof(uint16_t), &inc);
}

void opentac_debug_inc(OpentacBuilder *builder, int8_t inc1, uint8_t inc2) {
    uint8_t args[2] = { inc1, inc2 };
    opentac_debug_opcode(builder, OPENTAC_DBG_INC, sizeof(uint8_t [2]), args);
}

void opentac_debug_end(OpentacBuilder *builder) {
    opentac_debug_opcode(builder, OPENTAC_DBG_END, 0, NULL);
}

OpentacType **opentac_type_ptr_of(OpentacBuilder *builder, OpentacType *t) {
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType **type = &builder->typeset.types[i];
        if (*type == t) {
            return type;
        }
    }
    return NULL;
}

#define BASIC_TYPE_FN(t, s, a) \
    for (size_t i = 0; i < builder->typeset.len; i++) { \
        OpentacType **type = &builder->typeset.types[i]; \
        if ((*type)->tag == t) {                         \
            return type; \
        } \
    } \
\
    if (builder->typeset.len == builder->typeset.cap) { \
        opentac_grow_typeset(builder, builder->typeset.cap * 2); \
    } \
\
    OpentacType *type = malloc(sizeof(OpentacType)); \
    type->tag = t; \
    type->size = s; \
    type->align = a; \
    OpentacType **dest = &builder->typeset.types[builder->typeset.len++]; \
    *dest = type; \
    return dest;

OpentacType **opentac_typep_unit(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_UNIT, 0, 1)
}

OpentacType **opentac_typep_never(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_NEVER, 0, 1)
}

OpentacType **opentac_typep_bool(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_BOOL, 1, 1)
}

OpentacType **opentac_typep_i8(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_I8, 1, 1)
}

OpentacType **opentac_typep_i16(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_I16, 2, 2)
}

OpentacType **opentac_typep_i32(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_I32, 4, 4)
}

OpentacType **opentac_typep_i64(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_I64, 8, 8)
}

OpentacType **opentac_typep_ui8(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_UI8, 1, 1)
}

OpentacType **opentac_typep_ui16(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_UI16, 2, 2)
}

OpentacType **opentac_typep_ui32(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_UI32, 4, 4)
}

OpentacType **opentac_typep_ui64(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_UI64, 8, 8)
}

OpentacType **opentac_typep_f32(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_F32, 4, 4)
}

OpentacType **opentac_typep_f64(OpentacBuilder *builder) {
    opentac_assert(builder);
    
    BASIC_TYPE_FN(OPENTAC_TYPE_F64, 8, 8)
}

OpentacType **opentac_typep_named(OpentacBuilder *builder, int tag, OpentacString *name) {
    opentac_assert(builder);
    opentac_assert(name);
    opentac_assert(tag == OPENTAC_TYPE_STRUCT || tag == OPENTAC_TYPE_UNION);
    
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType **type = &builder->typeset.types[i];
        if ((*type)->tag == tag && strcmp((*type)->struc.name->data, name->data) == 0) {
            opentac_del_string(name);
            return type;
        }
    }

    if (builder->typeset.len == builder->typeset.cap) {
        opentac_grow_typeset(builder, builder->typeset.cap * 2);
    }

    OpentacType *type = malloc(sizeof(OpentacType));
    type->tag = tag;
    /* type->size = 0; */
    /* type->align = 0; */
    type->struc.name = name;
    type->struc.len = 0;
    type->struc.elems = NULL;
    OpentacType **dest = &builder->typeset.types[builder->typeset.len++];
    *dest = type;

    return dest;
}

OpentacType **opentac_typep_ptr(OpentacBuilder *builder, OpentacType *pointee) {
    opentac_assert(builder);
    opentac_assert(pointee);
    
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType **type = &builder->typeset.types[i];
        if ((*type)->tag == OPENTAC_TYPE_PTR && (*type)->ptr.pointee == pointee) {
            return type;
        }
    }

    if (builder->typeset.len == builder->typeset.cap) {
        opentac_grow_typeset(builder, builder->typeset.cap * 2);
    }

    OpentacType *type = malloc(sizeof(OpentacType));
    type->tag = OPENTAC_TYPE_PTR;
    type->size = 8;
    type->align = 8;
    type->ptr.pointee = pointee;
    OpentacType **dest = &builder->typeset.types[builder->typeset.len++];
    *dest = type;

    return dest;
}

OpentacType **opentac_typep_fn(OpentacBuilder *builder, size_t len, OpentacRefType *params, OpentacType *result) {
    opentac_assert(builder);
    opentac_assert(params);
    opentac_assert(result);
    
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType **type = &builder->typeset.types[i];
        if ((*type)->tag == OPENTAC_TYPE_FN
            && (*type)->fn.result == result
            && (*type)->fn.len == len) {
            for (size_t j = 0; j < (*type)->fn.len; j++) {
                OpentacRefType *param = &(*type)->fn.params[j];
                if (param->type != params[j].type && param->ref != params[j].ref) {
                    goto cont;
                }
            }
            free(params);
            return type;
        }
cont:
        (void) 0;
    }

    if (builder->typeset.len == builder->typeset.cap) {
        opentac_grow_typeset(builder, builder->typeset.cap * 2);
    }

    OpentacType *type = malloc(sizeof(OpentacType));
    type->tag = OPENTAC_TYPE_FN;
    type->size = 8;
    type->align = 8;
    type->fn.result = result;
    type->fn.len = len;
    type->fn.params = params;
    OpentacType **dest = &builder->typeset.types[builder->typeset.len++];
    *dest = type;

    return dest;
}

OpentacType **opentac_typep_tuple(OpentacBuilder *builder, uint64_t size, uint64_t align, size_t len, OpentacType **elems) {
    opentac_assert(builder);
    opentac_assert(elems);
    
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType **type = &builder->typeset.types[i];
        if ((*type)->tag == OPENTAC_TYPE_TUPLE && (*type)->tuple.len == len) {
            for (size_t j = 0; j < (*type)->tuple.len; j++) {
                OpentacType *elem = (*type)->tuple.elems[j];
                if (elem != elems[j]) {
                    goto cont;
                }
            }
            free(elems);
            return type;
        }
cont:
        (void) 0;
    }

    if (builder->typeset.len == builder->typeset.cap) {
        opentac_grow_typeset(builder, builder->typeset.cap * 2);
    }

    OpentacType *type = malloc(sizeof(OpentacType));
    type->tag = OPENTAC_TYPE_TUPLE;
    type->size = size;
    type->align = align;
    type->tuple.len = len;
    type->tuple.elems = elems;
    OpentacType **dest = &builder->typeset.types[builder->typeset.len++];
    *dest = type;

    return dest;
}

OpentacType **opentac_typep_struct(OpentacBuilder *builder, uint64_t size, uint64_t align, OpentacString *name, size_t len, OpentacType **elems, OpentacString **fields) {
    opentac_assert(builder);
    opentac_assert(name);
    opentac_assert(elems);
    
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType **type = &builder->typeset.types[i];
        if ((*type)->tag == OPENTAC_TYPE_STRUCT && strcmp((*type)->struc.name->data, name->data)) {
            opentac_del_string(name);
            (*type)->struc.len = len;
            (*type)->struc.elems = elems;
            return type;
        }
    }

    if (builder->typeset.len == builder->typeset.cap) {
        opentac_grow_typeset(builder, builder->typeset.cap * 2);
    }

    OpentacType *type = malloc(sizeof(OpentacType));
    type->tag = OPENTAC_TYPE_STRUCT;
    type->size = size;
    type->align = align;
    type->struc.name = name;
    type->struc.len = len;
    type->struc.elems = elems;
    type->struc.fields = fields;
    OpentacType **dest = &builder->typeset.types[builder->typeset.len++];
    *dest = type;

    return dest;
}

OpentacType **opentac_typep_union(OpentacBuilder *builder, uint64_t size, uint64_t align, OpentacString *name, size_t len, OpentacType **elems) {
    opentac_assert(builder);
    opentac_assert(name);
    opentac_assert(elems);
    
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType **type = &builder->typeset.types[i];
        if ((*type)->tag == OPENTAC_TYPE_UNION && strcmp((*type)->struc.name->data, name->data)) {
            opentac_del_string(name);
            (*type)->struc.len = len;
            (*type)->struc.elems = elems;
            return type;
        }
    }
    
    if (builder->typeset.len == builder->typeset.cap) {
        opentac_grow_typeset(builder, builder->typeset.cap * 2);
    }

    OpentacType *type = malloc(sizeof(OpentacType));
    type->tag = OPENTAC_TYPE_UNION;
    type->size = size;
    type->align = align;
    type->struc.name = name;
    type->struc.len = len;
    type->struc.elems = elems;
    OpentacType **dest = &builder->typeset.types[builder->typeset.len++];
    *dest = type;

    return dest;
}

OpentacType **opentac_typep_array(OpentacBuilder *builder, OpentacType *elem_type, uint64_t len) {
    opentac_assert(builder);
    opentac_assert(elem_type);
    
    for (size_t i = 0; i < builder->typeset.len; i++) {
        OpentacType **type = &builder->typeset.types[i];
        if ((*type)->tag == OPENTAC_TYPE_ARRAY && (*type)->array.elem_type == elem_type && (*type)->array.len == len) {
            return type;
        }
    }

    if (builder->typeset.len == builder->typeset.cap) {
        opentac_grow_typeset(builder, builder->typeset.cap * 2);
    }

    OpentacType *type = malloc(sizeof(OpentacType));
    type->tag = OPENTAC_TYPE_ARRAY;
    type->size = elem_type->size * len;
    type->align = elem_type->align;
    type->array.elem_type = elem_type;
    type->array.len = len;
    OpentacType **dest = &builder->typeset.types[builder->typeset.len++];
    *dest = type;

    return dest;
}

OpentacString *opentac_string(const char *str) {
    opentac_assert(str);
    
    size_t len = strlen(str);
    size_t cap = len + 1;
    OpentacString *string = malloc(sizeof(OpentacString) + cap);
    string->len = len;
    string->cap = cap;
    strcpy(string->data, str);
    return string;
}

void opentac_del_string(OpentacString *str) {
    free(str);
}
