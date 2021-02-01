#include <limits.h>
#include "include/opentac.h"

static void opentac_alloc_sort_live(struct OpentacInterval *intervals, size_t lo, size_t hi);
static size_t opentac_alloc_partition_live(struct OpentacInterval *intervals, size_t lo, size_t hi);

static void opentac_alloc_sort_active(struct OpentacInterval *live, struct OpentacActive *active, size_t lo, size_t hi);
static size_t opentac_alloc_partition_active(struct OpentacInterval *live, struct OpentacActive *active, size_t lo, size_t hi);

static void opentac_alloc_memswap(void *a, void *b, size_t size, void *temp);

static void opentac_alloc_fn(struct OpentacRegalloc *alloc, OpentacBuilder *builder, OpentacFnBuilder *fn);
static void opentac_alloc_stmt(struct OpentacRegalloc *alloc, OpentacBuilder *builder, OpentacFnBuilder *fn, OpentacStmt *stmt, size_t idx);

static inline int log2i(unsigned int val) {
    if (val == 0) return INT_MIN;
    if (val == 1) return 0;
    int log = 0;
    while (val >>= 1) log++;
    return log;
}

static inline int log2ll(unsigned long long val) {
    if (val == 0) return INT_MIN;
    if (val == 1) return 0;
    int log = 0;
    while (val >>= 1) log++;
    return log;
}

struct OpentacPurposePair {
    size_t index;
    struct OpentacPurpose purpose;
};

void opentac_alloc_linscan(struct OpentacRegalloc *alloc, size_t len, struct OpentacGmReg *regs, size_t paramc, struct OpentacGmReg *params) {
    alloc->registers.len = len;
    alloc->registers.cap = 32;
    alloc->registers.registers = malloc(sizeof(struct OpentacGmReg) * alloc->registers.cap);

    for (size_t i = 0; i < len; i++) {
        alloc->registers.registers[i] = regs[i];
    }
    
    alloc->parameters.len = paramc;
    alloc->parameters.cap = 32;
    alloc->parameters.registers = malloc(sizeof(struct OpentacGmReg) * alloc->parameters.cap);

    for (size_t i = 0; i < len; i++) {
        alloc->parameters.registers[i] = params[i];
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

        interval->reg = NULL;
        alloc->stack.intervals[alloc->stack.len++] = *interval;
    } else {
        if (alloc->live.len == alloc->live.cap) {
            alloc->live.cap *= 2;
            alloc->live.intervals = realloc(alloc->live.intervals, alloc->live.cap * sizeof(struct OpentacInterval));
        }

        interval->reg = NULL;
        alloc->live.intervals[alloc->live.len++] = *interval;
    }
}

void opentac_alloc_param(struct OpentacRegalloc *alloc, struct OpentacInterval *interval, uint32_t param) {
    if (interval->stack || interval->ti.size > 8) {
        if (alloc->stack.len == alloc->stack.cap) {
            alloc->stack.cap *= 2;
            alloc->stack.intervals = realloc(alloc->stack.intervals, alloc->stack.cap * sizeof(struct OpentacInterval));
        }

        interval->reg = NULL;
        alloc->stack.intervals[alloc->stack.len++] = *interval;
    } else {
        if (alloc->live.len == alloc->live.cap) {
            alloc->live.cap *= 2;
            alloc->live.intervals = realloc(alloc->live.intervals, alloc->live.cap * sizeof(struct OpentacInterval));
        }

        if (param < alloc->parameters.len) {
            interval->reg = alloc->parameters.registers[param].regs[log2ll(interval->ti.size)].name;
        }
        alloc->live.intervals[alloc->live.len++] = *interval;
    }
}

void opentac_alloc_find(struct OpentacRegalloc *alloc, OpentacBuilder *builder, OpentacFnBuilder *fn) {
    opentac_alloc_fn(alloc, builder, fn);
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

        dest->entries[dest->len].start = alloc->live.intervals[i].start;
        dest->entries[dest->len].end = alloc->live.intervals[i].end;
        dest->entries[dest->len].key = opentac_string(alloc->live.intervals[i].name);
        dest->entries[dest->len++].purpose = alloc->live.intervals[i].purpose;
    }

    for (size_t i = 0; i < alloc->stack.len; i++) {
        if (dest->len == dest->cap) {
            dest->cap *= 2;
            dest->entries = realloc(dest->entries, dest->cap * sizeof(struct OpentacRegEntry));
        }

        dest->entries[dest->len].start = alloc->live.intervals[i].start;
        dest->entries[dest->len].end = alloc->live.intervals[i].end;
        dest->entries[dest->len].key = opentac_string(alloc->stack.intervals[i].name);
        dest->entries[dest->len++].purpose = alloc->stack.intervals[i].purpose;
    }
}

static void opentac_alloc_fn(struct OpentacRegalloc *alloc, OpentacBuilder *builder, OpentacFnBuilder *fn) {
    for (size_t i = 0; i < fn->params.len; i++) {
        int stack = 0;
        // p + 8 hexadecimals + \0
        char *name = malloc(10);
        snprintf(name, 10, "p%x", (uint32_t) i);
        OpentacTypeInfo ti = { .size = fn->params.params[i].type->size, .align = fn->params.params[i].type->align };
        switch (fn->params.params[i].ref) {
        case OPENTAC_REF_NONE:
            break;
        case OPENTAC_REF_IN:
            ti.size = 8;
            ti.align = 8;
            break;
        case OPENTAC_REF_OUT:
            ti.size = 8;
            ti.align = 8;
            break;
        }
        struct OpentacPurpose purpose = {
            .tag = OPENTAC_REG_SPILLED,
            .stack.offset = 0,
            .stack.size = log2ll(ti.size),
            .stack.align = log2ll(ti.align)
        };
        OpentacLifetime start = -1;
        OpentacLifetime end = -1;
        struct OpentacInterval interval = {
            .stack = stack,
            .name = name,
            .ti = ti,
            .purpose = purpose,
            .start = start,
            .end = end
        };
        opentac_alloc_param(alloc, &interval, i);
    }
    for (size_t i = 0; i < fn->len; i++) {
        OpentacStmt *stmt = fn->stmts + i;
        opentac_alloc_stmt(alloc, builder, fn, stmt, i);
    }
}

static void opentac_alloc_stmt(struct OpentacRegalloc *alloc, OpentacBuilder *builder, OpentacFnBuilder *fn, OpentacStmt *stmt, size_t idx) {
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
        OpentacTypeInfo ti;
        ti.size = builder->typeset.types[stmt->type]->size;
        ti.align = builder->typeset.types[stmt->type]->align;
        struct OpentacPurpose purpose = {
            .tag = OPENTAC_REG_SPILLED,
            .stack.offset = 0,
            .stack.size = log2ll(ti.size),
            .stack.align = log2ll(ti.align)
        };
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
        OpentacTypeInfo ti;
        // pointer size
        ti.size = 8;
        ti.align = 8;
        struct OpentacPurpose purpose = {
            .tag = OPENTAC_REG_SPILLED,
            .stack.offset = 0,
            .stack.size = log2ll(ti.size),
            .stack.align = log2ll(ti.align)
        };
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
    case OPENTAC_OP_INDEX_ASSIGN:
    case OPENTAC_OP_MEMCPY: {
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
        const char *name = NULL;
        for (size_t i = 0; i < fn->name_table.len; i++) {
            if ((int32_t) fn->name_table.entries[i].ival == stmt->target) {
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
    } break;
    }
}

int opentac_alloc_allocate(struct OpentacRegalloc *alloc) {
    opentac_alloc_sort_live(alloc->live.intervals, 0, alloc->live.len - 1);

    size_t expire_len = 0;
    size_t *expire = malloc(sizeof(size_t) * alloc->live.len);

    size_t delta_len = 0;
    struct OpentacPurposePair *delta = malloc(sizeof(struct OpentacPurposePair) * alloc->registers.len);

    for (size_t idx = 0; idx < alloc->live.len; idx++) {
        struct OpentacInterval *i = alloc->live.intervals + idx;

        opentac_alloc_sort_active(alloc->live.intervals, alloc->active.actives, 0, alloc->active.len - 1);
        for (size_t idx0 = 0; idx0 < alloc->active.len; idx0++) {
            size_t j = alloc->active.actives[idx0].index;
            if (alloc->live.intervals[j].ti.size) {
                OpentacLifetime lt = alloc->live.intervals[j].end;
                if (lt >= i->start) {
                    break;
                }
                expire[expire_len++] = idx0;
            }
        }

        for (size_t idx0 = 0; idx0 < expire_len; idx0++) {
            size_t j = expire[idx0];
            struct OpentacActive active = alloc->active.actives[j];
            --alloc->active.len;
            memmove(alloc->active.actives + j, alloc->active.actives + j + 1, (alloc->active.len - j) * sizeof(struct OpentacActive));
            for (size_t i = idx0 + 1; i < expire_len; i++) {
                if (expire[i] > j) {
                    --expire[i];
                }
            }
            if (alloc->registers.len == alloc->registers.cap) {
                alloc->registers.cap *= 2;
                alloc->registers.registers = realloc(alloc->registers.registers, sizeof(struct OpentacGmReg) * alloc->registers.cap);
            }
            alloc->registers.registers[alloc->registers.len++] = active.reg;
        }
        expire_len = 0;

        if (i->reg) {
            size_t j;
            size_t k;
            for (j = 0; j < alloc->registers.len; j++) {
                for (k = 0; k < OPENTAC_SIZE_COUNT; k++) {
                    if (strcmp(i->reg, alloc->registers.registers[j].regs[k].name) == 0) {
                        // yeah, I use labels when they're clear
                        // no, I don't care about "goto statements considered harmful"
                        // check out "'goto statements considered harmful' considered harmful"
                        goto brk;
                    }
                }
            }
brk:
            if (j == alloc->registers.len) {
                fprintf(stderr, "error: cannot find a free register with this name: %s\n", i->reg);
                return 1;
            }
            struct OpentacGmReg reg = alloc->registers.registers[j];
            delta[delta_len].index = idx;
            delta[delta_len].purpose.tag = OPENTAC_REG_ALLOCATED;
            delta[delta_len++].purpose.reg = reg.regs[k];

            alloc->active.actives[alloc->active.len].index = idx;
            alloc->active.actives[alloc->active.len++].reg = reg;

            --alloc->registers.len;
            memmove(alloc->registers.registers + j, alloc->registers.registers + j + 1, (alloc->registers.len - j) * sizeof(struct OpentacGmReg));
        } else if (!alloc->registers.len) {
            alloc->offset += 8;
            delta[delta_len].index = idx;
            delta[delta_len].purpose.tag = OPENTAC_REG_SPILLED;
            delta[delta_len++].purpose.stack.offset = alloc->offset;
        } else {
            if (i->ti.size) {
                struct OpentacGmReg reg = alloc->registers.registers[--alloc->registers.len];
                delta[delta_len].index = idx;
                delta[delta_len].purpose.tag = OPENTAC_REG_ALLOCATED;
                delta[delta_len++].purpose.reg = reg.regs[log2ll(i->ti.size)];
                
                if (alloc->active.len == alloc->active.cap) {
                    alloc->active.cap *= 2;
                    alloc->active.actives = realloc(alloc->active.actives, sizeof(struct OpentacActive) * alloc->active.cap);
                }
            
                alloc->active.actives[alloc->active.len].index = idx;
                alloc->active.actives[alloc->active.len++].reg = reg;
            } else {
                delta[delta_len].index = idx;
                delta[delta_len].purpose.tag = OPENTAC_REG_UNIT;
                delta[delta_len++].purpose.reg.name = NULL;
            }
        }

        for (size_t idx0 = 0; idx0 < delta_len; idx0++) {
            alloc->live.intervals[delta[idx0].index].purpose = delta[idx0].purpose;
        }
        delta_len = 0;
    }

    free(expire);
    free(delta);

    return 0;
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
    int64_t pivot = intervals[hi].start;
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
    int64_t pivot = live[hi].end;
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
