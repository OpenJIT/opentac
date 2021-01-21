#ifndef OPENTAC_H
#define OPENTAC_H 1

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define opentac_stringify1(x) #x
#define opentac_stringify(x) opentac_stringify1(x)
#define opentac_assertf(x, fmt, ...) do {       \
        if (!(x)) { \
            fprintf(stderr, "error: assertion failed at %s:%d: " fmt "\n", __FILE__, __LINE__, __VA_ARGS__); \
            exit(1); \
        } \
    } while (0)
#define opentac_assert(x) opentac_assertf(x, "%s", opentac_stringify(x))

typedef struct OpentacType OpentacType;
typedef struct OpentacBuilder OpentacBuilder;
typedef struct OpentacItem OpentacItem;
typedef struct OpentacFnBuilder OpentacFnBuilder;
typedef struct OpentacDecl OpentacDecl;
typedef struct OpentacStmt OpentacStmt;
typedef struct OpentacOpcode OpentacOpcode;
typedef struct OpentacValue OpentacValue;
typedef union OpentacVal OpentacVal;
typedef struct OpentacString OpentacString;
typedef struct OpentacTypeInfo OpentacTypeInfo;
typedef struct OpentacRegalloc OpentacRegalloc;
typedef int32_t OpentacRegister;
typedef uint32_t OpentacLabel;

struct OpentacTypeset {
    size_t len;
    size_t cap;
    OpentacType **types;
};

struct OpentacBuilder {
    size_t len;
    size_t cap;
    OpentacItem **items;
    OpentacItem **current;
    struct OpentacTypeset typeset;
};

struct OpentacDecl {
    OpentacString *name;
    OpentacType *type;
};

struct OpentacEntry {
    OpentacString *key;
    union {
        uint32_t ival;
        void *pval;
    };
};

struct OpentacNameTable {
    size_t len;
    size_t cap;
    struct OpentacEntry *entries;
};

struct OpentacParams {
    size_t len;
    size_t cap;
    OpentacType **params;
};

struct OpentacFnBuilder {
    OpentacString *name;
    struct OpentacNameTable name_table;
    struct OpentacParams params;
    OpentacRegister param;
    OpentacRegister reg;
    OpentacLabel label;
    size_t len;
    size_t cap;
    OpentacStmt *stmts;
    OpentacStmt *current;
};

enum {
    OPENTAC_ITEM_DECL,
    OPENTAC_ITEM_FN,
};

struct OpentacItem {
    int tag;
    union {
        OpentacDecl decl;
        OpentacFnBuilder fn;
    };
};

enum {
    OPENTAC_OP_NOP = 0x00,
    OPENTAC_OP_ALLOCA,
    OPENTAC_OP_INDEX_ASSIGN,
    OPENTAC_OP_ASSIGN_INDEX,
    OPENTAC_OP_PARAM,
    OPENTAC_OP_RETURN,
    OPENTAC_OP_LT,
    OPENTAC_OP_LE,
    OPENTAC_OP_EQ,
    OPENTAC_OP_NE,
    OPENTAC_OP_GT,
    OPENTAC_OP_GE,
    OPENTAC_OP_BITAND,
    OPENTAC_OP_BITXOR,
    OPENTAC_OP_BITOR,
    OPENTAC_OP_SHL,
    OPENTAC_OP_SHR,
    OPENTAC_OP_ROL,
    OPENTAC_OP_ROR,
    OPENTAC_OP_ADD,
    OPENTAC_OP_SUB,
    OPENTAC_OP_MUL,
    OPENTAC_OP_DIV,
    OPENTAC_OP_MOD,
    OPENTAC_OP_CALL,
    OPENTAC_OP_NOT,
    OPENTAC_OP_NEG,
    OPENTAC_OP_REF,
    OPENTAC_OP_DEREF,
    OPENTAC_OP_COPY,
    OPENTAC_OP_BRANCH = 0xff00,
};

// size should be 32 bits
struct OpentacOpcode {
    unsigned int left:4;
    unsigned int right:4;
    unsigned int opcode:24;
};

enum {
    OPENTAC_VAL_ERROR,
    OPENTAC_VAL_NAMED,
    OPENTAC_VAL_REG,
    OPENTAC_VAL_UNIT,
    OPENTAC_VAL_BOOL,
    OPENTAC_VAL_I8,
    OPENTAC_VAL_I16,
    OPENTAC_VAL_I32,
    OPENTAC_VAL_I64,
    OPENTAC_VAL_UI8,
    OPENTAC_VAL_UI16,
    OPENTAC_VAL_UI32,
    OPENTAC_VAL_UI64,
    OPENTAC_VAL_F32,
    OPENTAC_VAL_F64,
    OPENTAC_VAL_PTR,
};

// size should be 64 bits
union OpentacVal {
    OpentacString *name;
    OpentacRegister regval;
    bool bval;
    int8_t i8val;
    int16_t i16val;
    int32_t i32val;
    int64_t i64val;
    uint8_t ui8val;
    uint16_t ui16val;
    uint32_t ui32val;
    uint64_t ui64val;
    float fval;
    double dval;
    uint8_t *ptrval;
};

struct OpentacValue {
    int tag;
    OpentacVal val;
};

// size should be 32 + 32 + 64 + 64 bits = 24 bytes
struct OpentacStmt {
    OpentacOpcode tag;
    union {
        OpentacRegister target;
        OpentacLabel label;
    };
    OpentacVal left;
    OpentacVal right;
};

enum {
    OPENTAC_TYPE_UNIT,
    OPENTAC_TYPE_NEVER,
    OPENTAC_TYPE_BOOL,
    OPENTAC_TYPE_I8,
    OPENTAC_TYPE_I16,
    OPENTAC_TYPE_I32,
    OPENTAC_TYPE_I64,
    OPENTAC_TYPE_UI8,
    OPENTAC_TYPE_UI16,
    OPENTAC_TYPE_UI32,
    OPENTAC_TYPE_UI64,
    OPENTAC_TYPE_F32,
    OPENTAC_TYPE_F64,
    OPENTAC_TYPE_PTR,
    OPENTAC_TYPE_FN,
    OPENTAC_TYPE_TUPLE,
    OPENTAC_TYPE_STRUCT,
    OPENTAC_TYPE_UNION,
    OPENTAC_TYPE_ARRAY,
};

struct OpentacTypePtr {
    OpentacType *pointee;
};

struct OpentacTypeFn {
    size_t len;
    size_t cap;
    OpentacType **params;
    OpentacType *result;
};

struct OpentacTypeTuple {
    size_t len;
    size_t cap;
    OpentacType **elems;
};

struct OpentacTypeStruct {
    OpentacString *name;
    size_t len;
    size_t cap;
    OpentacType **elems;
};

struct OpentacTypeArray {
    OpentacType *elem_type;
    uint64_t len;
};

struct OpentacType {
    uint64_t size;
    uint64_t align;
    int tag;
    union {
        struct OpentacTypePtr ptr;
        struct OpentacTypeFn fn;
        struct OpentacTypeTuple tuple;
        struct OpentacTypeStruct struc;
        struct OpentacTypeArray array;
    };
};

struct OpentacString {
    size_t len;
    size_t cap;
    char data[];
};

struct OpentacTypeInfo {
    size_t size;
    size_t align;
};

enum {
    OPENTAC_REG_ALLOCATED,
    OPENTAC_REG_SPILLED,
};

// machine register
struct OpentacMReg {
    const char *name;
};

struct OpentacPurpose {
    int tag;
    union {
        struct OpentacMReg reg;
        uint64_t stack;
    };
};

struct OpentacRegEntry {
    OpentacString *key;
    struct OpentacPurpose purpose;
};

struct OpentacRegisterTable {
    size_t len;
    size_t cap;
    struct OpentacRegEntry *entries;
};

typedef uint64_t OpentacLifetime;

struct OpentacInterval {
    int stack;
    const char *name;
    OpentacTypeInfo ti;
    struct OpentacPurpose purpose;
    OpentacLifetime start;
    OpentacLifetime end;
};

struct OpentacPool {
    size_t len;
    size_t cap;
    struct OpentacMReg *registers;
};

struct OpentacIntervals {
    size_t len;
    size_t cap;
    struct OpentacInterval *intervals;
};

struct OpentacActive {
    size_t index;
    struct OpentacMReg reg;
};

struct OpentacActives {
    size_t len;
    size_t cap;
    struct OpentacActive *actives;
};

struct OpentacRegalloc {
    struct OpentacPool registers;
    struct OpentacIntervals live;
    struct OpentacIntervals stack;
    struct OpentacActives active;
    uint64_t offset;
};

OpentacBuilder *opentac_parse(FILE *file);

void opentac_builder(OpentacBuilder *builder);
void opentac_builder_with_cap(OpentacBuilder *builder, size_t cap);
OpentacBuilder *opentac_builderp();
OpentacBuilder *opentac_builderp_with_cap(size_t cap);

void opentac_alloc_linscan(struct OpentacRegalloc *alloc, size_t len, const char **registers);
void opentac_alloc_add(struct OpentacRegalloc *alloc, struct OpentacInterval *interval);
void opentac_alloc_allocate(struct OpentacRegalloc *alloc);
void opentac_alloc_find(struct OpentacRegalloc *alloc, OpentacBuilder *builder);
void opentac_alloc_regtable(struct OpentacRegisterTable *dest, struct OpentacRegalloc *alloc);

void opentac_build_decl(OpentacBuilder *builder, OpentacString *name, OpentacType *type);
void opentac_build_function(OpentacBuilder *builder, OpentacString *name);
void opentac_build_function_param(OpentacBuilder *builder, OpentacString *name, OpentacType *type);
void opentac_finish_function(OpentacBuilder *builder);
OpentacItem *opentac_item_ptr(OpentacBuilder *builder);
void opentac_set_item(OpentacBuilder *builder, OpentacItem *item);
void opentac_builder_insert(OpentacBuilder *builder, size_t index);
void opentac_builder_goto(OpentacBuilder *builder, size_t index);
void opentac_builder_goto_end(OpentacBuilder *builder);

size_t opentac_stmt_offset(OpentacBuilder *builder, OpentacStmt *stmt);
OpentacStmt *opentac_stmt_ptr(OpentacBuilder *builder);
OpentacStmt *opentac_stmt_at(OpentacBuilder *builder, size_t offset);
OpentacValue opentac_build_alloca(OpentacBuilder *builder, uint64_t size, uint64_t align);
OpentacValue opentac_build_binary(OpentacBuilder *builder, int opcode, OpentacValue left, OpentacValue right);
OpentacValue opentac_build_unary(OpentacBuilder *builder, int opcode, OpentacValue value);
void opentac_build_index_assign(OpentacBuilder *builder, OpentacRegister target, OpentacValue offset, OpentacValue value);
OpentacValue opentac_build_assign_index(OpentacBuilder *builder, OpentacValue value, OpentacValue offset);
void opentac_build_param(OpentacBuilder *builder, OpentacValue value);
OpentacValue opentac_build_call(OpentacBuilder *builder, OpentacValue func, uint64_t nparams);
void opentac_build_return(OpentacBuilder *builder, OpentacValue value);
void opentac_build_if_branch(OpentacBuilder *builder, int relop, OpentacValue left, OpentacValue right, OpentacLabel label);
void opentac_build_branch(OpentacBuilder *builder, OpentacValue value);

void opentac_fn_insert(OpentacBuilder *builder, size_t index);
void opentac_fn_goto(OpentacBuilder *builder, size_t index);
void opentac_fn_goto_end(OpentacBuilder *builder);
void opentac_fn_bind_int(OpentacBuilder *builder, OpentacString *name, uint32_t val);
void opentac_fn_bind_ptr(OpentacBuilder *builder, OpentacString *name, void *val);
uint32_t opentac_fn_get_int(OpentacBuilder *builder, OpentacString *name);
void *opentac_fn_get_ptr(OpentacBuilder *builder, OpentacString *name);

OpentacType *opentac_type_unit(OpentacBuilder *builder);
OpentacType *opentac_type_never(OpentacBuilder *builder);
OpentacType *opentac_type_bool(OpentacBuilder *builder);
OpentacType *opentac_type_i8(OpentacBuilder *builder);
OpentacType *opentac_type_i16(OpentacBuilder *builder);
OpentacType *opentac_type_i32(OpentacBuilder *builder);
OpentacType *opentac_type_i64(OpentacBuilder *builder);
OpentacType *opentac_type_ui8(OpentacBuilder *builder);
OpentacType *opentac_type_ui16(OpentacBuilder *builder);
OpentacType *opentac_type_ui32(OpentacBuilder *builder);
OpentacType *opentac_type_ui64(OpentacBuilder *builder);
OpentacType *opentac_type_f32(OpentacBuilder *builder);
OpentacType *opentac_type_f64(OpentacBuilder *builder);
OpentacType *opentac_type_named(OpentacBuilder *builder, int tag, OpentacString *name);
OpentacType *opentac_type_ptr(OpentacBuilder *builder, OpentacType *pointee);
OpentacType *opentac_type_fn(OpentacBuilder *builder, size_t len, OpentacType **params, OpentacType *result);
OpentacType *opentac_type_tuple(OpentacBuilder *builder, size_t len, OpentacType **elems);
OpentacType *opentac_type_struct(OpentacBuilder *builder, OpentacString *name, size_t len, OpentacType **elems);
OpentacType *opentac_type_union(OpentacBuilder *builder, OpentacString *name, size_t len, OpentacType **elems);
OpentacType *opentac_type_array(OpentacBuilder *builder, OpentacType *elem_type, uint64_t len);

OpentacString *opentac_string(const char *str);
void opentac_del_string(OpentacString *str);

#endif /* OPENTAC_H */
