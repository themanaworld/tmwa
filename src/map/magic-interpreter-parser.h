/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
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
     EFFECT = 294,
     ATEND = 295,
     ATTRIGGER = 296,
     PC_F = 297,
     MOB_F = 298,
     ENTITY_F = 299,
     TARGET_F = 300,
     IF = 301,
     THEN = 302,
     ELSE = 303,
     FOREACH = 304,
     FOR = 305,
     DO = 306,
     SLEEP = 307,
     OR = 308
   };
#endif
/* Tokens.  */
#define INT 258
#define STRING 259
#define ID 260
#define DIR 261
#define CONST 262
#define PROCEDURE 263
#define CALL 264
#define SILENT 265
#define LOCAL 266
#define NONMAGIC 267
#define SHL 268
#define SHR 269
#define EQ 270
#define NEQ 271
#define GTE 272
#define LTE 273
#define ANDAND 274
#define OROR 275
#define SCRIPT_DATA 276
#define TO 277
#define TOWARDS 278
#define TELEPORT_ANCHOR 279
#define SPELL 280
#define LET 281
#define IN 282
#define END 283
#define DARROW 284
#define STRING_TY 285
#define REQUIRE 286
#define CATALYSTS 287
#define COMPONENTS 288
#define MANA 289
#define CASTTIME 290
#define SKIP 291
#define ABORT 292
#define BREAK 293
#define EFFECT 294
#define ATEND 295
#define ATTRIGGER 296
#define PC_F 297
#define MOB_F 298
#define ENTITY_F 299
#define TARGET_F 300
#define IF 301
#define THEN 302
#define ELSE 303
#define FOREACH 304
#define FOR 305
#define DO 306
#define SLEEP 307
#define OR 308




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 74 "magic-interpreter-parser.y"
{
    int i;
    char *s;
    int op;
    magic_conf_t *magic_conf;
    val_t value;
    expr_t *expr;
    e_location_t location;
    e_area_t area;
    args_rec_t arg_list;
    struct { int letdefs_nr; letdef_t *letdefs; } letdefs;
    spell_t *spell;
    struct { int id, ty; } spellarg_def;
    letdef_t vardef;
    spellguard_t *spellguard;
    component_t *components;
    struct {int id, count; } component;
    effect_t *effect;
    proc_t *proc;
}
/* Line 1489 of yacc.c.  */
#line 176 "magic-interpreter-parser.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE magic_frontend_lval;

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYLTYPE magic_frontend_lloc;
