#include "include/opentac.h"

static size_t opentac_fprint_decl(FILE *out, OpentacDecl *decl);
static size_t opentac_fprint_fn(FILE *out, OpentacBuilder *builder, OpentacFnBuilder *fn);
static size_t opentac_fprint_type(FILE *out, OpentacType *type);
static size_t opentac_fprint_reftype(FILE *out, OpentacRefType *type);
static size_t opentac_fprint_param(FILE *out, OpentacRefType *type, size_t i);
static size_t opentac_fprint_stmt(FILE *out, OpentacBuilder *builder, OpentacStmt *stmt);
static size_t opentac_fprint_value(FILE *out, OpentacValue *value);

size_t opentac_fprint(FILE *out, OpentacBuilder *builder) {
    size_t len = 0;
    for (size_t i = 0; i < builder->len; i++) {
        switch (builder->items[i]->tag) {
        case OPENTAC_ITEM_DECL:
            len += opentac_fprint_decl(out, &builder->items[i]->decl);
            break;
        case OPENTAC_ITEM_FN:
            len += opentac_fprint_fn(out, builder, &builder->items[i]->fn);
            len += fprintf(out, "\n");
            break;
        }
    }
    return len;
}

static size_t opentac_fprint_decl(FILE *out, OpentacDecl *decl) {
    size_t len = 0;
    len += fprintf(out, "%s: ", decl->name->data);
    len += opentac_fprint_type(out, decl->type);
    return len;
}

static size_t opentac_fprint_fn(FILE *out, OpentacBuilder *builder, OpentacFnBuilder *fn) {
    size_t len = 0;
    len += fprintf(out, "%s :: (", fn->name->data);
    for (size_t i = 0; i < fn->params.len; i++) {
        len += opentac_fprint_param(out, &fn->params.params[i], i);
        if (i < fn->params.len - 1) {
            len += fprintf(out, ", ");
        }
    }
    len += fprintf(out, ") => {\n");
    for (size_t i = 0; i < fn->len; i++) {
        len += fprintf(out, "  ");
        len += opentac_fprint_stmt(out, builder, &fn->stmts[i]);
        len += fprintf(out, "\n");
    }
    len += fprintf(out, "}\n");
    return len;
}

static size_t opentac_fprint_param(FILE *out, OpentacRefType *type, size_t i) {
    size_t len = 0;
    switch (type->ref) {
    case OPENTAC_REF_IN:
        len += fprintf(out, "in ");
        break;
    case OPENTAC_REF_OUT:
        len += fprintf(out, "out ");
        break;
    }
    len += fprintf(out, "p%zu: ", i);
    len += opentac_fprint_type(out, type->type);
    return len;
}

static size_t opentac_fprint_type(FILE *out, OpentacType *type) {
    size_t len = 0;
    switch (type->tag) {
    case OPENTAC_TYPE_UNIT:
        len += fprintf(out, "unit");
        break;
    case OPENTAC_TYPE_NEVER:
        len += fprintf(out, "never");
        break;
    case OPENTAC_TYPE_BOOL:
        len += fprintf(out, "bool");
        break;
    case OPENTAC_TYPE_I8:
        len += fprintf(out, "i8");
        break;
    case OPENTAC_TYPE_I16:
        len += fprintf(out, "i16");
        break;
    case OPENTAC_TYPE_I32:
        len += fprintf(out, "i32");
        break;
    case OPENTAC_TYPE_I64:
        len += fprintf(out, "i64");
        break;
    case OPENTAC_TYPE_UI8:
        len += fprintf(out, "ui8");
        break;
    case OPENTAC_TYPE_UI16:
        len += fprintf(out, "ui16");
        break;
    case OPENTAC_TYPE_UI32:
        len += fprintf(out, "ui32");
        break;
    case OPENTAC_TYPE_UI64:
        len += fprintf(out, "ui64");
        break;
    case OPENTAC_TYPE_F32:
        len += fprintf(out, "f32");
        break;
    case OPENTAC_TYPE_F64:
        len += fprintf(out, "f64");
        break;
    case OPENTAC_TYPE_PTR:
        len += fprintf(out, "^");
        len += opentac_fprint_type(out, type->ptr.pointee);
        break;
    case OPENTAC_TYPE_FN:
        len += fprintf(out, "(");
        for (size_t i = 0; i < type->fn.len; i++) {
            len += opentac_fprint_reftype(out, &type->fn.params[i]);
            if (i < type->fn.len - 1) {
                len += fprintf(out, ", ");
            }
        }
        len += fprintf(out, ") -> ");
        len += opentac_fprint_type(out, type->fn.result);
        break;
    case OPENTAC_TYPE_TUPLE:
        len += fprintf(out, "(");
        for (size_t i = 0; i < type->tuple.len; i++) {
            len += opentac_fprint_type(out, type->tuple.elems[i]);
            if (i < type->tuple.len - 1) {
                len += fprintf(out, ", ");
            }
        }
        len += fprintf(out, ")");
        break;
    case OPENTAC_TYPE_STRUCT:
        len += fprintf(out, "struct %s", type->struc.name->data);
        break;
    case OPENTAC_TYPE_UNION:
        len += fprintf(out, "union %s", type->struc.name->data);
        break;
    case OPENTAC_TYPE_ARRAY:
        len += fprintf(out, "[");
        len += opentac_fprint_type(out, type->array.elem_type);
        len += fprintf(out, ", %zu]", type->array.len);
        break;
    }
    return len;
}

static size_t opentac_fprint_reftype(FILE *out, OpentacRefType *type) {
    size_t len = 0;
    switch (type->ref) {
    case OPENTAC_REF_IN:
        len += fprintf(out, "in ");
        break;
    case OPENTAC_REF_OUT:
        len += fprintf(out, "out ");
        break;
    }
    len += opentac_fprint_type(out, type->type);
    return len;
}

static size_t opentac_fprint_stmt(FILE *out, OpentacBuilder *builder, OpentacStmt *stmt) {
    size_t len = 0;
    OpentacValue arg0 = { .tag = stmt->tag.left, .val = stmt->left };
    OpentacValue arg1 = { .tag = stmt->tag.right, .val = stmt->right };
    switch (stmt->tag.opcode) {
    case OPENTAC_OP_NOP:
        len += fprintf(out, "nop");
        break;
    case OPENTAC_OP_ALLOCA:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := ");
        len += fprintf(out, "alloca ");
        len += opentac_fprint_type(out, builder->typeset.types[stmt->type]);
        len += fprintf(out, ", %lu, %lu", stmt->left.ui64val, stmt->right.ui64val);
        break;
    case OPENTAC_OP_INDEX_ASSIGN: 
        if (stmt->target < 0) {
            len += fprintf(out, "p%u[", ~(uint32_t) stmt->target);
        } else {
            len += fprintf(out, "t%u[", stmt->target);
        }
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, "] := ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_ASSIGN_INDEX:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, "[");
        len += opentac_fprint_value(out, &arg1);
        len += fprintf(out, "]");
        break;
    case OPENTAC_OP_PARAM:
        len += fprintf(out, "param ");
        len += opentac_fprint_value(out, &arg0);
        break;
    case OPENTAC_OP_RETURN:
        len += fprintf(out, "return ");
        len += opentac_fprint_value(out, &arg0);
        break;
    case OPENTAC_OP_LT:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := lt ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_LE:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := le ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_EQ:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := eq ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_NE:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := ne ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_GT:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := gt ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_GE:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := ge ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_BITAND:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := bitand ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_BITXOR:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := xor ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_BITOR:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := bitor ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_SHL:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := shl ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_SHR:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := shr ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_ROL:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := rol ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_ROR:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := ror ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_ADD:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := add ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_SUB:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := sub ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_MUL:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := mul ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_DIV:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := div ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_MOD:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := mod ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_CALL:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := call ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        break;
    case OPENTAC_OP_NOT:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := not ");
        len += opentac_fprint_value(out, &arg0);
        break;
    case OPENTAC_OP_NEG:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := neg ");
        len += opentac_fprint_value(out, &arg0);
        break;
    case OPENTAC_OP_REF:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := ref ");
        len += opentac_fprint_value(out, &arg0);
        break;
    case OPENTAC_OP_DEREF:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := deref ");
        len += opentac_fprint_value(out, &arg0);
        break;
    case OPENTAC_OP_COPY:
        len += fprintf(out, "t%u", stmt->target);
        len += fprintf(out, " := copy ");
        len += opentac_fprint_value(out, &arg0);
        break;
    case OPENTAC_OP_BRANCH:
        len += fprintf(out, "branch %lu", stmt->left.ui64val);
        break;
    case OPENTAC_OP_BRANCH | OPENTAC_OP_EQ:
        len += fprintf(out, "if eq ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        len += fprintf(out, " branch %u", stmt->label);
        break;
    case OPENTAC_OP_BRANCH | OPENTAC_OP_NE:
        len += fprintf(out, "if ne ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        len += fprintf(out, " branch %u", stmt->label);
        break;
    case OPENTAC_OP_BRANCH | OPENTAC_OP_LT:
        len += fprintf(out, "if lt ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        len += fprintf(out, " branch %u", stmt->label);
        break;
    case OPENTAC_OP_BRANCH | OPENTAC_OP_LE:
        len += fprintf(out, "if le ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        len += fprintf(out, " branch %u", stmt->label);
        break;
    case OPENTAC_OP_BRANCH | OPENTAC_OP_GT:
        len += fprintf(out, "if gt ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        len += fprintf(out, " branch %u", stmt->label);
        break;
    case OPENTAC_OP_BRANCH | OPENTAC_OP_GE:
        len += fprintf(out, "if ge ");
        len += opentac_fprint_value(out, &arg0);
        len += fprintf(out, ", ");
        len += opentac_fprint_value(out, &arg1);
        len += fprintf(out, " branch %u", stmt->label);
        break;
    }
    len += fprintf(out, ";");
    return len;
}

static size_t opentac_fprint_value(FILE *out, OpentacValue *value) {
    size_t len = 0;
    switch (value->tag) {
    case OPENTAC_VAL_ERROR:
        len += fprintf(out, "error");
        break;
    case OPENTAC_VAL_NAMED:
        len += fprintf(out, "%s", value->val.name->data);
        break;
    case OPENTAC_VAL_REG:
        if (value->val.regval < 0) {
            len += fprintf(out, "p%u", ~(uint32_t) value->val.regval);
        } else {
            len += fprintf(out, "t%u", value->val.regval);
        }
        break;
    case OPENTAC_VAL_UNIT:
        len += fprintf(out, "unit");
        break;
    case OPENTAC_VAL_BOOL:
        len += fprintf(out, "%s", value->val.bval? "true" : "false");
        break;
    case OPENTAC_VAL_I8:
        len += fprintf(out, "%d", value->val.i8val);
        break;
    case OPENTAC_VAL_I16:
        len += fprintf(out, "%d", value->val.i16val);
        break;
    case OPENTAC_VAL_I32:
        len += fprintf(out, "%d", value->val.i32val);
        break;
    case OPENTAC_VAL_I64:
        len += fprintf(out, "%ld", value->val.i64val);
        break;
    case OPENTAC_VAL_UI8:
        len += fprintf(out, "%u", value->val.ui8val);
        break;
    case OPENTAC_VAL_UI16:
        len += fprintf(out, "%u", value->val.ui16val);
        break;
    case OPENTAC_VAL_UI32:
        len += fprintf(out, "%u", value->val.ui32val);
        break;
    case OPENTAC_VAL_UI64:
        len += fprintf(out, "%lu", value->val.ui64val);
        break;
    case OPENTAC_VAL_F32:
        len += fprintf(out, "%f", value->val.fval);
        break;
    case OPENTAC_VAL_F64:
        len += fprintf(out, "%f", value->val.dval);
        break;
    }
    return len;
}
