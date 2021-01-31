%{
#include "include/opentac.h"
#define DEFAULT_TYPES_CAP 8
#define YYERROR_VERBOSE 1

int yylex(void);
void yyerror(char const *);

OpentacBuilder *opentac_b = NULL;
OpentacString *yystrval = NULL;
unsigned int yyopval;
int64_t yyival[2];
size_t yyivalc = 0;
double yydval = 0.0;
int yystatus = 0;
static OpentacString *yydeclname;
static OpentacString *yyargname;
static OpentacString *yyregval;
static OpentacString *yylblval;
static int yyvalc = 0;
static OpentacValue yyvals[2];
static int yytvalc = 0;
static OpentacType **yytval[4];
static size_t yytplen;
static size_t yytpcap;
static OpentacType **yytpval;
static size_t yyrtplen;
static size_t yyrtpcap;
static OpentacRefType *yyrtpval;
%}

%define api.value.type {OpentacBuilder}
%token ERROR
%token INTEGER
%token REAL
%token STRING
%token IDENT
%token KW_IN
%token KW_OUT
%token KW_ALLOCA
%token KW_IF
%token KW_BRANCH
%token KW_RETURN
%token KW_I8
%token KW_I16
%token KW_I32
%token KW_I64
%token KW_I128
%token KW_U8
%token KW_U16
%token KW_U32
%token KW_U64
%token KW_U128
%token KW_F32
%token KW_F64
%token KW_BOOL
%token KW_UNIT
%token KW_NEVER
%token KW_STRUCT
%token KW_UNION
%token KW_TUPLE
%token KW_PARAM
%token KW_TRUE
%token KW_FALSE
%token SYM_DEF
%token SYM_LET
%token SYM_DARROW
%token SYM_SARROW
%token SYM_SEMICOLON
%token SYM_COLON
%token SYM_COMMA
%token SYM_CARET
%token SYM_PARENL
%token SYM_PARENR
%token SYM_CURLYL
%token SYM_CURLYR
%token SYM_SQUAREL
%token SYM_SQUARER
%token KW_LT
%token KW_LE
%token KW_EQ
%token KW_NE
%token KW_GT
%token KW_GE
%token KW_BITAND
%token KW_BITXOR
%token KW_BITOR
%token KW_SHL
%token KW_SHR
%token KW_ROL
%token KW_ROR
%token KW_ADD
%token KW_SUB
%token KW_MUL
%token KW_DIV
%token KW_MOD
%token KW_CALL
%token KW_NOT
%token KW_NEG
%token KW_REF
%token KW_DEREF
%token KW_COPY
                                                                        
%%

root:
                %empty
        |       root item
        ;

item:
                declaration
        |       function
        ;

declaration:
                declname SYM_COLON type SYM_SEMICOLON {
                    opentac_build_decl(opentac_b, yydeclname, *yytval[--yytvalc]);
                  }
        ;

function:
                fndef SYM_DARROW SYM_CURLYL stmts SYM_CURLYR {
                    opentac_finish_function(opentac_b);
                  }
        ;

fndef:
                fndefname SYM_DEF paramslist
        ;

declname:
                IDENT { yydeclname = yystrval; }
        ;

fndefname:
                IDENT {
                    opentac_build_function(opentac_b, yystrval);
                  }
        ;

paramslist:
                SYM_PARENL SYM_PARENR
        |       SYM_PARENL paramslist_inner SYM_PARENR
        |       SYM_PARENL paramslist_inner SYM_COMMA SYM_PARENR
        ;

paramslist_inner:
                arg
        |       paramslist_inner SYM_COMMA arg
        ;

arg:
                argname SYM_COLON type {
                    OpentacRefType param = { .type = *yytval[--yytvalc], .ref = OPENTAC_REF_NONE };
                    opentac_build_function_param(opentac_b, yyargname, &param);
                  }
        |       KW_IN argname SYM_COLON type {
                    OpentacRefType param = { .type = *yytval[--yytvalc], .ref = OPENTAC_REF_IN };
                    opentac_build_function_param(opentac_b, yyargname, &param);
                  }
        |       KW_OUT argname SYM_COLON type {
                    OpentacRefType result = { .type = *yytval[--yytvalc], .ref = OPENTAC_REF_OUT };
                    opentac_build_function_param(opentac_b, yyargname, &result);
                  }
        ;

argname:
                IDENT { yyargname = yystrval; }
        ;

type:
                KW_I8 { yytval[yytvalc++] = opentac_typep_i8(opentac_b); }
        |       KW_I16 { yytval[yytvalc++] = opentac_typep_i16(opentac_b); }
        |       KW_I32 { yytval[yytvalc++] = opentac_typep_i32(opentac_b); }
        |       KW_I64 { yytval[yytvalc++] = opentac_typep_i64(opentac_b); }
        |       KW_U8 { yytval[yytvalc++] = opentac_typep_ui8(opentac_b); }
        |       KW_U16 { yytval[yytvalc++] = opentac_typep_ui16(opentac_b); }
        |       KW_U32 { yytval[yytvalc++] = opentac_typep_ui32(opentac_b); }
        |       KW_U64 { yytval[yytvalc++] = opentac_typep_ui64(opentac_b); }
        |       KW_F32 { yytval[yytvalc++] = opentac_typep_f32(opentac_b); }
        |       KW_F64 { yytval[yytvalc++] = opentac_typep_f64(opentac_b); }
        |       KW_BOOL { yytval[yytvalc++] = opentac_typep_bool(opentac_b); }
        |       KW_UNIT { yytval[yytvalc++] = opentac_typep_unit(opentac_b); }
        |       KW_NEVER { yytval[yytvalc++] = opentac_typep_never(opentac_b); }
        |       SYM_CARET type {
                    OpentacType *t = *yytval[--yytvalc];
                    yytval[yytvalc++] = opentac_typep_ptr(opentac_b, t);
                  }
        |       KW_TUPLE typelist1 {
                    // TODO: size and alignment
                    yytval[yytvalc++] = opentac_typep_tuple(opentac_b, 0, 1, yytplen, yytpval);
                  }
        |       KW_STRUCT IDENT {
                    yytval[yytvalc++] = opentac_typep_named(opentac_b, OPENTAC_TYPE_STRUCT, yystrval);
                  }
        |       KW_UNION IDENT {
                    yytval[yytvalc++] = opentac_typep_named(opentac_b, OPENTAC_TYPE_UNION, yystrval);
                  }
        |       SYM_SQUAREL type SYM_COMMA INTEGER SYM_SQUARER {
                    OpentacType *t = *yytval[--yytvalc];
                    yytval[yytvalc++] = opentac_typep_array(opentac_b, t, yyival[0]);
                    yyivalc = 0;
                  }
        |       reftypelist0 SYM_SARROW type {
                    OpentacType *t = *yytval[--yytvalc];
                    yytval[yytvalc++] = opentac_typep_fn(opentac_b, yyrtplen, yyrtpval, t);
                  }
        ;

reftypelist0:
                SYM_PARENL SYM_PARENR {
                    yyrtplen = 0;
                    yyrtpval = malloc(sizeof(OpentacType *));
                  }
        |       SYM_PARENL reftypelist_inner SYM_PARENR
        |       SYM_PARENL reftypelist_inner SYM_COMMA SYM_PARENR
        ;

typelist1:
                SYM_PARENL typelist_inner SYM_PARENR
        |       SYM_PARENL typelist_inner SYM_COMMA SYM_PARENR
        ;

typelist_inner:
                type {
                    yytpcap = DEFAULT_TYPES_CAP;
                    yytplen = 0;
                    yytpval = malloc(yytpcap * sizeof(OpentacType *));
                    yytpval[yytplen++] = *yytval[--yytvalc];
                  }
        |       typelist_inner SYM_COMMA type {
                    if (yytplen == yytpcap) {
                      yytpcap *= 2;
                      yytpval = realloc(yytpval, yytpcap * sizeof(OpentacType *));
                    }
                    yytpval[yytplen++] = *yytval[--yytvalc];
                  }
        ;

reftypelist_inner:
                type {
                    yyrtpcap = DEFAULT_TYPES_CAP;
                    yyrtplen = 0;
                    yyrtpval = malloc(yyrtpcap * sizeof(OpentacRefType));
                    yyrtpval[yyrtplen].type = *yytval[--yytvalc];
                    yyrtpval[yyrtplen++].ref = OPENTAC_REF_NONE;
                  }
        |      KW_IN type {
                    yyrtpcap = DEFAULT_TYPES_CAP;
                    yyrtplen = 0;
                    yyrtpval = malloc(yyrtpcap * sizeof(OpentacRefType));
                    yyrtpval[yyrtplen].type = *yytval[--yytvalc];
                    yyrtpval[yyrtplen++].ref = OPENTAC_REF_IN;
                  }
        |      KW_OUT type {
                    yyrtpcap = DEFAULT_TYPES_CAP;
                    yyrtplen = 0;
                    yyrtpval = malloc(yyrtpcap * sizeof(OpentacRefType));
                    yyrtpval[yyrtplen].type = *yytval[--yytvalc];
                    yyrtpval[yyrtplen++].ref = OPENTAC_REF_OUT;
                  }
        |       reftypelist_inner SYM_COMMA type {
                    if (yyrtplen == yyrtpcap) {
                      yyrtpcap *= 2;
                      yyrtpval = realloc(yyrtpval, yyrtpcap * sizeof(OpentacRefType));
                    }
                    yyrtpval[yyrtplen].type = *yytval[--yytvalc];
                    yyrtpval[yyrtplen++].ref = OPENTAC_REF_NONE;
                  }
        |       reftypelist_inner SYM_COMMA KW_IN type {
                    if (yyrtplen == yyrtpcap) {
                      yyrtpcap *= 2;
                      yyrtpval = realloc(yyrtpval, yyrtpcap * sizeof(OpentacRefType));
                    }
                    yyrtpval[yyrtplen].type = *yytval[--yytvalc];
                    yyrtpval[yyrtplen++].ref = OPENTAC_REF_IN;
                  }
        |       reftypelist_inner SYM_COMMA KW_OUT type {
                    if (yyrtplen == yyrtpcap) {
                      yyrtpcap *= 2;
                      yyrtpval = realloc(yyrtpval, yyrtpcap * sizeof(OpentacRefType));
                    }
                    yyrtpval[yyrtplen].type = *yytval[--yytvalc];
                    yyrtpval[yyrtplen++].ref = OPENTAC_REF_OUT;
                  }
        ;

stmts:
                %empty
        |       stmts stmt
        ;

stmt:
                reg SYM_LET binary type SYM_COMMA value SYM_COMMA value SYM_SEMICOLON {
                    OpentacValue target = opentac_build_binary(opentac_b, yyopval, yytval[--yytvalc], yyvals[0], yyvals[1]);
                    opentac_fn_bind_int(opentac_b, yyregval, target.val.regval);
                    yyvalc = 0;
                  }
        |       reg SYM_LET unary type SYM_COMMA value SYM_SEMICOLON {
                    OpentacValue target = opentac_build_unary(opentac_b, yyopval, yytval[--yytvalc], yyvals[0]);
                    opentac_fn_bind_int(opentac_b, yyregval, target.val.regval);
                    yyvalc = 0;
                  }
        |       reg SYM_LET KW_ALLOCA type SYM_COMMA INTEGER SYM_COMMA INTEGER SYM_SEMICOLON {
                    OpentacValue target = opentac_build_alloca(opentac_b, yytval[--yytvalc], yyival[0], yyival[1]);
                    opentac_fn_bind_int(opentac_b, yyregval, target.val.regval);
                    yyivalc = 0;
                    yyvalc = 0;
                  }
        |       reg SYM_SQUAREL value SYM_SQUARER SYM_LET value SYM_SEMICOLON {
                    OpentacRegister reg = opentac_fn_get_int(opentac_b, yyregval);
                    opentac_del_string(yyregval);
                    opentac_build_index_assign(opentac_b, reg, yyvals[0], yyvals[1]);
                    yyvalc = 0;
                  }
        |       reg SYM_LET value SYM_SQUAREL value SYM_SQUARER SYM_SEMICOLON {
                    OpentacValue target = opentac_build_assign_index(opentac_b, yyvals[0], yyvals[1]);
                    opentac_fn_bind_int(opentac_b, yyregval, target.val.regval);
                    yyvalc = 0;
                  }
        |       KW_PARAM type SYM_COMMA value SYM_SEMICOLON {
                    opentac_build_param(opentac_b, yytval[--yytvalc], yyvals[0]);
                    yyvalc = 0;
                  }
        |       KW_RETURN type SYM_COMMA value SYM_SEMICOLON {
                    opentac_build_return(opentac_b, yytval[--yytvalc], yyvals[0]);
                    yyvalc = 0;
                  }
        |       KW_IF binary value SYM_COMMA value KW_BRANCH label SYM_SEMICOLON {
                    OpentacLabel label = opentac_fn_get_int(opentac_b, yylblval);
                    opentac_del_string(yyregval);
                    opentac_build_if_branch(opentac_b, yyopval, yyvals[0], yyvals[1], label);
                    yyvalc = 0;
                  }
        |       KW_BRANCH value SYM_SEMICOLON {
                    opentac_build_branch(opentac_b, yyvals[0]);
                    yyvalc = 0;
                  }
        ;

reg:
                IDENT { yyregval = yystrval; }
        ;

label:
                IDENT { yylblval = yystrval; }
        ;

value:
                IDENT {
                    yyvals[yyvalc].tag = OPENTAC_VAL_NAMED;
                    yyvals[yyvalc++].val.name = yystrval;
                  }
        |       INTEGER SYM_COLON type {
                    switch ((*yytval[--yytvalc])->tag) {
                    case OPENTAC_TYPE_I8:
                      yyvals[yyvalc].tag = OPENTAC_VAL_I8;
                      yyvals[yyvalc++].val.i8val = yyival[0];
                      yyivalc = 0;
                      break;
                    case OPENTAC_TYPE_I16:
                      yyvals[yyvalc].tag = OPENTAC_VAL_I16;
                      yyvals[yyvalc++].val.i16val = yyival[0];
                      yyivalc = 0;
                      break;
                    case OPENTAC_TYPE_I32:
                      yyvals[yyvalc].tag = OPENTAC_VAL_I32;
                      yyvals[yyvalc++].val.i32val = yyival[0];
                      yyivalc = 0;
                      break;
                    case OPENTAC_TYPE_I64:
                      yyvals[yyvalc].tag = OPENTAC_VAL_I64;
                      yyvals[yyvalc++].val.i64val = yyival[0];
                      yyivalc = 0;
                      break;
                    case OPENTAC_TYPE_UI8:
                      yyvals[yyvalc].tag = OPENTAC_VAL_UI8;
                      yyvals[yyvalc++].val.ui8val = yyival[0];
                      yyivalc = 0;
                      break;
                    case OPENTAC_TYPE_UI16:
                      yyvals[yyvalc].tag = OPENTAC_VAL_UI16;
                      yyvals[yyvalc++].val.ui16val = yyival[0];
                      yyivalc = 0;
                      break;
                    case OPENTAC_TYPE_UI32:
                      yyvals[yyvalc].tag = OPENTAC_VAL_UI32;
                      yyvals[yyvalc++].val.ui32val = yyival[0];
                      yyivalc = 0;
                      break;
                    case OPENTAC_TYPE_UI64:
                      yyvals[yyvalc].tag = OPENTAC_VAL_UI64;
                      yyvals[yyvalc++].val.ui64val = yyival[0];
                      yyivalc = 0;
                      break;
                    case OPENTAC_TYPE_F32:
                      yyvals[yyvalc].tag = OPENTAC_VAL_F32;
                      yyvals[yyvalc++].val.fval = (float) yyival[0];
                      yyivalc = 0;
                      break;
                    case OPENTAC_TYPE_F64:
                      yyvals[yyvalc].tag = OPENTAC_VAL_F64;
                      yyvals[yyvalc++].val.dval = (double) yyival[0];
                      yyivalc = 0;
                      break;
                    default:
                      yyerror("type error: integers can only be declared as integer or floating point types");
                      yystatus = 1;
                      yyvals[yyvalc++].tag = OPENTAC_VAL_ERROR;
                      break;
                    }
                  }
        |       REAL SYM_COLON type {
                    switch ((*yytval[--yytvalc])->tag) {
                    case OPENTAC_TYPE_F32:
                      yyvals[yyvalc].tag = OPENTAC_VAL_F32;
                      yyvals[yyvalc++].val.fval = (float) yydval;
                      break;
                    case OPENTAC_TYPE_F64:
                      yyvals[yyvalc].tag = OPENTAC_VAL_F64;
                      yyvals[yyvalc++].val.dval = yydval;
                      break;
                    default:
                      yyerror("type error: real numbers can only be declared as floating point types");
                      yystatus = 1;
                      yyvals[yyvalc++].tag = OPENTAC_VAL_ERROR;
                      break;
                    }
                  }
        |       KW_TRUE {
                    yyvals[yyvalc].tag = OPENTAC_VAL_BOOL;
                    yyvals[yyvalc++].val.bval = 1;
                  }
        |       KW_FALSE {
                    yyvals[yyvalc].tag = OPENTAC_VAL_BOOL;
                    yyvals[yyvalc++].val.bval = 0;
                  }
        |       KW_UNIT {
                    yyvals[yyvalc++].tag = OPENTAC_VAL_UNIT;
                  }
        ;

binary:
	  	KW_LT { yyopval = OPENTAC_OP_LT; }
	| 	KW_LE { yyopval = OPENTAC_OP_LE; }
	| 	KW_EQ { yyopval = OPENTAC_OP_EQ; }
	| 	KW_NE { yyopval = OPENTAC_OP_NE; }
	| 	KW_GT { yyopval = OPENTAC_OP_GT; }
	| 	KW_GE { yyopval = OPENTAC_OP_GE; }
	| 	KW_BITAND { yyopval = OPENTAC_OP_BITAND; }
	| 	KW_BITXOR { yyopval = OPENTAC_OP_BITXOR; }
	| 	KW_BITOR { yyopval = OPENTAC_OP_BITOR; }
	| 	KW_SHL { yyopval = OPENTAC_OP_SHL; }
	| 	KW_SHR { yyopval = OPENTAC_OP_SHR; }
	| 	KW_ROL { yyopval = OPENTAC_OP_ROL; }
	| 	KW_ROR { yyopval = OPENTAC_OP_ROR; }
	| 	KW_ADD { yyopval = OPENTAC_OP_ADD; }
	| 	KW_SUB { yyopval = OPENTAC_OP_SUB; }
	| 	KW_MUL { yyopval = OPENTAC_OP_MUL; }
	| 	KW_DIV { yyopval = OPENTAC_OP_DIV; }
	| 	KW_MOD { yyopval = OPENTAC_OP_MOD; }
	| 	KW_CALL { yyopval = OPENTAC_OP_CALL; }
;

unary:
	  	KW_NOT { yyopval = OPENTAC_OP_NOT; }
	| 	KW_NEG { yyopval = OPENTAC_OP_NEG; }
	| 	KW_REF { yyopval = OPENTAC_OP_REF; }
	| 	KW_DEREF { yyopval = OPENTAC_OP_DEREF; }
	| 	KW_COPY { yyopval = OPENTAC_OP_COPY; }
        ;

%%
