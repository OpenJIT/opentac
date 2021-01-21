#include <errno.h>
#include "include/opentac.h"

void yyerror(const char *error) {
    fprintf(stderr, "error: %s\n", error);
}

int main(int argc, const char **argv) {
    FILE *input = stdin;
    if (argc >= 2) {
        input = fopen(argv[1], "r");
        if (!input) {
            int err = errno;
            yyerror(strerror(err));
            return err;
        }
    }

    OpentacBuilder *builder = opentac_parse(input);
    fclose(input);

    for (size_t i = 0; i < builder->len; i++) {
        if (builder->items[i]->tag != OPENTAC_ITEM_FN) {
            continue;
        }
        OpentacFnBuilder *fn = &builder->items[i]->fn;
        const char *params[] = {
            "rdx",
        };
        const char *registers[] = {
            "rax",
            "rcx",
            "rdx",
            "rbx",
        };
        OpentacRegalloc alloc;
        opentac_alloc_linscan(&alloc, 4, registers, 1, params);
        opentac_alloc_find(&alloc, fn);
        if (opentac_alloc_allocate(&alloc)) {
            return 1;
        }
        struct OpentacRegisterTable table;
        opentac_alloc_regtable(&table, &alloc);

        for (size_t i = 0; i < table.len; i++) {
            printf("%s: ", table.entries[i].key->data);
            if (table.entries[i].purpose.tag == OPENTAC_REG_SPILLED) {
                printf("[%04lx]", table.entries[i].purpose.stack);
            } else if (table.entries[i].purpose.tag == OPENTAC_REG_ALLOCATED) {
                printf("%s", table.entries[i].purpose.reg.name);
            }
            printf("\n");
        }
    }

    return 0;
}
