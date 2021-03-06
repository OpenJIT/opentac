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
typedef struct OpentacRefType OpentacRefType;
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

enum {
    OPENTAC_REF_NONE,
    OPENTAC_REF_IN,
    OPENTAC_REF_OUT,
};

struct OpentacRefType {
    OpentacType *type;
    int ref;
};

struct OpentacTypeset {
    size_t len;
    size_t cap;
    OpentacType **types;
};

enum {
    OPENTAC_DBG_SET_FILE, // uint8_t, uint32_t
    OPENTAC_DBG_SET_DIR, // uint8_t, uint32_t
    OPENTAC_DBG_SET_COL, // uint8_t, uint16_t
    OPENTAC_DBG_SET_PC, // uint8_t, uint64_t
    OPENTAC_DBG_SET_LINE, // uint8_t, uint32_t
    OPENTAC_DBG_INC_PC, // uint8_t, uint16_t
    OPENTAC_DBG_INC_LINE, // uint8_t, int16_t
    OPENTAC_DBG_INC, // uint8_t, uint8_t, int8_t
    OPENTAC_DBG_END, // uint8_t
};

struct OpentacDebugStmt {
    uint8_t opcode;
    struct {
        uint8_t u8val;
        int8_t i8val;
        uint16_t u16val;
        int16_t i16val;
        uint32_t u32val;
        int32_t i32val;
        uint64_t u64val;
        int64_t i64val;
    } arg0, arg1;
};

struct OpentacDebugData {
    size_t len;
    size_t cap;
    void *data;
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
    OpentacRefType *params;
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
    struct OpentacDebugData debug;
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
    OPENTAC_OP_MEMCPY = 0x100,
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
};

struct OpentacValue {
    int tag;
    OpentacVal val;
};

// size should be 32 + 32 + 64 + 64 + 64 bits = 32 bytes
struct OpentacStmt {
    OpentacOpcode tag;
    union {
        OpentacRegister target;
        OpentacLabel label;
    };
    uint64_t type;
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
    OpentacRefType *params;
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
    OpentacString **fields;
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
    OPENTAC_REG_UNIT,
    OPENTAC_REG_ALLOCATED,
    OPENTAC_REG_SPILLED,
};

enum {
    // these have to be `8 << OPENTAC_SIZE_XX = XX`
    // i.e. log2(bitwidth) - 3
    OPENTAC_SIZE_8 = 0,
    OPENTAC_SIZE_16 = 1,
    OPENTAC_SIZE_32 = 2,
    OPENTAC_SIZE_64 = 3,
    OPENTAC_SIZE_COUNT,
};

// machine register
struct OpentacMReg {
    uint32_t number;
    int size;
};

// generic machine register
struct OpentacGmReg {
    struct OpentacMReg regs[OPENTAC_SIZE_COUNT];
};

// machine stack-allocated value
struct OpentacMStack {
    uint64_t offset;
    int size;
    int align;
};

struct OpentacPurpose {
    int tag;
    union {
        struct OpentacMReg reg;
        struct OpentacMStack stack;
    };
};

typedef int64_t OpentacLifetime;

struct OpentacRegEntry {
    OpentacLifetime start;
    OpentacLifetime end;
    OpentacString *key;
    struct OpentacPurpose purpose;
};

struct OpentacRegisterTable {
    size_t len;
    size_t cap;
    struct OpentacRegEntry *entries;
};

struct OpentacInterval {
    const char *name;
    OpentacTypeInfo ti;
    struct OpentacPurpose purpose;
    OpentacLifetime start;
    OpentacLifetime end;
    int stack;
    int reserved;
    uint32_t reg;
    int size;
};

struct OpentacPool {
    size_t len;
    size_t cap;
    struct OpentacGmReg *registers;
};

struct OpentacIntervals {
    size_t len;
    size_t cap;
    struct OpentacInterval *intervals;
};

struct OpentacActive {
    size_t index;
    struct OpentacGmReg reg;
};

struct OpentacActives {
    size_t len;
    size_t cap;
    struct OpentacActive *actives;
};

struct OpentacRegalloc {
    struct OpentacPool registers;
    struct OpentacPool parameters;
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

void opentac_alloc_linscan(struct OpentacRegalloc *alloc, size_t len, struct OpentacGmReg *regs, size_t paramc, struct OpentacGmReg *params);
void opentac_alloc_add(struct OpentacRegalloc *alloc, struct OpentacInterval *interval);
void opentac_alloc_param(struct OpentacRegalloc *alloc, struct OpentacInterval *interval, uint32_t param);
int opentac_alloc_allocate(struct OpentacRegalloc *alloc);
void opentac_alloc_find(struct OpentacRegalloc *alloc, OpentacBuilder *builder, OpentacFnBuilder *fn);
void opentac_alloc_regtable(struct OpentacRegisterTable *dest, struct OpentacRegalloc *alloc);

void opentac_build_decl(OpentacBuilder *builder, OpentacString *name, OpentacType *type);
void opentac_build_function(OpentacBuilder *builder, OpentacString *name);
void opentac_build_function_param(OpentacBuilder *builder, OpentacString *name, OpentacRefType *type);
void opentac_finish_function(OpentacBuilder *builder);
OpentacItem *opentac_item_ptr(OpentacBuilder *builder);
void opentac_set_item(OpentacBuilder *builder, OpentacItem *item);
void opentac_builder_insert(OpentacBuilder *builder, size_t index);
void opentac_builder_goto(OpentacBuilder *builder, size_t index);
void opentac_builder_goto_end(OpentacBuilder *builder);

size_t opentac_stmt_offset(OpentacBuilder *builder, OpentacStmt *stmt);
OpentacStmt *opentac_stmt_ptr(OpentacBuilder *builder);
OpentacStmt *opentac_stmt_at(OpentacBuilder *builder, size_t offset);

OpentacValue opentac_build_alloca(OpentacBuilder *builder, OpentacType **t, uint64_t size, uint64_t align);
OpentacValue opentac_build_binary(OpentacBuilder *builder, int op, OpentacType **t, OpentacValue left, OpentacValue right);
OpentacValue opentac_build_unary(OpentacBuilder *builder, int op, OpentacType **t, OpentacValue value);
void opentac_build_index_assign(OpentacBuilder *builder, OpentacType **t, OpentacRegister target, OpentacValue offset, OpentacValue value);
OpentacValue opentac_build_assign_index(OpentacBuilder *builder, OpentacType **t, OpentacValue value, OpentacValue offset);
void opentac_build_param(OpentacBuilder *builder, OpentacType **t, OpentacValue value);
OpentacValue opentac_build_call(OpentacBuilder *builder, OpentacType **t, OpentacValue func, uint64_t nparams);
void opentac_build_return(OpentacBuilder *builder, OpentacType **t, OpentacValue value);
void opentac_build_if_branch(OpentacBuilder *builder, int relop, OpentacValue left, OpentacValue right, OpentacLabel label);
void opentac_build_branch(OpentacBuilder *builder, OpentacValue value);

void opentac_fn_insert(OpentacBuilder *builder, size_t index);
void opentac_fn_goto(OpentacBuilder *builder, size_t index);
void opentac_fn_goto_end(OpentacBuilder *builder);
void opentac_fn_bind_int(OpentacBuilder *builder, OpentacString *name, uint32_t val);
void opentac_fn_bind_ptr(OpentacBuilder *builder, OpentacString *name, void *val);
uint32_t opentac_fn_get_int(OpentacBuilder *builder, OpentacString *name);
void *opentac_fn_get_ptr(OpentacBuilder *builder, OpentacString *name);

void opentac_debug_next(struct OpentacDebugStmt *stmt, struct OpentacDebugData *data, size_t *idx);
void opentac_debug_set_file(OpentacBuilder *builder, uint32_t);
void opentac_debug_set_file(OpentacBuilder *builder, uint32_t);
void opentac_debug_set_dir(OpentacBuilder *builder, uint32_t);
void opentac_debug_set_col(OpentacBuilder *builder, uint16_t);
void opentac_debug_set_pc(OpentacBuilder *builder, uint64_t);
void opentac_debug_set_line(OpentacBuilder *builder, uint32_t);
void opentac_debug_inc_pc(OpentacBuilder *builder, uint16_t);
void opentac_debug_inc_line(OpentacBuilder *builder, int16_t);
void opentac_debug_inc(OpentacBuilder *builder, int8_t, uint8_t);
void opentac_debug_end(OpentacBuilder *builder);

OpentacType **opentac_type_ptr_of(OpentacBuilder *builder, OpentacType *type);

OpentacType **opentac_typep_unit(OpentacBuilder *builder);
OpentacType **opentac_typep_never(OpentacBuilder *builder);
OpentacType **opentac_typep_bool(OpentacBuilder *builder);
OpentacType **opentac_typep_i8(OpentacBuilder *builder);
OpentacType **opentac_typep_i16(OpentacBuilder *builder);
OpentacType **opentac_typep_i32(OpentacBuilder *builder);
OpentacType **opentac_typep_i64(OpentacBuilder *builder);
OpentacType **opentac_typep_ui8(OpentacBuilder *builder);
OpentacType **opentac_typep_ui16(OpentacBuilder *builder);
OpentacType **opentac_typep_ui32(OpentacBuilder *builder);
OpentacType **opentac_typep_ui64(OpentacBuilder *builder);
OpentacType **opentac_typep_f32(OpentacBuilder *builder);
OpentacType **opentac_typep_f64(OpentacBuilder *builder);
OpentacType **opentac_typep_named(OpentacBuilder *builder, int tag, OpentacString *name);
OpentacType **opentac_typep_ptr(OpentacBuilder *builder, OpentacType *pointee);
OpentacType **opentac_typep_fn(OpentacBuilder *builder, size_t len, OpentacRefType *params, OpentacType *result);
OpentacType **opentac_typep_tuple(OpentacBuilder *builder, uint64_t size, uint64_t align, size_t len, OpentacType **elems);
OpentacType **opentac_typep_struct(OpentacBuilder *builder, uint64_t size, uint64_t align, OpentacString *name, size_t len, OpentacType **elems, OpentacString **fields);
OpentacType **opentac_typep_union(OpentacBuilder *builder, uint64_t size, uint64_t align, OpentacString *name, size_t len, OpentacType **elems);
OpentacType **opentac_typep_array(OpentacBuilder *builder, OpentacType *elem_type, uint64_t len);

OpentacString *opentac_string(const char *str);
void opentac_del_string(OpentacString *str);

size_t opentac_fprint(FILE *out, OpentacBuilder *builder);
int opentac_pass0(OpentacBuilder *builder);

#endif /* OPENTAC_H */
