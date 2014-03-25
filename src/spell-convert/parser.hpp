/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

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

#ifndef YY_SPELL_CONVERTER_SRC_SPELL_CONVERT_PARSER_HPP_INCLUDED
# define YY_SPELL_CONVERTER_SRC_SPELL_CONVERT_PARSER_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int spell_converterdebug;
#endif
/* "%code requires" blocks.  */
#line 2 "../src/spell-convert/parser.ypp" /* yacc.c:1909  */

/* vim: set ft=yacc: */
#include "../strings/rstring.hpp"

#include "ast.hpp"

#undef YYERROR_VERBOSE
#define YYERROR_VERBOSE 1

#line 54 "src/spell-convert/parser.hpp" /* yacc.c:1909  */

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    INT = 258,
    STRING = 259,
    ID = 260,
    DIR = 261,
    CONST = 262,
    PROCEDURE = 263,
    CALL = 264,
    SILENT = 265,
    LOCAL = 266,
    NONMAGIC = 267,
    SHL = 268,
    SHR = 269,
    EQ = 270,
    NEQ = 271,
    GTE = 272,
    LTE = 273,
    ANDAND = 274,
    OROR = 275,
    SCRIPT_DATA = 276,
    TO = 277,
    TOWARDS = 278,
    TELEPORT_ANCHOR = 279,
    SPELL = 280,
    LET = 281,
    IN = 282,
    END = 283,
    DARROW = 284,
    STRING_TY = 285,
    REQUIRE = 286,
    CATALYSTS = 287,
    COMPONENTS = 288,
    MANA = 289,
    CASTTIME = 290,
    SKIP = 291,
    ABORT = 292,
    BREAK = 293,
    EFFECT_ = 294,
    ATEND = 295,
    ATTRIGGER = 296,
    PC_F = 297,
    NPC_F = 298,
    MOB_F = 299,
    ENTITY_F = 300,
    TARGET_F = 301,
    IF = 302,
    THEN = 303,
    ELSE = 304,
    FOREACH = 305,
    FOR = 306,
    DO = 307,
    SLEEP = 308,
    OR = 309
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 27 "../src/spell-convert/parser.ypp" /* yacc.c:1909  */

    RString *s;
    std::vector<RString> *vs;
    Effect *e;
    std::deque<Effect *> *ve;
    SpellDef *spelldef;
    SpellArg *spellarg;
    TopLevel *top;
    Expression *expr;
    std::vector<Expression *> *vx;
    Location *loc;
    Item *it;
    std::vector<Item *> *vit;
    Assignment *a;
    std::vector<Assignment *> *va;
    SpellBod *b;
    std::vector<SpellBod *> *vb;
    SpellGuard *g;
    std::vector<SpellGuard *> *vg;

#line 142 "src/spell-convert/parser.hpp" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE spell_converterlval;

int spell_converterparse (void);

#endif /* !YY_SPELL_CONVERTER_SRC_SPELL_CONVERT_PARSER_HPP_INCLUDED  */
