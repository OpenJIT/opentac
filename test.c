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

    OpentacBuilder *result = opentac_parse(input);
    fclose(input);

    return result == NULL;
}
