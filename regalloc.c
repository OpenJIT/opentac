#include "include/opentac.h"

static void opentac_alloc_remove(void *dest, size_t size, size_t len, void *intervals, size_t idx);

static void opentac_alloc_sort_live(struct OpentacInterval *intervals, size_t lo, size_t hi);
static size_t opentac_alloc_partition_live(struct OpentacInterval *intervals, size_t lo, size_t hi);

static void opentac_alloc_sort_active(struct OpentacInterval *live, struct OpentacActive *active, size_t lo, size_t hi);
static size_t opentac_alloc_partition_active(struct OpentacInterval *live, struct OpentacActive *active, size_t lo, size_t hi);

static void opentac_alloc_memswap(void *a, void *b, size_t size, void *temp);

static void opentac_alloc_fn(struct OpentacRegalloc *alloc, OpentacFnBuilder *fn);
static void opentac_alloc_stmt(struct OpentacRegalloc *alloc, OpentacFnBuilder *fn, OpentacStmt *stmt, size_t idx);

struct OpentacPurposePair {
    size_t index;
    struct OpentacPurpose purpose;
};

void opentac_alloc_linscan(struct OpentacRegalloc *alloc, size_t len, const char **registers) {
    alloc->registers.len = len;
    alloc->registers.cap = 32;
    alloc->registers.registers = malloc(sizeof(struct OpentacMReg) * alloc->registers.cap);

    for (size_t i = 0; i < len; i++) {
        alloc->registers.registers[i].name = registers[i];
    }
    
    alloc->stack.len = 0;
    alloc->stack.cap = 32;
    alloc->stack.intervals = malloc(sizeof(struct OpentacInterval) * alloc->stack.cap);
    
    alloc->live.len = 0;
    alloc->live.cap = 32;
    alloc->live.intervals = malloc(sizeof(struct OpentacInterval) * alloc->live.cap);
    
    alloc->active.len = 0;
    alloc->active.cap = 32;
    alloc->active.actives = malloc(sizeof(struct OpentacActive) * alloc->active.cap);
    
    alloc->offset = 0;
}

void opentac_alloc_add(struct OpentacRegalloc *alloc, struct OpentacInterval *interval) {
    if (interval->stack || interval->ti.size > 8) {
        if (alloc->stack.len == alloc->stack.cap) {
            alloc->stack.cap *= 2;
            alloc->stack.intervals = realloc(alloc->stack.intervals, alloc->stack.cap * sizeof(struct OpentacInterval));
        }

        alloc->stack.intervals[alloc->stack.len++] = *interval;
    } else {
        if (alloc->live.len == alloc->live.cap) {
            alloc->live.cap *= 2;
            alloc->live.intervals = realloc(alloc->live.intervals, alloc->live.cap * sizeof(struct OpentacInterval));
        }

        alloc->live.intervals[alloc->live.len++] = *interval;
    }
}

void opentac_alloc_find(struct OpentacRegalloc *alloc, OpentacBuilder *builder) {
    for (size_t i = 0; i < builder->len; i++) {
        OpentacItem *item = builder->items[i];
        switch (item->tag) {
        case OPENTAC_ITEM_DECL:
            break;
        case OPENTAC_ITEM_FN:
            opentac_alloc_fn(alloc, &item->fn);
            break;
        }
    }
}

void opentac_alloc_regtable(struct OpentacRegisterTable *dest, struct OpentacRegalloc *alloc) {
    size_t cap = 32;
    dest->len = 0;
    dest->cap = cap;
    dest->entries = malloc(cap * sizeof(struct OpentacRegEntry));

    for (size_t i = 0; i < alloc->live.len; i++) {
        if (dest->len == dest->cap) {
            dest->cap *= 2;
            dest->entries = realloc(dest->entries, dest->cap * sizeof(struct OpentacRegEntry));
        }

        dest->entries[dest->len].key = opentac_string(alloc->live.intervals[i].name);
        dest->entries[dest->len++].purpose = alloc->live.intervals[i].purpose;
    }

    for (size_t i = 0; i < alloc->stack.len; i++) {
        if (dest->len == dest->cap) {
            dest->cap *= 2;
            dest->entries = realloc(dest->entries, dest->cap * sizeof(struct OpentacRegEntry));
        }

        dest->entries[dest->len].key = opentac_string(alloc->stack.intervals[i].name);
        dest->entries[dest->len++].purpose = alloc->stack.intervals[i].purpose;
    }
}

static void opentac_alloc_fn(struct OpentacRegalloc *alloc, OpentacFnBuilder *fn) {
    for (size_t i = 0; i < fn->len; i++) {
        OpentacStmt *stmt = fn->stmts + i;
        opentac_alloc_stmt(alloc, fn, stmt, i);
    }
}

static void opentac_alloc_stmt(struct OpentacRegalloc *alloc, OpentacFnBuilder *fn, OpentacStmt *stmt, size_t idx) {
    switch (stmt->tag.opcode) {
    case OPENTAC_OP_ASSIGN_INDEX:
    case OPENTAC_OP_LT:
    case OPENTAC_OP_LE:
    case OPENTAC_OP_EQ:
    case OPENTAC_OP_NE:
    case OPENTAC_OP_GT:
    case OPENTAC_OP_GE:
    case OPENTAC_OP_BITAND:
    case OPENTAC_OP_BITXOR:
    case OPENTAC_OP_BITOR:
    case OPENTAC_OP_SHL:
    case OPENTAC_OP_SHR:
    case OPENTAC_OP_ROL:
    case OPENTAC_OP_ROR:
    case OPENTAC_OP_ADD:
    case OPENTAC_OP_SUB:
    case OPENTAC_OP_MUL:
    case OPENTAC_OP_DIV:
    case OPENTAC_OP_MOD:
    case OPENTAC_OP_CALL:
        if (stmt->tag.right == OPENTAC_VAL_NAMED) {
            for (size_t i = 0; i < alloc->live.len; i++) {
                if (strcmp(alloc->live.intervals[i].name, stmt->right.name->data) == 0) {
                    alloc->live.intervals[i].end = idx;
                }
            }
        } else if (stmt->tag.right == OPENTAC_VAL_REG) {
            const char *name = NULL;
            for (size_t i = 0; i < fn->name_table.len; i++) {
                if ((int32_t) fn->name_table.entries[i].ival == stmt->right.regval) {
                    name = fn->name_table.entries[i].key->data;
                    break;
                }
            }
            if (name) {
                for (size_t i = 0; i < alloc->live.len; i++) {
                    if (strcmp(alloc->live.intervals[i].name, name) == 0) {
                        alloc->live.intervals[i].end = idx;
                    }
                }
            }
        }
        /* fallthrough */
    case OPENTAC_OP_NOT:
    case OPENTAC_OP_NEG:
    case OPENTAC_OP_REF:
    case OPENTAC_OP_DEREF:
    case OPENTAC_OP_COPY: {
        if (stmt->tag.left == OPENTAC_VAL_NAMED) {
            for (size_t i = 0; i < alloc->live.len; i++) {
                if (strcmp(alloc->live.intervals[i].name, stmt->left.name->data) == 0) {
                    alloc->live.intervals[i].end = idx;
                }
            }
        } else if (stmt->tag.left == OPENTAC_VAL_REG) {
            const char *name = NULL;
            for (size_t i = 0; i < fn->name_table.len; i++) {
                if ((int32_t) fn->name_table.entries[i].ival == stmt->left.regval) {
                    name = fn->name_table.entries[i].key->data;
                    break;
                }
            }
            if (name) {
                for (size_t i = 0; i < alloc->live.len; i++) {
                    if (strcmp(alloc->live.intervals[i].name, name) == 0) {
                        alloc->live.intervals[i].end = idx;
                    }
                }
            }
        }
        
        int stack = 0;
        // t + 8 hexadecimals + \0
        char *name = malloc(10);
        snprintf(name, 10, "t%x", stmt->target);
        // TODO: placeholder typeinfo
        OpentacTypeInfo ti = { .size = 8, .align = 8 };
        struct OpentacPurpose purpose = { .tag = OPENTAC_REG_SPILLED, .stack = 0 };
        OpentacLifetime start = idx;
        OpentacLifetime end = idx;
        struct OpentacInterval interval = {
            .stack = stack,
            .name = name,
            .ti = ti,
            .purpose = purpose,
            .start = start,
            .end = end
        };
        opentac_alloc_add(alloc, &interval);
    }
        /* fallthrough */
    case OPENTAC_OP_INDEX_ASSIGN:
    case OPENTAC_OP_BRANCH | OPENTAC_OP_LT:
    case OPENTAC_OP_BRANCH | OPENTAC_OP_LE:
    case OPENTAC_OP_BRANCH | OPENTAC_OP_EQ:
    case OPENTAC_OP_BRANCH | OPENTAC_OP_NE:
    case OPENTAC_OP_BRANCH | OPENTAC_OP_GT:
    case OPENTAC_OP_BRANCH | OPENTAC_OP_GE:
        if (stmt->tag.right == OPENTAC_VAL_NAMED) {
            for (size_t i = 0; i < alloc->live.len; i++) {
                if (strcmp(alloc->live.intervals[i].name, stmt->right.name->data) == 0) {
                    alloc->live.intervals[i].end = idx;
                }
            }
        } else if (stmt->tag.right == OPENTAC_VAL_REG) {
            const char *name = NULL;
            for (size_t i = 0; i < fn->name_table.len; i++) {
                if ((int32_t) fn->name_table.entries[i].ival == stmt->right.regval) {
                    name = fn->name_table.entries[i].key->data;
                    break;
                }
            }
            if (name) {
                for (size_t i = 0; i < alloc->live.len; i++) {
                    if (strcmp(alloc->live.intervals[i].name, name) == 0) {
                        alloc->live.intervals[i].end = idx;
                    }
                }
            }
        }
        /* fallthrough */
    case OPENTAC_OP_PARAM:
    case OPENTAC_OP_RETURN:
        if (stmt->tag.left == OPENTAC_VAL_NAMED) {
            for (size_t i = 0; i < alloc->live.len; i++) {
                if (strcmp(alloc->live.intervals[i].name, stmt->left.name->data) == 0) {
                    alloc->live.intervals[i].end = idx;
                }
            }
        } else if (stmt->tag.left == OPENTAC_VAL_REG) {
            const char *name = NULL;
            for (size_t i = 0; i < fn->name_table.len; i++) {
                if ((int32_t) fn->name_table.entries[i].ival == stmt->left.regval) {
                    name = fn->name_table.entries[i].key->data;
                    break;
                }
            }
            if (name) {
                for (size_t i = 0; i < alloc->live.len; i++) {
                    if (strcmp(alloc->live.intervals[i].name, name) == 0) {
                        alloc->live.intervals[i].end = idx;
                    }
                }
            }
        }
    case OPENTAC_OP_BRANCH:
    case OPENTAC_OP_NOP:
        break;
    case OPENTAC_OP_ALLOCA: {
        int stack = 0;
        // t + 8 hexadecimals + \0
        char *name = malloc(10);
        snprintf(name, 10, "t%x", stmt->target);
        // TODO: placeholder typeinfo
        OpentacTypeInfo ti = { .size = 8, .align = 8 };
        struct OpentacPurpose purpose = { .tag = OPENTAC_REG_SPILLED, .stack = 0 };
        OpentacLifetime start = idx;
        OpentacLifetime end = idx;
        struct OpentacInterval interval = {
            .stack = stack,
            .name = name,
            .ti = ti,
            .purpose = purpose,
            .start = start,
            .end = end
        };
        opentac_alloc_add(alloc, &interval);
    } break;
    }
}

void opentac_alloc_allocate(struct OpentacRegalloc *alloc) {
    opentac_alloc_sort_live(alloc->live.intervals, 0, alloc->live.len - 1);

    size_t expire_len = 0;
    size_t *expire = malloc(sizeof(size_t) * alloc->live.len);

    size_t delta_len = 0;
    struct OpentacPurposePair *delta = malloc(sizeof(struct OpentacPurposePair) * alloc->registers.len);

    for (size_t idx = 0; idx < alloc->live.len; idx++) {
        struct OpentacInterval *i = alloc->live.intervals + idx;

        for (size_t idx0 = 0; idx0 < alloc->active.len; idx0++) {
            size_t j = alloc->active.actives[idx0].index;
            OpentacLifetime lt = alloc->live.intervals[j].end;
            if (lt >= i->start) {
                break;
            }
            expire[expire_len++] = j;
        }

        for (size_t idx0 = 0; idx0 < expire_len; idx0++) {
            size_t j = expire[idx0];
            struct OpentacActive active;
            opentac_alloc_remove(&active, sizeof(struct OpentacActive), alloc->active.len, alloc->active.actives, j);
            if (alloc->registers.len == alloc->registers.cap) {
                alloc->registers.cap *= 2;
                alloc->registers.registers = realloc(alloc->registers.registers, sizeof(struct OpentacMReg) * alloc->registers.cap);
            }
            alloc->registers.registers[alloc->registers.len++] = active.reg;
        }
        expire_len = 0;

        if (!alloc->registers.len) {
            if (alloc->active.len) {
                size_t j = alloc->active.len - 1;
                struct OpentacActive *active = alloc->active.actives + j;
                struct OpentacInterval *spill = alloc->live.intervals + active->index;
                if (spill->end > i->end) {
                    delta[delta_len].index = idx;
                    delta[delta_len++].purpose = spill->purpose;
                    alloc->offset += 8;
                    delta[delta_len].index = active->index;
                    delta[delta_len].purpose.tag = OPENTAC_REG_SPILLED;
                    delta[delta_len++].purpose.stack = alloc->offset;
                } else {
                    alloc->offset += 8;
                    delta[delta_len].index = idx;
                    delta[delta_len].purpose.tag = OPENTAC_REG_SPILLED;
                    delta[delta_len++].purpose.stack = alloc->offset;
                }
            }
        } else {
            struct OpentacMReg reg = alloc->registers.registers[--alloc->registers.len];
            delta[delta_len].index = idx;
            delta[delta_len].purpose.tag = OPENTAC_REG_ALLOCATED;
            delta[delta_len++].purpose.reg = reg;
            opentac_alloc_sort_active(alloc->live.intervals, alloc->active.actives, 0, alloc->active.len - 1);
        }

        for (size_t idx0 = 0; idx0 < delta_len; idx0++) {
            alloc->live.intervals[delta[idx0].index].purpose = delta[idx0].purpose;
        }
        delta_len = 0;
    }

    free(expire);
    free(delta);
}

static void opentac_alloc_remove(void *dest, size_t size, size_t len, void *ptr, size_t idx) {
    memcpy(dest, ((uint8_t *) ptr) + idx * size, size);
    memmove(((uint8_t *) ptr) + idx * size, ((uint8_t *) ptr) + (idx + 1) * size, (len - idx - 1) * size);
}

// quicksort
static void opentac_alloc_sort_live(struct OpentacInterval *intervals, size_t lo, size_t hi) {
    if (hi == (size_t) -1) {
        return;
    }
    if (lo < hi) {
        size_t p = opentac_alloc_partition_live(intervals, lo, hi);
        opentac_alloc_sort_live(intervals, lo, p - 1);
        opentac_alloc_sort_live(intervals, p + 1, hi);
    }
}

static size_t opentac_alloc_partition_live(struct OpentacInterval *intervals, size_t lo, size_t hi) {
    uint8_t temp[sizeof(struct OpentacInterval)];
    uint64_t pivot = intervals[hi].start;
    size_t i = lo;
    for (size_t j = lo; j < hi; j++) {
        if (intervals[j].start < pivot) {
            opentac_alloc_memswap(intervals + i, intervals + j, sizeof(struct OpentacInterval), temp);
            ++i;
        }
    }
    opentac_alloc_memswap(intervals + i, intervals + hi, sizeof(struct OpentacInterval), temp);
    return i;
}

static void opentac_alloc_sort_active(struct OpentacInterval *live, struct OpentacActive *active, size_t lo, size_t hi) {
    if (hi == (size_t) -1) {
        return;
    }
    if (lo < hi) {
        size_t p = opentac_alloc_partition_active(live, active, lo, hi);
        opentac_alloc_sort_active(live, active, lo, p - 1);
        opentac_alloc_sort_active(live, active, p + 1, hi);
    }
}

static size_t opentac_alloc_partition_active(struct OpentacInterval *live, struct OpentacActive *active, size_t lo, size_t hi) {
    uint8_t temp[sizeof(struct OpentacActive)];
    uint64_t pivot = live[hi].end;
    size_t i = lo;
    for (size_t j = lo; j < hi; j++) {
        if (live[j].end < pivot) {
            opentac_alloc_memswap(active + i, active + j, sizeof(struct OpentacActive), temp);
            ++i;
        }
    }
    opentac_alloc_memswap(active + i, active + hi, sizeof(struct OpentacActive), temp);
    return i;
}

static void opentac_alloc_memswap(void *a, void *b, size_t size, void *temp) {
    memcpy(temp, a, size);
    memcpy(a, b, size);
    memcpy(b, temp, size);
}
