/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_GRAMMAR_TAB_H_INCLUDED
# define YY_YY_GRAMMAR_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    ERROR = 258,
    INTEGER = 259,
    REAL = 260,
    STRING = 261,
    IDENT = 262,
    KW_IF = 263,
    KW_BRANCH = 264,
    KW_RETURN = 265,
    KW_I8 = 266,
    KW_I16 = 267,
    KW_I32 = 268,
    KW_I64 = 269,
    KW_I128 = 270,
    KW_U8 = 271,
    KW_U16 = 272,
    KW_U32 = 273,
    KW_U64 = 274,
    KW_U128 = 275,
    KW_F32 = 276,
    KW_F64 = 277,
    KW_BOOL = 278,
    KW_UNIT = 279,
    KW_NEVER = 280,
    KW_STRUCT = 281,
    KW_UNION = 282,
    KW_TUPLE = 283,
    KW_PARAM = 284,
    KW_TRUE = 285,
    KW_FALSE = 286,
    SYM_DEF = 287,
    SYM_LET = 288,
    SYM_DARROW = 289,
    SYM_SARROW = 290,
    SYM_SEMICOLON = 291,
    SYM_COLON = 292,
    SYM_COMMA = 293,
    SYM_CARET = 294,
    SYM_PARENL = 295,
    SYM_PARENR = 296,
    SYM_CURLYL = 297,
    SYM_CURLYR = 298,
    SYM_SQUAREL = 299,
    SYM_SQUARER = 300,
    KW_LT = 301,
    KW_LE = 302,
    KW_EQ = 303,
    KW_NE = 304,
    KW_GT = 305,
    KW_GE = 306,
    KW_BITAND = 307,
    KW_BITXOR = 308,
    KW_BITOR = 309,
    KW_SHL = 310,
    KW_SHR = 311,
    KW_ROL = 312,
    KW_ROR = 313,
    KW_ADD = 314,
    KW_SUB = 315,
    KW_MUL = 316,
    KW_DIV = 317,
    KW_MOD = 318,
    KW_CALL = 319,
    KW_NOT = 320,
    KW_NEG = 321,
    KW_REF = 322,
    KW_DEREF = 323,
    KW_COPY = 324
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef OpentacBuilder YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_GRAMMAR_TAB_H_INCLUDED  */
