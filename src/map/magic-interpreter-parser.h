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
     SHL = 267,
     SHR = 268,
     EQ = 269,
     NEQ = 270,
     GTE = 271,
     LTE = 272,
     ANDAND = 273,
     OROR = 274,
     SCRIPT_DATA = 275,
     TO = 276,
     TOWARDS = 277,
     TELEPORT_ANCHOR = 278,
     SPELL = 279,
     LET = 280,
     IN = 281,
     END = 282,
     DARROW = 283,
     STRING_TY = 284,
     REQUIRE = 285,
     CATALYSTS = 286,
     COMPONENTS = 287,
     MANA = 288,
     CASTTIME = 289,
     SKIP = 290,
     ABORT = 291,
     BREAK = 292,
     EFFECT = 293,
     ATEND = 294,
     ATTRIGGER = 295,
     PC_F = 296,
     MOB_F = 297,
     ENTITY_F = 298,
     TARGET_F = 299,
     IF = 300,
     THEN = 301,
     ELSE = 302,
     FOREACH = 303,
     FOR = 304,
     DO = 305,
     SLEEP = 306,
     OR = 307
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
#define SHL 267
#define SHR 268
#define EQ 269
#define NEQ 270
#define GTE 271
#define LTE 272
#define ANDAND 273
#define OROR 274
#define SCRIPT_DATA 275
#define TO 276
#define TOWARDS 277
#define TELEPORT_ANCHOR 278
#define SPELL 279
#define LET 280
#define IN 281
#define END 282
#define DARROW 283
#define STRING_TY 284
#define REQUIRE 285
#define CATALYSTS 286
#define COMPONENTS 287
#define MANA 288
#define CASTTIME 289
#define SKIP 290
#define ABORT 291
#define BREAK 292
#define EFFECT 293
#define ATEND 294
#define ATTRIGGER 295
#define PC_F 296
#define MOB_F 297
#define ENTITY_F 298
#define TARGET_F 299
#define IF 300
#define THEN 301
#define ELSE 302
#define FOREACH 303
#define FOR 304
#define DO 305
#define SLEEP 306
#define OR 307




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 71 "magic-interpreter-parser.y"
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
#line 174 "magic-interpreter-parser.h"
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
