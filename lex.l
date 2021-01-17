%{
#include "include/opentac.h"
#include "grammar.tab.h"

extern OpentacString *yystrval;
extern int64_t yyival;
extern double yydval;
%}

%option noyywrap

string \"[^\n]+\"

ws [ \t\n]+

ident [A-Za-z_\.][A-Za-z_\.0-9]*
integer [0-9]+
real1 [-+]?{integer}\.?([eE][-+]?{integer})?
real2 [-+]?{integer}\.{integer}([eE][-+]?{integer})?
real {real1}|{real2}
kw_if "if"
kw_branch "branch"
kw_return "return"
kw_i8 "i8"
kw_i16 "i16"
kw_i32 "i32"
kw_i64 "i64"
kw_u8 "u8"
kw_u16 "u16"
kw_u32 "u32"
kw_u64 "u64"
kw_f32 "f32"
kw_f64 "f64"
kw_bool "bool"
kw_unit "unit"
kw_never "never"
kw_struct "struct"
kw_union "union"
kw_tuple "tuple"
kw_param "param"
kw_true "true"
kw_false "false"
sym_def "::"
sym_let ":="
sym_darrow "=>"
sym_sarrow "->"
sym_semicolon ";"
sym_colon ":"
sym_comma ","
sym_caret "^"
sym_parenl "("
sym_parenr ")"
sym_curlyl "{"
sym_curlyr "}"
sym_squarel "["
sym_squarer "]"
kw_lt "lt"
kw_le "le"
kw_eq "eq"
kw_ne "ne"
kw_gt "gt"
kw_ge "ge"
kw_bitand "bitand"
kw_bitxor "bitxor"
kw_bitor "bitor"
kw_shl "shl"
kw_shr "shr"
kw_rol "rol"
kw_ror "ror"
kw_add "add"
kw_sub "sub"
kw_mul "mul"
kw_div "div"
kw_mod "mod"
kw_call "call"
kw_not "not"
kw_neg "neg"
kw_ref "ref"
kw_deref "deref"
kw_copy "copy"

%%

{ws} /* skip */

{kw_if} { return KW_IF; }
{kw_branch} { return KW_BRANCH; }
{kw_return} { return KW_RETURN; }
{kw_i8} { return KW_I8; }
{kw_i16} { return KW_I16; }
{kw_i32} { return KW_I32; }
{kw_i64} { return KW_I64; }
{kw_u8} { return KW_U8; }
{kw_u16} { return KW_U16; }
{kw_u32} { return KW_U32; }
{kw_u64} { return KW_U64; }
{kw_f32} { return KW_F32; }
{kw_f64} { return KW_F64; }
{kw_bool} { return KW_BOOL; }
{kw_unit} { return KW_UNIT; }
{kw_never} { return KW_NEVER; }
{kw_struct} { return KW_STRUCT; }
{kw_union} { return KW_UNION; }
{kw_tuple} { return KW_TUPLE; }
{kw_param} { return KW_PARAM; }
{kw_true} { return KW_TRUE; }
{kw_false} { return KW_FALSE; }
{sym_def} { return SYM_DEF; }
{sym_let} { return SYM_LET; }
{sym_darrow} { return SYM_DARROW; }
{sym_sarrow} { return SYM_SARROW; }
{sym_semicolon} { return SYM_SEMICOLON; }
{sym_colon} { return SYM_COLON; }
{sym_comma} { return SYM_COMMA; }
{sym_caret} { return SYM_CARET; }
{sym_parenl} { return SYM_PARENL; }
{sym_parenr} { return SYM_PARENR; }
{sym_curlyl} { return SYM_CURLYL; }
{sym_curlyr} { return SYM_CURLYR; }
{sym_squarel} { return SYM_SQUAREL; }
{sym_squarer} { return SYM_SQUARER; }
{kw_lt} { return KW_LT; }
{kw_le} { return KW_LE; }
{kw_eq} { return KW_EQ; }
{kw_ne} { return KW_NE; }
{kw_gt} { return KW_GT; }
{kw_ge} { return KW_GE; }
{kw_bitand} { return KW_BITAND; }
{kw_bitxor} { return KW_BITXOR; }
{kw_bitor} { return KW_BITOR; }
{kw_shl} { return KW_SHL; }
{kw_shr} { return KW_SHR; }
{kw_rol} { return KW_ROL; }
{kw_ror} { return KW_ROR; }
{kw_add} { return KW_ADD; }
{kw_sub} { return KW_SUB; }
{kw_mul} { return KW_MUL; }
{kw_div} { return KW_DIV; }
{kw_mod} { return KW_MOD; }
{kw_call} { return KW_CALL; }
{kw_not} { return KW_NOT; }
{kw_neg} { return KW_NEG; }
{kw_ref} { return KW_REF; }
{kw_deref} { return KW_DEREF; }
{kw_copy} { return KW_COPY; }

{ident} {
  OpentacString *str = opentac_string(yytext);
  yystrval = str;
  return IDENT;
}

{integer} {
  yyival = atoll(yytext);
  return INTEGER;
}

{real} {
  yydval = atof(yytext);
  return REAL;
}

{string} {
  OpentacString *str = opentac_string(yytext);
  yystrval = str;
  return STRING;
}

. {
  fprintf(stderr, "error: lexer error: \"%s\"\n", yytext);
}

%%
