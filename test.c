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
        struct OpentacGmReg params[] = {
            {
                .regs = {
                    { .size = OPENTAC_SIZE_8, .number = 3 },
                    { .size = OPENTAC_SIZE_16, .number = 3 },
                    { .size = OPENTAC_SIZE_32, .number = 3 },
                    { .size = OPENTAC_SIZE_64, .number = 3 },
                }
            },
        };
        struct OpentacGmReg registers[] = {
            {
                .regs = {
                    { .size = OPENTAC_SIZE_8, .number = 0 },
                    { .size = OPENTAC_SIZE_16, .number = 0 },
                    { .size = OPENTAC_SIZE_32, .number = 0 },
                    { .size = OPENTAC_SIZE_64, .number = 0 },
                }
            },
            {
                .regs = {
                    { .size = OPENTAC_SIZE_8, .number = 2 },
                    { .size = OPENTAC_SIZE_16, .number = 2 },
                    { .size = OPENTAC_SIZE_32, .number = 2 },
                    { .size = OPENTAC_SIZE_64, .number = 2 },
                }
            },
            {
                .regs = {
                    { .size = OPENTAC_SIZE_8, .number = 3 },
                    { .size = OPENTAC_SIZE_16, .number = 3 },
                    { .size = OPENTAC_SIZE_32, .number = 3 },
                    { .size = OPENTAC_SIZE_64, .number = 3 },
                }
            },
            {
                .regs = {
                    { .size = OPENTAC_SIZE_8, .number = 4 },
                    { .size = OPENTAC_SIZE_16, .number = 4 },
                    { .size = OPENTAC_SIZE_32, .number = 4 },
                    { .size = OPENTAC_SIZE_64, .number = 4 },
                }
            },
        };
        OpentacRegalloc alloc;
        opentac_alloc_linscan(&alloc, 4, registers, 1, params);
        opentac_alloc_find(&alloc, builder, fn);
        if (opentac_alloc_allocate(&alloc)) {
            return 1;
        }
        struct OpentacRegisterTable table;
        opentac_alloc_regtable(&table, &alloc);

        for (size_t i = 0; i < table.len; i++) {
            printf("%s: ", table.entries[i].key->data);
            if (table.entries[i].purpose.tag == OPENTAC_REG_SPILLED) {
                printf("[%04lx]", table.entries[i].purpose.stack.offset);
            } else if (table.entries[i].purpose.tag == OPENTAC_REG_ALLOCATED) {
                printf("r%u", table.entries[i].purpose.reg.number);
            } else if (table.entries[i].purpose.tag == OPENTAC_REG_UNIT) {
                printf("unit");
            }
            printf("\n");
        }
    }

    return 0;
}
