#include "include/opentac.h"

static int opentac_pass0_fn(OpentacFnBuilder *fn);
static int opentac_pass0_fn_deref(OpentacFnBuilder *fn, size_t i);

int opentac_pass0(OpentacBuilder *builder) {
    int status = 0;
    for (size_t i = 0; i < builder->len; i++) {
        if (builder->items[i]->tag == OPENTAC_ITEM_FN) {
            status |= opentac_pass0_fn(&builder->items[i]->fn);
        }
    }
    return status;
}

static int opentac_pass0_fn(OpentacFnBuilder *fn) {
    int status = 0;
    for (size_t i = 0; i < fn->len; i++) {
        if (fn->stmts[i].tag.opcode == OPENTAC_OP_DEREF) {
            status |= opentac_pass0_fn_deref(fn, i);
        }
    }
    return status;
}

static int opentac_pass0_fn_deref(OpentacFnBuilder *fn, size_t i) {
    OpentacStmt *base = &fn->stmts[i];
    int status = 0;
    for (size_t j = i + 1; j < fn->len; j++) {
        if (fn->stmts[j].tag.opcode == OPENTAC_OP_INDEX_ASSIGN
            && fn->stmts[j].tag.right == OPENTAC_VAL_REG
            && fn->stmts[j].right.regval == base->target) {
            fn->stmts[j].tag.opcode = OPENTAC_OP_MEMCPY;
            fn->stmts[j].tag.right = base->tag.left;
            fn->stmts[j].right = base->left;
        }
        if (fn->stmts[j].tag.opcode == OPENTAC_OP_REF
            && fn->stmts[j].tag.left == OPENTAC_VAL_REG
            && fn->stmts[j].left.regval == base->target) {
            fn->stmts[j].tag.opcode = OPENTAC_OP_COPY;
            fn->stmts[j].tag.left = base->tag.left;
            fn->stmts[j].left = base->left;
        }
    }
    return status;
}
