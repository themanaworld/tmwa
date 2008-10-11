/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 1

/* Substitute the variable and function names.  */
#define yyparse magic_frontend_parse
#define yylex   magic_frontend_lex
#define yyerror magic_frontend_error
#define yylval  magic_frontend_lval
#define yychar  magic_frontend_char
#define yydebug magic_frontend_debug
#define yynerrs magic_frontend_nerrs
#define yylloc magic_frontend_lloc

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




/* Copy the first part of user declarations.  */
#line 1 "magic-interpreter-parser.y"

#include "magic-interpreter.h"
#include "magic-expr.h"
#include <stdarg.h>

magic_conf_t magic_conf;

static int
intern_id(char *id_name);


static expr_t *
fun_expr(char *name, int args_nr, expr_t **args, int line, int column);

#define BIN_EXPR(x, name, arg1, arg2, line, column) { expr_t *e[2]; e[0] = arg1; e[1] = arg2; x = fun_expr(name, 2, e, line, column); }

static int failed_flag = 0;

static void
magic_frontend_error(const char *msg);

static void
fail(int line, int column, char *fmt, ...);

static spell_t *
new_spell(spellguard_t *guard);

static spellguard_t *
spellguard_implication(spellguard_t *a, spellguard_t *b);

static spellguard_t *
new_spellguard(int ty);

static effect_t *
new_effect(int ty);

static effect_t *
set_effect_continuation(effect_t *src, effect_t *continuation);

static void
add_spell(spell_t *spell, int line_nr);

static void
add_teleport_anchor(teleport_anchor_t *anchor, int line_nr);

static effect_t *
op_effect(char *name, int args_nr, expr_t **args, int line, int column);

int
magic_frontend_lex();

static void
install_proc(proc_t *proc);

static effect_t *
call_proc(char *name, int args_nr, expr_t **args, int line_nr, int column);

static void
bind_constant(char *name, val_t *val, int line_nr);

static val_t *
find_constant(char *name);




/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

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
/* Line 187 of yacc.c.  */
#line 295 "magic-interpreter-parser.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

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


/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 320 "magic-interpreter-parser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  16
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   920

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  72
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  32
/* YYNRULES -- Number of rules.  */
#define YYNRULES  108
/* YYNRULES -- Number of states.  */
#define YYNSTATES  247

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   307

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,    14,    22,     2,
      70,    71,    12,    10,    16,    11,     2,    13,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    17,    18,
       8,     7,     9,     2,    15,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    20,     2,    21,    23,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    19,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     8,     9,    12,    13,    15,    17,
      21,    25,    30,    37,    45,    54,    55,    58,    61,    62,
      68,    70,    72,    74,    76,    78,    80,    82,    84,    88,
      92,    96,   100,   104,   108,   112,   116,   120,   124,   128,
     132,   136,   140,   144,   148,   152,   156,   160,   165,   169,
     170,   172,   174,   178,   187,   189,   198,   208,   210,   215,
     217,   221,   225,   227,   231,   235,   239,   244,   245,   248,
     249,   252,   254,   258,   262,   264,   268,   271,   274,   277,
     280,   283,   287,   289,   293,   297,   299,   301,   303,   305,
     307,   309,   311,   313,   317,   320,   323,   326,   329,   334,
     342,   351,   358,   363,   367,   373,   375,   382,   383
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      73,     0,    -1,    -1,    77,    74,    73,    -1,    -1,    74,
      18,    -1,    -1,    76,    -1,     5,    -1,    76,    16,     5,
      -1,     5,     7,    82,    -1,    24,     5,     7,    82,    -1,
      40,     5,    17,    82,     7,    82,    -1,    25,     5,    70,
      75,    71,     7,   103,    -1,    78,    41,     5,    79,    17,
      82,     7,    87,    -1,    -1,    28,    78,    -1,    27,    78,
      -1,    -1,    70,     5,    17,    80,    71,    -1,    58,    -1,
      46,    -1,     6,    -1,     3,    -1,     4,    -1,    81,    -1,
       5,    -1,    86,    -1,    82,    10,    82,    -1,    82,    11,
      82,    -1,    82,    12,    82,    -1,    82,    14,    82,    -1,
      82,    13,    82,    -1,    82,     8,    82,    -1,    82,     9,
      82,    -1,    82,    22,    82,    -1,    82,    23,    82,    -1,
      82,    19,    82,    -1,    82,    29,    82,    -1,    82,    30,
      82,    -1,    82,    34,    82,    -1,    82,    33,    82,    -1,
      82,    35,    82,    -1,    82,    36,    82,    -1,    82,    31,
      82,    -1,    82,     7,    82,    -1,    82,    32,    82,    -1,
       5,    70,    83,    71,    -1,    70,    82,    71,    -1,    -1,
      84,    -1,    82,    -1,    84,    16,    82,    -1,    15,    70,
      82,    16,    82,    16,    82,    71,    -1,    85,    -1,    85,
      15,    10,    70,    82,    16,    82,    71,    -1,    85,    39,
      82,    17,    70,    82,    16,    82,    71,    -1,    90,    -1,
      42,    88,    43,    90,    -1,    74,    -1,    88,    89,    74,
      -1,     5,     7,    82,    -1,    91,    -1,    91,    19,    90,
      -1,    94,    45,    91,    -1,    70,    90,    71,    -1,    55,
     103,    92,    93,    -1,    -1,    57,   103,    -1,    -1,    56,
     103,    -1,    96,    -1,    94,    69,    94,    -1,    70,    95,
      71,    -1,    94,    -1,    94,    16,    95,    -1,    47,    82,
      -1,    48,    97,    -1,    49,    97,    -1,    50,    82,    -1,
      51,    82,    -1,    20,    98,    21,    -1,    99,    -1,    98,
      16,    99,    -1,     3,    12,   100,    -1,   100,    -1,     4,
      -1,     3,    -1,    58,    -1,    59,    -1,    60,    -1,    41,
      -1,    61,    -1,    70,   103,    71,    -1,    52,    18,    -1,
      53,    18,    -1,    44,    18,    -1,    54,    18,    -1,     5,
       7,    82,    18,    -1,    65,   101,     5,    43,    82,    67,
     102,    -1,    66,     5,     7,    82,    38,    82,    67,   102,
      -1,    62,    82,    63,   102,    64,   102,    -1,    62,    82,
      63,   102,    -1,    68,    82,    18,    -1,     5,    70,    83,
      71,    18,    -1,    37,    -1,    26,     5,    70,    83,    71,
      18,    -1,    -1,   102,    74,   103,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   206,   206,   207,   213,   214,   220,   221,   225,   231,
     238,   249,   255,   266,   275,   288,   289,   294,   302,   303,
     309,   311,   316,   319,   322,   328,   331,   342,   345,   347,
     349,   351,   353,   355,   357,   359,   361,   363,   365,   367,
     369,   371,   373,   375,   377,   379,   381,   384,   389,   394,
     395,   400,   405,   412,   416,   420,   426,   436,   438,   447,
     451,   460,   473,   475,   484,   486,   488,   499,   500,   506,
     507,   512,   514,   520,   525,   527,   532,   536,   540,   544,
     548,   555,   560,   564,   571,   573,   578,   587,   592,   594,
     596,   598,   600,   605,   607,   609,   611,   613,   615,   626,
     633,   640,   646,   652,   656,   660,   667,   674,   675
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "STRING", "ID", "DIR", "'='",
  "'<'", "'>'", "'+'", "'-'", "'*'", "'/'", "'%'", "'@'", "','", "':'",
  "';'", "'|'", "'['", "']'", "'&'", "'^'", "CONST", "PROCEDURE", "CALL",
  "SILENT", "LOCAL", "SHL", "SHR", "EQ", "NEQ", "GTE", "LTE", "ANDAND",
  "OROR", "SCRIPT_DATA", "TO", "TOWARDS", "TELEPORT_ANCHOR", "SPELL",
  "LET", "IN", "END", "DARROW", "STRING_TY", "REQUIRE", "CATALYSTS",
  "COMPONENTS", "MANA", "CASTTIME", "SKIP", "ABORT", "BREAK", "EFFECT",
  "ATEND", "ATTRIGGER", "PC_F", "MOB_F", "ENTITY_F", "TARGET_F", "IF",
  "THEN", "ELSE", "FOREACH", "FOR", "DO", "SLEEP", "OR", "'('", "')'",
  "$accept", "spellconf", "semicolons", "proc_formals_list",
  "proc_formals_list_ne", "spellconf_option", "spell_flags", "argopt",
  "arg_ty", "value", "expr", "arg_list", "arg_list_ne", "location", "area",
  "spelldef", "defs", "def", "spellbody_list", "spellbody",
  "maybe_trigger", "maybe_end", "spellguard", "spellguard_list", "prereq",
  "items", "item_list", "item", "item_name", "selection", "effect",
  "effect_list", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,    61,    60,    62,
      43,    45,    42,    47,    37,    64,    44,    58,    59,   124,
      91,    93,    38,    94,   262,   263,   264,   265,   266,   267,
     268,   269,   270,   271,   272,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,   306,   307,
      40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    72,    73,    73,    74,    74,    75,    75,    76,    76,
      77,    77,    77,    77,    77,    78,    78,    78,    79,    79,
      80,    80,    81,    81,    81,    82,    82,    82,    82,    82,
      82,    82,    82,    82,    82,    82,    82,    82,    82,    82,
      82,    82,    82,    82,    82,    82,    82,    82,    82,    83,
      83,    84,    84,    85,    86,    86,    86,    87,    87,    88,
      88,    89,    90,    90,    91,    91,    91,    92,    92,    93,
      93,    94,    94,    94,    95,    95,    96,    96,    96,    96,
      96,    97,    98,    98,    99,    99,   100,   100,   101,   101,
     101,   101,   101,   102,   102,   102,   102,   102,   102,   102,
     102,   102,   102,   102,   102,   102,   102,   103,   103
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     3,     0,     2,     0,     1,     1,     3,
       3,     4,     6,     7,     8,     0,     2,     2,     0,     5,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     4,     3,     0,
       1,     1,     3,     8,     1,     8,     9,     1,     4,     1,
       3,     3,     1,     3,     3,     3,     4,     0,     2,     0,
       2,     1,     3,     3,     1,     3,     2,     2,     2,     2,
       2,     3,     1,     3,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     2,     2,     2,     2,     4,     7,
       8,     6,     4,     3,     5,     1,     6,     0,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     0,     0,    15,    15,     0,     0,     4,     0,
       0,     0,     0,    17,    16,     0,     1,     2,     0,    23,
      24,    26,    22,     0,     0,    25,    10,    54,    27,     0,
       6,     0,     5,     3,    18,    49,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    11,
       8,     0,     7,     0,     0,     0,    51,     0,    50,     0,
      48,    45,    33,    34,    28,    29,    30,    32,    31,    37,
      35,    36,    38,    39,    44,    46,    41,    40,    42,    43,
       0,     0,     0,     0,     0,     0,     0,    47,     0,     0,
       0,     0,   107,     9,    45,     0,     0,    52,     0,     0,
       0,     0,     0,   105,     0,     0,     0,     0,     0,     0,
       0,     0,   107,     4,    13,    21,    20,     0,     0,     0,
       0,     0,     0,    49,     0,    96,    94,    95,    97,     0,
      91,    88,    89,    90,    92,     0,     0,     0,     0,   107,
      19,     4,     0,     0,     0,     0,     0,   107,     0,    14,
      57,    62,     0,    71,     0,     0,     0,     0,     0,    49,
       0,     0,     0,   103,    93,   108,    59,     0,    76,     0,
      77,    78,    79,    80,    67,     0,    74,     0,     0,     0,
       0,    53,    55,     0,    98,     0,     0,   102,     0,     0,
       0,     0,     4,    87,    86,     0,    82,    85,   107,    69,
      65,     0,    73,     0,    63,    64,     0,    72,    56,   104,
       0,     0,     0,     0,     0,    58,    60,     0,     0,    81,
      68,   107,    66,    74,    75,   106,   101,     0,     0,    61,
      87,    84,    83,    70,    99,     0,   100
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     7,    17,    61,    62,     8,     9,    65,   127,    25,
      66,    67,    68,    27,    28,   159,   177,   202,   185,   161,
     209,   232,   162,   187,   163,   180,   205,   206,   207,   145,
     123,   124
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -170
static const yytype_int16 yypact[] =
{
     100,     5,    18,    53,   -18,   -18,    56,    16,  -170,    26,
       0,    67,    10,  -170,  -170,    64,  -170,   150,    80,  -170,
    -170,    17,  -170,    27,     0,  -170,   730,    12,  -170,     0,
      91,     0,  -170,  -170,    28,     0,     0,   238,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    89,     0,   730,
    -170,    31,    93,   760,    99,    90,   730,    39,   101,   520,
    -170,   109,   877,   877,   890,   890,   236,   236,   236,   109,
     109,   109,   109,   109,   877,   877,   877,   877,   849,   820,
      51,   550,   119,   118,     0,   114,     0,  -170,     0,     0,
       0,    63,   174,  -170,   288,    11,   790,   730,   580,   610,
       0,     1,   127,  -170,   126,   129,   135,   136,     0,    54,
     152,     0,   174,  -170,  -170,  -170,  -170,    98,    88,     0,
       0,   640,     0,     0,   102,  -170,  -170,  -170,  -170,   453,
    -170,  -170,  -170,  -170,  -170,   154,   163,   670,   105,   155,
    -170,  -170,     0,   160,   160,     0,     0,   174,   146,  -170,
    -170,   115,    14,  -170,   268,   311,     0,   700,   111,     0,
     174,   140,     0,  -170,  -170,  -170,   166,     6,   730,    60,
    -170,  -170,   730,   730,   128,   131,     9,   132,   116,   116,
     183,  -170,  -170,   341,  -170,   169,   133,   125,     0,   488,
     191,   116,  -170,   193,  -170,    -3,  -170,  -170,   174,   156,
    -170,   183,  -170,   116,  -170,  -170,   183,  -170,  -170,  -170,
     188,   174,   384,     0,     0,  -170,   166,    62,    60,  -170,
    -170,   174,  -170,    -9,  -170,  -170,  -170,   174,   414,   730,
    -170,  -170,  -170,  -170,  -170,   174,  -170
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -170,   198,  -101,  -170,  -170,  -170,    68,  -170,  -170,  -170,
     -10,  -116,  -170,  -170,  -170,  -170,  -170,  -170,  -126,    21,
    -170,  -170,  -134,     8,  -170,    70,  -170,    -6,     2,  -170,
    -169,  -102
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -16
static const yytype_int16 yytable[] =
{
      26,   197,   160,    19,    20,    21,    22,   211,   132,     4,
       5,   200,    10,   228,    37,    23,    16,   168,   229,    59,
     148,    63,   149,    11,   186,   211,    69,    57,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,   175,    91,   201,
     176,    58,   236,   196,   189,   184,   217,   125,    12,   189,
     190,    15,   214,   203,   204,   240,   204,    18,   244,   126,
      24,   133,    13,    14,    29,   225,   246,   233,   190,   186,
      30,    31,   233,   190,   104,    34,   106,    35,   107,   108,
     109,    19,    20,    21,    22,   140,    60,    36,    64,    90,
     131,   226,    92,    23,    95,     1,   230,    96,   139,    93,
      97,   147,   141,   142,   143,   144,    38,    98,    71,   164,
     165,   100,   167,   103,     2,     3,   102,     4,     5,   243,
     151,   105,   134,   110,   188,   152,   153,   154,   155,   156,
       6,   -15,   178,   157,   135,   182,   183,   136,    37,    19,
      20,    21,    22,   137,   138,     1,   193,   146,   158,   171,
     111,    23,   199,   152,   153,   154,   155,   156,    32,   150,
     172,   157,   169,    32,     2,     3,   174,     4,     5,   111,
     179,   112,   195,   198,    32,   208,   213,   219,   222,   221,
       6,   -15,   113,   152,   153,   154,   155,   156,   224,   114,
     112,   157,   210,   212,   220,   227,   235,   115,   116,   117,
     215,   113,   231,   238,   239,    33,   158,   118,   114,   234,
     119,   120,   242,   121,   181,   122,   115,   116,   117,   241,
     152,   153,   154,   155,   156,     0,   118,     0,     0,   119,
     120,     0,   121,    38,   122,    38,    39,    40,    41,    42,
      43,    44,    45,   216,     0,    46,     0,    46,    47,    48,
      47,    48,     0,     0,     0,    49,    50,    49,    50,    51,
      52,    53,    54,    55,    56,    38,    39,    40,    41,    42,
      43,    44,    45,     0,     0,     0,     0,    46,   -12,     0,
      47,    48,     0,   -12,     0,    38,     0,    49,    50,    51,
      52,    53,    54,    55,    56,     0,   -12,     0,     0,    70,
       0,     0,   -12,   -12,     0,   -12,   -12,     0,    38,    39,
      40,    41,    42,    43,    44,    45,     0,     0,   -12,   -12,
      46,     0,     0,    47,    48,     0,     0,     0,     0,   191,
      49,    50,    51,    52,    53,    54,    55,    56,    38,    39,
      40,    41,    42,    43,    44,    45,     0,     0,     0,     0,
      46,     0,     0,    47,    48,     0,     0,     0,     0,     0,
      49,    50,    51,    52,    53,    54,    55,    56,     0,     0,
       0,     0,   192,     0,     0,     0,     0,     0,     0,     0,
       0,    38,    39,    40,    41,    42,    43,    44,    45,     0,
       0,     0,     0,    46,     0,     0,    47,    48,     0,     0,
       0,     0,   218,    49,    50,    51,    52,    53,    54,    55,
      56,    38,    39,    40,    41,    42,    43,    44,    45,     0,
       0,     0,     0,    46,     0,     0,    47,    48,     0,     0,
       0,     0,     0,    49,    50,    51,    52,    53,    54,    55,
      56,   237,     0,     0,     0,     0,     0,     0,     0,     0,
      38,    39,    40,    41,    42,    43,    44,    45,     0,     0,
       0,     0,    46,     0,     0,    47,    48,     0,     0,     0,
       0,   245,    49,    50,    51,    52,    53,    54,    55,    56,
       0,     0,     0,     0,     0,    38,    39,    40,    41,    42,
      43,    44,    45,     0,     0,     0,     0,    46,     0,     0,
      47,    48,     0,     0,     0,     0,   170,    49,    50,    51,
      52,    53,    54,    55,    56,     0,   223,    38,    39,    40,
      41,    42,    43,    44,    45,     0,    99,     0,     0,    46,
       0,     0,    47,    48,     0,     0,     0,     0,     0,    49,
      50,    51,    52,    53,    54,    55,    56,    38,    39,    40,
      41,    42,    43,    44,    45,     0,     0,   101,     0,    46,
       0,     0,    47,    48,     0,     0,     0,     0,     0,    49,
      50,    51,    52,    53,    54,    55,    56,    38,    39,    40,
      41,    42,    43,    44,    45,     0,   129,     0,     0,    46,
       0,     0,    47,    48,     0,     0,     0,     0,     0,    49,
      50,    51,    52,    53,    54,    55,    56,    38,    39,    40,
      41,    42,    43,    44,    45,     0,   130,     0,     0,    46,
       0,     0,    47,    48,     0,     0,     0,     0,     0,    49,
      50,    51,    52,    53,    54,    55,    56,    38,    39,    40,
      41,    42,    43,    44,    45,     0,   166,     0,     0,    46,
       0,     0,    47,    48,     0,     0,     0,     0,     0,    49,
      50,    51,    52,    53,    54,    55,    56,    38,    39,    40,
      41,    42,    43,    44,    45,     0,     0,     0,   173,    46,
       0,     0,    47,    48,     0,     0,     0,     0,     0,    49,
      50,    51,    52,    53,    54,    55,    56,    38,    39,    40,
      41,    42,    43,    44,    45,     0,     0,     0,   194,    46,
       0,     0,    47,    48,     0,     0,     0,     0,     0,    49,
      50,    51,    52,    53,    54,    55,    56,    38,    39,    40,
      41,    42,    43,    44,    45,     0,     0,     0,     0,    46,
       0,     0,    47,    48,     0,     0,     0,     0,     0,    49,
      50,    51,    52,    53,    54,    55,    56,    94,    39,    40,
      41,    42,    43,    44,    45,     0,     0,     0,     0,    46,
       0,     0,    47,    48,     0,     0,     0,     0,     0,    49,
      50,    51,    52,    53,    54,    55,    56,   128,    39,    40,
      41,    42,    43,    44,    45,     0,     0,     0,     0,    46,
       0,     0,    47,    48,     0,     0,     0,     0,     0,    49,
      50,    51,    52,    53,    54,    55,    56,    38,    39,    40,
      41,    42,    43,    44,    45,     0,     0,     0,     0,    46,
       0,     0,    47,    48,     0,     0,     0,     0,     0,    49,
      50,    51,    52,    53,    54,    55,    38,    39,    40,    41,
      42,    43,    44,    45,     0,     0,     0,     0,    46,     0,
       0,    47,    48,     0,     0,     0,     0,     0,    49,    50,
      51,    52,    53,    54,    38,     0,     0,    41,    42,    43,
      44,    45,     0,     0,     0,     0,    46,    38,     0,    47,
      48,     0,    43,    44,    45,     0,    49,    50,     0,    46,
       0,     0,    47,    48,     0,     0,     0,     0,     0,    49,
      50
};

static const yytype_int16 yycheck[] =
{
      10,   170,   128,     3,     4,     5,     6,    16,     7,    27,
      28,     5,     7,    16,    24,    15,     0,   133,    21,    29,
     122,    31,   123,     5,   158,    16,    36,    15,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,   149,    58,    43,
     151,    39,   221,   169,    45,   157,   190,    46,     5,    45,
      69,     5,   188,     3,     4,     3,     4,    41,   237,    58,
      70,    70,     4,     5,     7,   201,   245,   211,    69,   213,
      70,    17,   216,    69,    94,     5,    96,    70,    98,    99,
     100,     3,     4,     5,     6,    41,     5,    70,    70,    10,
     110,   202,    71,    15,     5,     5,   208,    17,   118,    16,
      71,   121,    58,    59,    60,    61,     7,    16,   128,   129,
     130,    70,   132,     5,    24,    25,     7,    27,    28,   231,
      42,    17,     5,    70,    19,    47,    48,    49,    50,    51,
      40,    41,   152,    55,    18,   155,   156,    18,   158,     3,
       4,     5,     6,    18,    18,     5,   166,     5,    70,     5,
       5,    15,   172,    47,    48,    49,    50,    51,    18,    71,
       7,    55,    70,    18,    24,    25,    71,    27,    28,     5,
      20,    26,    71,    43,    18,    57,    70,    18,   198,    64,
      40,    41,    37,    47,    48,    49,    50,    51,     7,    44,
      26,    55,    71,    71,    71,    12,    18,    52,    53,    54,
     189,    37,    56,   223,   224,    17,    70,    62,    44,   211,
      65,    66,   228,    68,   154,    70,    52,    53,    54,   227,
      47,    48,    49,    50,    51,    -1,    62,    -1,    -1,    65,
      66,    -1,    68,     7,    70,     7,     8,     9,    10,    11,
      12,    13,    14,    70,    -1,    19,    -1,    19,    22,    23,
      22,    23,    -1,    -1,    -1,    29,    30,    29,    30,    31,
      32,    33,    34,    35,    36,     7,     8,     9,    10,    11,
      12,    13,    14,    -1,    -1,    -1,    -1,    19,     0,    -1,
      22,    23,    -1,     5,    -1,     7,    -1,    29,    30,    31,
      32,    33,    34,    35,    36,    -1,    18,    -1,    -1,    71,
      -1,    -1,    24,    25,    -1,    27,    28,    -1,     7,     8,
       9,    10,    11,    12,    13,    14,    -1,    -1,    40,    41,
      19,    -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    71,
      29,    30,    31,    32,    33,    34,    35,    36,     7,     8,
       9,    10,    11,    12,    13,    14,    -1,    -1,    -1,    -1,
      19,    -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,
      29,    30,    31,    32,    33,    34,    35,    36,    -1,    -1,
      -1,    -1,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     7,     8,     9,    10,    11,    12,    13,    14,    -1,
      -1,    -1,    -1,    19,    -1,    -1,    22,    23,    -1,    -1,
      -1,    -1,    71,    29,    30,    31,    32,    33,    34,    35,
      36,     7,     8,     9,    10,    11,    12,    13,    14,    -1,
      -1,    -1,    -1,    19,    -1,    -1,    22,    23,    -1,    -1,
      -1,    -1,    -1,    29,    30,    31,    32,    33,    34,    35,
      36,    67,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       7,     8,     9,    10,    11,    12,    13,    14,    -1,    -1,
      -1,    -1,    19,    -1,    -1,    22,    23,    -1,    -1,    -1,
      -1,    67,    29,    30,    31,    32,    33,    34,    35,    36,
      -1,    -1,    -1,    -1,    -1,     7,     8,     9,    10,    11,
      12,    13,    14,    -1,    -1,    -1,    -1,    19,    -1,    -1,
      22,    23,    -1,    -1,    -1,    -1,    63,    29,    30,    31,
      32,    33,    34,    35,    36,    -1,    38,     7,     8,     9,
      10,    11,    12,    13,    14,    -1,    16,    -1,    -1,    19,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    34,    35,    36,     7,     8,     9,
      10,    11,    12,    13,    14,    -1,    -1,    17,    -1,    19,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    34,    35,    36,     7,     8,     9,
      10,    11,    12,    13,    14,    -1,    16,    -1,    -1,    19,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    34,    35,    36,     7,     8,     9,
      10,    11,    12,    13,    14,    -1,    16,    -1,    -1,    19,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    34,    35,    36,     7,     8,     9,
      10,    11,    12,    13,    14,    -1,    16,    -1,    -1,    19,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    34,    35,    36,     7,     8,     9,
      10,    11,    12,    13,    14,    -1,    -1,    -1,    18,    19,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    34,    35,    36,     7,     8,     9,
      10,    11,    12,    13,    14,    -1,    -1,    -1,    18,    19,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    34,    35,    36,     7,     8,     9,
      10,    11,    12,    13,    14,    -1,    -1,    -1,    -1,    19,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    34,    35,    36,     7,     8,     9,
      10,    11,    12,    13,    14,    -1,    -1,    -1,    -1,    19,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    34,    35,    36,     7,     8,     9,
      10,    11,    12,    13,    14,    -1,    -1,    -1,    -1,    19,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    34,    35,    36,     7,     8,     9,
      10,    11,    12,    13,    14,    -1,    -1,    -1,    -1,    19,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    34,    35,     7,     8,     9,    10,
      11,    12,    13,    14,    -1,    -1,    -1,    -1,    19,    -1,
      -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    29,    30,
      31,    32,    33,    34,     7,    -1,    -1,    10,    11,    12,
      13,    14,    -1,    -1,    -1,    -1,    19,     7,    -1,    22,
      23,    -1,    12,    13,    14,    -1,    29,    30,    -1,    19,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    29,
      30
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     5,    24,    25,    27,    28,    40,    73,    77,    78,
       7,     5,     5,    78,    78,     5,     0,    74,    41,     3,
       4,     5,     6,    15,    70,    81,    82,    85,    86,     7,
      70,    17,    18,    73,     5,    70,    70,    82,     7,     8,
       9,    10,    11,    12,    13,    14,    19,    22,    23,    29,
      30,    31,    32,    33,    34,    35,    36,    15,    39,    82,
       5,    75,    76,    82,    70,    79,    82,    83,    84,    82,
      71,    82,    82,    82,    82,    82,    82,    82,    82,    82,
      82,    82,    82,    82,    82,    82,    82,    82,    82,    82,
      10,    82,    71,    16,     7,     5,    17,    71,    16,    16,
      70,    17,     7,     5,    82,    17,    82,    82,    82,    82,
      70,     5,    26,    37,    44,    52,    53,    54,    62,    65,
      66,    68,    70,   102,   103,    46,    58,    80,     7,    16,
      16,    82,     7,    70,     5,    18,    18,    18,    18,    82,
      41,    58,    59,    60,    61,   101,     5,    82,   103,    74,
      71,    42,    47,    48,    49,    50,    51,    55,    70,    87,
      90,    91,    94,    96,    82,    82,    16,    82,    83,    70,
      63,     5,     7,    18,    71,   103,    74,    88,    82,    20,
      97,    97,    82,    82,   103,    90,    94,    95,    19,    45,
      69,    71,    71,    82,    18,    71,    83,   102,    43,    82,
       5,    43,    89,     3,     4,    98,    99,   100,    57,    92,
      71,    16,    71,    70,    90,    91,    70,    94,    71,    18,
      71,    64,    82,    38,     7,    90,    74,    12,    16,    21,
     103,    56,    93,    94,    95,    18,   102,    67,    82,    82,
       3,   100,    99,   103,   102,    67,   102
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;
/* Location data for the look-ahead symbol.  */
YYLTYPE yylloc;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;

  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[2];

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;
#if YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 0;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 206 "magic-interpreter-parser.y"
    {;}
    break;

  case 3:
#line 208 "magic-interpreter-parser.y"
    {;}
    break;

  case 4:
#line 213 "magic-interpreter-parser.y"
    {;}
    break;

  case 5:
#line 215 "magic-interpreter-parser.y"
    {;}
    break;

  case 6:
#line 220 "magic-interpreter-parser.y"
    { (yyval.proc) = aCalloc(sizeof(proc_t), 1); ;}
    break;

  case 7:
#line 222 "magic-interpreter-parser.y"
    { (yyval.proc) = (yyvsp[(1) - (1)].proc); ;}
    break;

  case 8:
#line 226 "magic-interpreter-parser.y"
    { (yyval.proc) = aCalloc(sizeof(proc_t), 1);
                            (yyval.proc)->args_nr = 1;
                            (yyval.proc)->args = malloc(sizeof(int));
                            (yyval.proc)->args[0] = intern_id((yyvsp[(1) - (1)].s));
                          ;}
    break;

  case 9:
#line 232 "magic-interpreter-parser.y"
    { (yyval.proc) = (yyvsp[(1) - (3)].proc);
                            (yyval.proc)->args = realloc((yyval.proc)->args, sizeof(int) * (1 + (yyval.proc)->args_nr));
                            (yyval.proc)->args[(yyval.proc)->args_nr++] = intern_id((yyvsp[(3) - (3)].s));
                          ;}
    break;

  case 10:
#line 239 "magic-interpreter-parser.y"
    {
                            int var_id;
                            if (find_constant((yyvsp[(1) - (3)].s))) {
                                    fail((yylsp[(1) - (3)]).first_line, 0, "Attempt to redefine constant `%s' as global\n", (yyvsp[(1) - (3)].s));
                                    free((yyvsp[(1) - (3)].s));
                            } else {
                                    var_id = intern_id((yyvsp[(1) - (3)].s));
                                    magic_eval(&magic_default_env, &magic_conf.vars[var_id], (yyvsp[(3) - (3)].expr));
                            }
                          ;}
    break;

  case 11:
#line 250 "magic-interpreter-parser.y"
    {
                            val_t var;
                            magic_eval(&magic_default_env, &var, (yyvsp[(4) - (4)].expr));
                            bind_constant((yyvsp[(2) - (4)].s), &var, (yylsp[(1) - (4)]).first_line);
                          ;}
    break;

  case 12:
#line 256 "magic-interpreter-parser.y"
    {
                              teleport_anchor_t *anchor = calloc(sizeof(teleport_anchor_t), 1);
                              anchor->name = (yyvsp[(2) - (6)].s);
                              anchor->invocation = magic_eval_str(&magic_default_env, (yyvsp[(4) - (6)].expr));
                              anchor->location = (yyvsp[(6) - (6)].expr);

                              if (!failed_flag)
                                  add_teleport_anchor(anchor, (yylsp[(1) - (6)]).first_line);
                              failed_flag = 0;
                          ;}
    break;

  case 13:
#line 267 "magic-interpreter-parser.y"
    {
                              proc_t *proc = (yyvsp[(4) - (7)].proc);
                              proc->name = (yyvsp[(2) - (7)].s);
                              proc->body = (yyvsp[(7) - (7)].effect);
                              if (!failed_flag)
                                  install_proc(proc);
                              failed_flag = 0;
                          ;}
    break;

  case 14:
#line 276 "magic-interpreter-parser.y"
    { spell_t *spell = (yyvsp[(8) - (8)].spell);
                            spell->name = (yyvsp[(3) - (8)].s);
                            spell->invocation = magic_eval_str(&magic_default_env, (yyvsp[(6) - (8)].expr));
                            spell->arg = (yyvsp[(4) - (8)].spellarg_def).id;
                            spell->spellarg_ty = (yyvsp[(4) - (8)].spellarg_def).ty;
                            spell->flags = (yyvsp[(1) - (8)].i);
                            if (!failed_flag)
                                add_spell(spell, (yylsp[(1) - (8)]).first_line);
                            failed_flag = 0;
                          ;}
    break;

  case 15:
#line 288 "magic-interpreter-parser.y"
    { (yyval.i) = 0; ;}
    break;

  case 16:
#line 290 "magic-interpreter-parser.y"
    { if ((yyvsp[(2) - (2)].i) & SPELL_FLAG_LOCAL)
                                        fail((yylsp[(1) - (2)]).first_line, (yylsp[(1) - (2)]).first_column, "`LOCAL' specified more than once");
                                   (yyval.i) = (yyvsp[(2) - (2)].i) | SPELL_FLAG_LOCAL;
                                ;}
    break;

  case 17:
#line 295 "magic-interpreter-parser.y"
    { if ((yyvsp[(2) - (2)].i) & SPELL_FLAG_SILENT)
                                        fail((yylsp[(1) - (2)]).first_line, (yylsp[(1) - (2)]).first_column, "`SILENT' specified more than once");
                                   (yyval.i) = (yyvsp[(2) - (2)].i) | SPELL_FLAG_SILENT;
                                ;}
    break;

  case 18:
#line 302 "magic-interpreter-parser.y"
    { (yyval.spellarg_def).ty = SPELLARG_NONE; ;}
    break;

  case 19:
#line 304 "magic-interpreter-parser.y"
    { (yyval.spellarg_def).id = intern_id((yyvsp[(2) - (5)].s));
                            (yyval.spellarg_def).ty = (yyvsp[(4) - (5)].i); ;}
    break;

  case 20:
#line 310 "magic-interpreter-parser.y"
    { (yyval.i) = SPELLARG_PC; ;}
    break;

  case 21:
#line 312 "magic-interpreter-parser.y"
    { (yyval.i) = SPELLARG_STRING; ;}
    break;

  case 22:
#line 317 "magic-interpreter-parser.y"
    { (yyval.value).ty = TY_DIR;
                                  (yyval.value).v.v_int = (yyvsp[(1) - (1)].i); ;}
    break;

  case 23:
#line 320 "magic-interpreter-parser.y"
    { (yyval.value).ty = TY_INT;
                                  (yyval.value).v.v_int = (yyvsp[(1) - (1)].i); ;}
    break;

  case 24:
#line 323 "magic-interpreter-parser.y"
    { (yyval.value).ty = TY_STRING;
                                  (yyval.value).v.v_string = (yyvsp[(1) - (1)].s); ;}
    break;

  case 25:
#line 329 "magic-interpreter-parser.y"
    { (yyval.expr) = magic_new_expr(EXPR_VAL);
                                  (yyval.expr)->e.e_val = (yyvsp[(1) - (1)].value); ;}
    break;

  case 26:
#line 332 "magic-interpreter-parser.y"
    {
                                        val_t *val;
                                        if ((val = find_constant((yyvsp[(1) - (1)].s)))) {
                                                (yyval.expr) = magic_new_expr(EXPR_VAL);
                                                (yyval.expr)->e.e_val = *val;
                                        } else {
                                                (yyval.expr) = magic_new_expr(EXPR_ID);
                                                (yyval.expr)->e.e_id = intern_id((yyvsp[(1) - (1)].s));
                                        }
                                ;}
    break;

  case 27:
#line 343 "magic-interpreter-parser.y"
    { (yyval.expr) = magic_new_expr(EXPR_AREA);
                                  (yyval.expr)->e.e_area = (yyvsp[(1) - (1)].area); ;}
    break;

  case 28:
#line 346 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), "+", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 29:
#line 348 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), "-", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 30:
#line 350 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), "*", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 31:
#line 352 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), "%", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 32:
#line 354 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), "/", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 33:
#line 356 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), ">", (yyvsp[(3) - (3)].expr), (yyvsp[(1) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 34:
#line 358 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), ">", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 35:
#line 360 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), "&", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 36:
#line 362 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), "^", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 37:
#line 364 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), "|", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 38:
#line 366 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), "<<", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 39:
#line 368 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), ">>", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 40:
#line 370 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), ">=", (yyvsp[(3) - (3)].expr), (yyvsp[(1) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 41:
#line 372 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), ">=", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 42:
#line 374 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), "&&", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 43:
#line 376 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), "||", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 44:
#line 378 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), "=", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 45:
#line 380 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), "=", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 46:
#line 382 "magic-interpreter-parser.y"
    { BIN_EXPR((yyval.expr), "=", (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column);
                                  (yyval.expr) = fun_expr("not", 1, &(yyval.expr), (yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column); ;}
    break;

  case 47:
#line 385 "magic-interpreter-parser.y"
    { (yyval.expr) = fun_expr((yyvsp[(1) - (4)].s), (yyvsp[(3) - (4)].arg_list).args_nr, (yyvsp[(3) - (4)].arg_list).args, (yylsp[(1) - (4)]).first_line, (yylsp[(1) - (4)]).first_column);
                                  if ((yyvsp[(3) - (4)].arg_list).args)
                                          free((yyvsp[(3) - (4)].arg_list).args);
                                  free((yyvsp[(1) - (4)].s)); ;}
    break;

  case 48:
#line 390 "magic-interpreter-parser.y"
    { (yyval.expr) = (yyvsp[(2) - (3)].expr); ;}
    break;

  case 49:
#line 394 "magic-interpreter-parser.y"
    { (yyval.arg_list).args_nr = 0; ;}
    break;

  case 50:
#line 396 "magic-interpreter-parser.y"
    { (yyval.arg_list) = (yyvsp[(1) - (1)].arg_list) ;}
    break;

  case 51:
#line 401 "magic-interpreter-parser.y"
    { (yyval.arg_list).args = aCalloc(sizeof(expr_t *), 1);
                                  (yyval.arg_list).args_nr = 1;
                                  (yyval.arg_list).args[0] = (yyvsp[(1) - (1)].expr);
                                ;}
    break;

  case 52:
#line 406 "magic-interpreter-parser.y"
    { (yyval.arg_list).args = realloc((yyval.arg_list).args, (1 + (yyval.arg_list).args_nr) * sizeof(expr_t *));
                                  (yyval.arg_list).args[(yyval.arg_list).args_nr++] = (yyvsp[(3) - (3)].expr);
                                ;}
    break;

  case 53:
#line 413 "magic-interpreter-parser.y"
    { (yyval.location).m = (yyvsp[(3) - (8)].expr); (yyval.location).x = (yyvsp[(5) - (8)].expr); (yyval.location).y = (yyvsp[(7) - (8)].expr); ;}
    break;

  case 54:
#line 417 "magic-interpreter-parser.y"
    { (yyval.area).ty = AREA_LOCATION;
                                  (yyval.area).a.a_loc = (yyvsp[(1) - (1)].location);
				;}
    break;

  case 55:
#line 421 "magic-interpreter-parser.y"
    { (yyval.area).ty = AREA_RECT;
                                  (yyval.area).a.a_rect.loc = (yyvsp[(1) - (8)].location);
                                  (yyval.area).a.a_rect.width = (yyvsp[(5) - (8)].expr);
                                  (yyval.area).a.a_rect.height = (yyvsp[(7) - (8)].expr);
                                ;}
    break;

  case 56:
#line 427 "magic-interpreter-parser.y"
    { (yyval.area).ty = AREA_BAR;
                                  (yyval.area).a.a_bar.loc = (yyvsp[(1) - (9)].location);
                                  (yyval.area).a.a_bar.width = (yyvsp[(6) - (9)].expr);
                                  (yyval.area).a.a_bar.depth = (yyvsp[(8) - (9)].expr);
                                  (yyval.area).a.a_bar.dir = (yyvsp[(3) - (9)].expr);
                                ;}
    break;

  case 57:
#line 437 "magic-interpreter-parser.y"
    {  (yyval.spell) = new_spell((yyvsp[(1) - (1)].spellguard)); ;}
    break;

  case 58:
#line 439 "magic-interpreter-parser.y"
    {  (yyval.spell) = new_spell((yyvsp[(4) - (4)].spellguard)); 
                                   (yyval.spell)->letdefs_nr = (yyvsp[(2) - (4)].letdefs).letdefs_nr;
                                   (yyval.spell)->letdefs = (yyvsp[(2) - (4)].letdefs).letdefs;
                                   (yyval.spell)->spellguard = (yyvsp[(4) - (4)].spellguard);
                                ;}
    break;

  case 59:
#line 448 "magic-interpreter-parser.y"
    { (yyval.letdefs).letdefs_nr = 0;
                                  (yyval.letdefs).letdefs = (letdef_t *) malloc(1);
                                ;}
    break;

  case 60:
#line 452 "magic-interpreter-parser.y"
    { (yyval.letdefs) = (yyvsp[(1) - (3)].letdefs);
                                  (yyval.letdefs).letdefs_nr++;
                                  (yyval.letdefs).letdefs = realloc((yyval.letdefs).letdefs, sizeof(letdef_t) * (yyval.letdefs).letdefs_nr);
                                  (yyval.letdefs).letdefs[(yyvsp[(1) - (3)].letdefs).letdefs_nr] = (yyvsp[(2) - (3)].vardef);
                                ;}
    break;

  case 61:
#line 461 "magic-interpreter-parser.y"
    {
                                        if (find_constant((yyvsp[(1) - (3)].s))) {
                                                fail((yylsp[(1) - (3)]).first_line, (yylsp[(1) - (3)]).first_column, "Attempt to re-define constant `%s' as LET-bound variable.\n", (yyvsp[(1) - (3)].s));
                                                free((yyvsp[(1) - (3)].s));
                                        } else {
                                                (yyval.vardef).id = intern_id((yyvsp[(1) - (3)].s));
                                                (yyval.vardef).expr = (yyvsp[(3) - (3)].expr);
                                        }
                                ;}
    break;

  case 62:
#line 474 "magic-interpreter-parser.y"
    { (yyval.spellguard) = (yyvsp[(1) - (1)].spellguard); ;}
    break;

  case 63:
#line 476 "magic-interpreter-parser.y"
    { spellguard_t *sg = new_spellguard(SPELLGUARD_CHOICE);
                                  sg->next = (yyvsp[(1) - (3)].spellguard);
                                  sg->s.s_alt = (yyvsp[(3) - (3)].spellguard);
                                  (yyval.spellguard) = sg;
                                ;}
    break;

  case 64:
#line 485 "magic-interpreter-parser.y"
    { (yyval.spellguard) = spellguard_implication((yyvsp[(1) - (3)].spellguard), (yyvsp[(3) - (3)].spellguard)); ;}
    break;

  case 65:
#line 487 "magic-interpreter-parser.y"
    { (yyval.spellguard) = (yyvsp[(2) - (3)].spellguard); ;}
    break;

  case 66:
#line 489 "magic-interpreter-parser.y"
    { spellguard_t *sg = new_spellguard(SPELLGUARD_EFFECT);
                                  sg->s.s_effect.effect = (yyvsp[(2) - (4)].effect);
                                  sg->s.s_effect.at_trigger = (yyvsp[(3) - (4)].effect);
                                  sg->s.s_effect.at_end = (yyvsp[(4) - (4)].effect);
                                  (yyval.spellguard) = sg;
                                ;}
    break;

  case 67:
#line 499 "magic-interpreter-parser.y"
    { (yyval.effect) = NULL; ;}
    break;

  case 68:
#line 501 "magic-interpreter-parser.y"
    { (yyval.effect) = (yyvsp[(2) - (2)].effect); ;}
    break;

  case 69:
#line 506 "magic-interpreter-parser.y"
    { (yyval.effect) = NULL; ;}
    break;

  case 70:
#line 508 "magic-interpreter-parser.y"
    { (yyval.effect) = (yyvsp[(2) - (2)].effect); ;}
    break;

  case 71:
#line 513 "magic-interpreter-parser.y"
    { (yyval.spellguard) = (yyvsp[(1) - (1)].spellguard); ;}
    break;

  case 72:
#line 515 "magic-interpreter-parser.y"
    { spellguard_t *sg = new_spellguard(SPELLGUARD_CHOICE);
                                  sg->next = (yyvsp[(1) - (3)].spellguard);
                                  sg->s.s_alt = (yyvsp[(3) - (3)].spellguard);
                                  (yyval.spellguard) = sg;
                                ;}
    break;

  case 73:
#line 521 "magic-interpreter-parser.y"
    { (yyval.spellguard) = (yyvsp[(2) - (3)].spellguard); ;}
    break;

  case 74:
#line 526 "magic-interpreter-parser.y"
    { (yyval.spellguard) = (yyvsp[(1) - (1)].spellguard); ;}
    break;

  case 75:
#line 528 "magic-interpreter-parser.y"
    { (yyval.spellguard) = spellguard_implication ((yyvsp[(1) - (3)].spellguard), (yyvsp[(3) - (3)].spellguard)); ;}
    break;

  case 76:
#line 533 "magic-interpreter-parser.y"
    { (yyval.spellguard) = new_spellguard(SPELLGUARD_CONDITION);
                                  (yyval.spellguard)->s.s_condition = (yyvsp[(2) - (2)].expr);
                                ;}
    break;

  case 77:
#line 537 "magic-interpreter-parser.y"
    { (yyval.spellguard) = new_spellguard(SPELLGUARD_CATALYSTS);
                                  (yyval.spellguard)->s.s_catalysts = (yyvsp[(2) - (2)].components);
                                ;}
    break;

  case 78:
#line 541 "magic-interpreter-parser.y"
    { (yyval.spellguard) = new_spellguard(SPELLGUARD_COMPONENTS);
                                  (yyval.spellguard)->s.s_components = (yyvsp[(2) - (2)].components);
                                ;}
    break;

  case 79:
#line 545 "magic-interpreter-parser.y"
    { (yyval.spellguard) = new_spellguard(SPELLGUARD_MANA);
                                  (yyval.spellguard)->s.s_mana = (yyvsp[(2) - (2)].expr);
                                ;}
    break;

  case 80:
#line 549 "magic-interpreter-parser.y"
    { (yyval.spellguard) = new_spellguard(SPELLGUARD_CASTTIME);
                                  (yyval.spellguard)->s.s_casttime = (yyvsp[(2) - (2)].expr);
                                ;}
    break;

  case 81:
#line 556 "magic-interpreter-parser.y"
    { (yyval.components) = (yyvsp[(2) - (3)].components); ;}
    break;

  case 82:
#line 561 "magic-interpreter-parser.y"
    { (yyval.components) = NULL;
                                  magic_add_component(&(yyval.components), (yyvsp[(1) - (1)].component).id, (yyvsp[(1) - (1)].component).count);
                                ;}
    break;

  case 83:
#line 565 "magic-interpreter-parser.y"
    { (yyval.components) = (yyvsp[(1) - (3)].components);
                                  magic_add_component(&(yyval.components), (yyvsp[(3) - (3)].component).id, (yyvsp[(3) - (3)].component).count);
                                ;}
    break;

  case 84:
#line 572 "magic-interpreter-parser.y"
    { (yyval.component).id = (yyvsp[(3) - (3)].i); (yyval.component).count = (yyvsp[(1) - (3)].i); ;}
    break;

  case 85:
#line 574 "magic-interpreter-parser.y"
    { (yyval.component).id = (yyvsp[(1) - (1)].i); (yyval.component).count = 1; ;}
    break;

  case 86:
#line 579 "magic-interpreter-parser.y"
    { struct item_data *item = itemdb_searchname((yyvsp[(1) - (1)].s));
                                  if (!item) {
                                          fail ((yylsp[(1) - (1)]).first_line, (yylsp[(1) - (1)]).first_column, "Unknown item `%s'\n", (yyvsp[(1) - (1)].s));
                                          (yyval.i) = 0;
                                  } else
                                          (yyval.i) = item->nameid;
                                  free ((yyvsp[(1) - (1)].s));
                                ;}
    break;

  case 87:
#line 588 "magic-interpreter-parser.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); ;}
    break;

  case 88:
#line 593 "magic-interpreter-parser.y"
    { (yyval.i) = FOREACH_FILTER_PC; ;}
    break;

  case 89:
#line 595 "magic-interpreter-parser.y"
    { (yyval.i) = FOREACH_FILTER_MOB; ;}
    break;

  case 90:
#line 597 "magic-interpreter-parser.y"
    { (yyval.i) = FOREACH_FILTER_ENTITY; ;}
    break;

  case 91:
#line 599 "magic-interpreter-parser.y"
    { (yyval.i) = FOREACH_FILTER_SPELL; ;}
    break;

  case 92:
#line 601 "magic-interpreter-parser.y"
    { (yyval.i) = FOREACH_FILTER_TARGET; ;}
    break;

  case 93:
#line 606 "magic-interpreter-parser.y"
    { (yyval.effect) = (yyvsp[(2) - (3)].effect); ;}
    break;

  case 94:
#line 608 "magic-interpreter-parser.y"
    { (yyval.effect) = new_effect(EFFECT_SKIP); ;}
    break;

  case 95:
#line 610 "magic-interpreter-parser.y"
    { (yyval.effect) = new_effect(EFFECT_ABORT); ;}
    break;

  case 96:
#line 612 "magic-interpreter-parser.y"
    { (yyval.effect) = new_effect(EFFECT_END); ;}
    break;

  case 97:
#line 614 "magic-interpreter-parser.y"
    { (yyval.effect) = new_effect(EFFECT_BREAK); ;}
    break;

  case 98:
#line 616 "magic-interpreter-parser.y"
    {
                                        if (find_constant((yyvsp[(1) - (4)].s))) {
                                                fail((yylsp[(1) - (4)]).first_line, (yylsp[(1) - (4)]).first_column, "Attempt to re-define constant `%s' in assignment.", (yyvsp[(1) - (4)].s));
                                                free((yyvsp[(1) - (4)].s));
                                        } else {
                                                (yyval.effect) = new_effect(EFFECT_ASSIGN);
                                                (yyval.effect)->e.e_assign.id = intern_id((yyvsp[(1) - (4)].s));
                                                (yyval.effect)->e.e_assign.expr = (yyvsp[(3) - (4)].expr);
                                        }
                                ;}
    break;

  case 99:
#line 627 "magic-interpreter-parser.y"
    { (yyval.effect) = new_effect(EFFECT_FOREACH);
                                  (yyval.effect)->e.e_foreach.id = intern_id((yyvsp[(3) - (7)].s));
                                  (yyval.effect)->e.e_foreach.area = (yyvsp[(5) - (7)].expr);
                                  (yyval.effect)->e.e_foreach.body = (yyvsp[(7) - (7)].effect);
                                  (yyval.effect)->e.e_foreach.filter = (yyvsp[(2) - (7)].i);
                                ;}
    break;

  case 100:
#line 634 "magic-interpreter-parser.y"
    { (yyval.effect) = new_effect(EFFECT_FOR);
                                  (yyval.effect)->e.e_for.id = intern_id((yyvsp[(2) - (8)].s));
                                  (yyval.effect)->e.e_for.start = (yyvsp[(4) - (8)].expr);
                                  (yyval.effect)->e.e_for.stop = (yyvsp[(6) - (8)].expr);
                                  (yyval.effect)->e.e_for.body = (yyvsp[(8) - (8)].effect);
                                ;}
    break;

  case 101:
#line 641 "magic-interpreter-parser.y"
    { (yyval.effect) = new_effect(EFFECT_IF);
                                  (yyval.effect)->e.e_if.cond = (yyvsp[(2) - (6)].expr);
                                  (yyval.effect)->e.e_if.true_branch = (yyvsp[(4) - (6)].effect);
                                  (yyval.effect)->e.e_if.false_branch = (yyvsp[(6) - (6)].effect);
                                ;}
    break;

  case 102:
#line 647 "magic-interpreter-parser.y"
    { (yyval.effect) = new_effect(EFFECT_IF);
                                  (yyval.effect)->e.e_if.cond = (yyvsp[(2) - (4)].expr);
                                  (yyval.effect)->e.e_if.true_branch = (yyvsp[(4) - (4)].effect);
                                  (yyval.effect)->e.e_if.false_branch = new_effect(EFFECT_SKIP);
                                ;}
    break;

  case 103:
#line 653 "magic-interpreter-parser.y"
    { (yyval.effect) = new_effect(EFFECT_SLEEP);
                                  (yyval.effect)->e.e_sleep = (yyvsp[(2) - (3)].expr);
                                ;}
    break;

  case 104:
#line 657 "magic-interpreter-parser.y"
    { (yyval.effect) = op_effect((yyvsp[(1) - (5)].s), (yyvsp[(3) - (5)].arg_list).args_nr, (yyvsp[(3) - (5)].arg_list).args, (yylsp[(1) - (5)]).first_line, (yylsp[(1) - (5)]).first_column);
                                  free((yyvsp[(1) - (5)].s));
                                ;}
    break;

  case 105:
#line 661 "magic-interpreter-parser.y"
    { (yyval.effect) = new_effect(EFFECT_SCRIPT);
                                  (yyval.effect)->e.e_script = parse_script((unsigned char *) (yyvsp[(1) - (1)].s), (yylsp[(1) - (1)]).first_line);
                                  free((yyvsp[(1) - (1)].s));
                                  if ((yyval.effect)->e.e_script == NULL)
                                      fail((yylsp[(1) - (1)]).first_line, (yylsp[(1) - (1)]).first_column, "Failed to compile script\n");
                                ;}
    break;

  case 106:
#line 668 "magic-interpreter-parser.y"
    { (yyval.effect) = call_proc((yyvsp[(2) - (6)].s), (yyvsp[(4) - (6)].arg_list).args_nr, (yyvsp[(4) - (6)].arg_list).args, (yylsp[(1) - (6)]).first_line, (yylsp[(1) - (6)]).first_column);
                                  free((yyvsp[(2) - (6)].s));
                                ;}
    break;

  case 107:
#line 674 "magic-interpreter-parser.y"
    { (yyval.effect) = new_effect(EFFECT_SKIP); ;}
    break;

  case 108:
#line 676 "magic-interpreter-parser.y"
    { (yyval.effect) = set_effect_continuation((yyvsp[(1) - (3)].effect), (yyvsp[(3) - (3)].effect)); ;}
    break;


/* Line 1267 of yacc.c.  */
#line 2600 "magic-interpreter-parser.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the look-ahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 680 "magic-interpreter-parser.y"


/* We do incremental realloc here to store our results.  Since this happens only once
 * during startup for a relatively manageable set of configs, it should be fine. */

static int
intern_id(char *id_name)
{
        int i;

        for (i = 0; i < magic_conf.vars_nr; i++)
                if (!strcmp(id_name, magic_conf.var_name[i])) {
                        free(id_name);
                        return i;
                }

        /* Must add new */
        i = magic_conf.vars_nr++;
        magic_conf.var_name = realloc(magic_conf.var_name, magic_conf.vars_nr * sizeof(char *));
        magic_conf.var_name[i] = id_name;
        magic_conf.vars = realloc(magic_conf.vars, magic_conf.vars_nr * sizeof(val_t));
        magic_conf.vars[i].ty = TY_UNDEF;

        return i;
}

static void
add_spell(spell_t *spell, int line_nr)
{
        int index = magic_conf.spells_nr;
        int i;

        for (i = 0; i < index; i++) {
                if (!strcmp(magic_conf.spells[i]->name, spell->name)) {
                        fail(line_nr, 0, "Attempt to redefine spell `%s'\n", spell->name);
                        return;
                }
                if (!strcmp(magic_conf.spells[i]->invocation, spell->invocation)) {
                        fail(line_nr, 0, "Attempt to redefine spell invocation `%s' between spells `%s' and `%s'\n",
                             spell->invocation, magic_conf.spells[i]->name, spell->name);
                        return;
                }
        }
        magic_conf.spells_nr++;

        magic_conf.spells = realloc(magic_conf.spells, magic_conf.spells_nr * sizeof (spell_t*));
        magic_conf.spells[index] = spell;

            
}

static void
add_teleport_anchor(teleport_anchor_t *anchor, int line_nr)
{
        int index = magic_conf.anchors_nr;
        int i;

        for (i = 0; i < index; i++) {
                if (!strcmp(magic_conf.anchors[i]->name, anchor->name)) {
                        fail(line_nr, 0, "Attempt to redefine teleport anchor `%s'\n", anchor->name);
                        return;
                }
                if (!strcmp(magic_conf.anchors[i]->invocation, anchor->invocation)) {
                        fail(line_nr, 0, "Attempt to redefine anchor invocation `%s' between anchors `%s' and `%s'\n",
                             anchor->invocation, magic_conf.anchors[i]->name, anchor->name);
                        return;
                }
        }
        magic_conf.anchors_nr++;

        magic_conf.anchors = realloc(magic_conf.anchors, magic_conf.anchors_nr * sizeof (teleport_anchor_t*));
        magic_conf.anchors[index] = anchor;
}


static void
fail(int line, int column, char *fmt, ...)
{
        va_list ap;
        fprintf(stderr, "[magic-init]  L%d:%d: ", line, column);
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        failed_flag = 1;
}

static expr_t *
fun_expr(char *name, int args_nr, expr_t **args, int line, int column)
{
        int id;
        expr_t *expr;
        fun_t *fun = magic_get_fun(name, &id);

        if (!fun) {
                fail(line, column, "Unknown function `%s'\n", name);
        } else if (strlen(fun->signature) != args_nr) {
                fail(line, column, "Incorrect number of arguments to function `%s': Expected %d, found %d\n", name, strlen(fun->signature), args_nr);
                fun = NULL;
        }

        if (fun) {
                int i;

                expr = magic_new_expr(EXPR_FUNAPP);
                expr->e.e_funapp.line_nr = line;
                expr->e.e_funapp.column = column;
                expr->e.e_funapp.id = id;
                expr->e.e_funapp.args_nr = args_nr;

                for (i = 0; i < args_nr; i++)
                        expr->e.e_funapp.args[i] = args[i];
        } else { /* failure */
                expr = magic_new_expr(EXPR_VAL);
                expr->e.e_val.ty = TY_FAIL;
        }

        return expr;
}

static spell_t *
new_spell(spellguard_t *guard)
{
        spell_t *retval = calloc(1, sizeof(spell_t));
        retval->spellguard = guard;
        return retval;
}

static spellguard_t *
new_spellguard(int ty)
{
        spellguard_t *retval = calloc(1, sizeof(spellguard_t));
        retval->ty = ty;
        return retval;
}

static spellguard_t *
spellguard_implication(spellguard_t *a, spellguard_t *b)
{
        spellguard_t *retval = a;

        if (a == b) /* This can happen due to reference sharing:
                     * e.g.,
                     *  (R0 -> (R1 | R2)) => (R3)
                     * yields
                     *  (R0 -> (R1 -> R3 | R2 -> R3))
                     *
                     * So if we now add => R4 to that, we want
                     *  (R0 -> (R1 -> R3 -> R4 | R2 -> R3 -> R4))
                     *
                     * but we only need to add it once, because the R3 reference is shared.
                    */
                return retval;

                /* If the premise is a disjunction, b is the continuation of _all_ branches */
        if (a->ty == SPELLGUARD_CHOICE)
                spellguard_implication(a->s.s_alt, b);
        if (a->next)
                spellguard_implication(a->next, b);
        else
                a->next = b;
                

        return retval;
}

static effect_t *
new_effect(int ty)
{
        effect_t *effect = (effect_t *) calloc(1, sizeof(effect_t));
        effect->ty = ty;
        return effect;
}

static effect_t *
set_effect_continuation(effect_t *src, effect_t *continuation)
{
        effect_t *retval = src;
        /* This function is completely analogous to `spellguard_implication' above; read the control flow implications above first before pondering it. */

        if (src == continuation)
                return retval;

        /* For FOR and FOREACH, we use special stack handlers and thus don't have to set
         * the continuation.  It's only IF that we need to handle in this fashion. */
        if (src->ty == EFFECT_IF) {
                set_effect_continuation(src->e.e_if.true_branch, continuation);
                set_effect_continuation(src->e.e_if.false_branch, continuation);
        }
        if (src->next)
                set_effect_continuation(src->next, continuation);
        else
                src->next = continuation;

        return retval;
}

static effect_t *
op_effect(char *name, int args_nr, expr_t **args, int line, int column)
{
        int id;
        effect_t *effect;
        op_t *op = magic_get_op(name, &id);

        if (!op)
                fail(line, column, "Unknown operation `%s'\n", name);
        else if (strlen(op->signature) != args_nr) {
                fail(line, column, "Incorrect number of arguments to operation `%s': Expected %d, found %d\n", name, strlen(op->signature), args_nr);
                op = NULL;
        }

        if (op) {
                int i;

                effect = new_effect(EFFECT_OP);
                effect->e.e_op.line_nr = line;
                effect->e.e_op.column = column;
                effect->e.e_op.id = id;
                effect->e.e_op.args_nr = args_nr;

                for (i = 0; i < args_nr; i++)
                        effect->e.e_op.args[i] = args[i];
        } else /* failure */
                effect = new_effect(EFFECT_SKIP);

        return effect;
}


proc_t *procs = NULL;
int procs_nr = 0;


static void
install_proc(proc_t *proc)
{
        if (!procs) {
                procs = proc;
                procs_nr = 1;
        } else {
                procs = realloc(procs, sizeof(proc_t) * (1 + procs_nr));
                procs[procs_nr++] = *proc;
        }
}

static effect_t *
call_proc(char *name, int args_nr, expr_t **args, int line_nr, int column)
{
        proc_t *p = NULL;
        int i;
        effect_t *retval;

        for (i = 0; i < procs_nr; i++)
                if (!strcmp(procs[i].name, name)) {
                        p = &procs[i];
                        break;
                }

        if (!p) {
                fail(line_nr, column, "Unknown procedure `%s'\n", name);
                return new_effect(EFFECT_SKIP);
        }

        if (p->args_nr != args_nr) {
                fail(line_nr, column, "Procedure %s/%d invoked with %d parameters\n", name, p->args_nr, args_nr);
                return new_effect(EFFECT_SKIP);
        }

        retval = new_effect(EFFECT_CALL);
        retval->e.e_call.body = p->body;
        retval->e.e_call.args_nr = args_nr;
        retval->e.e_call.formals = p->args;
        retval->e.e_call.actuals = args;
        return retval;
}

struct const_def_rec {
        char *name;
        val_t val;
} *const_defs = NULL;

int const_defs_nr = 0;

static void
bind_constant(char *name, val_t *val, int line_nr)
{
        if (find_constant(name)) {
                fail(line_nr, 0, "Redefinition of constant `%s'\n", name);
                return;
        }

        if (!const_defs)
                const_defs = (struct const_def_rec *)malloc(sizeof(struct const_def_rec));
        else
                const_defs = (struct const_def_rec *)realloc(const_defs,
                                                             (const_defs_nr + 1) * sizeof(struct const_def_rec));

        const_defs[const_defs_nr].name = name;
        const_defs[const_defs_nr].val = *val;
        ++const_defs_nr;
}

static val_t *
find_constant(char *name)
{
        int i;
        for (i = 0; i < const_defs_nr; i++) {
                if (!strcmp(const_defs[i].name, name)) {
                        free(name);
                        return &const_defs[i].val;
                }
        }

        return NULL;
}




#define INTERN_ASSERT(name, id) { int zid = intern_id(name); if (zid != id) fprintf(stderr, "[magic-conf] INTERNAL ERROR: Builtin special var %s interned to %d, not %d as it should be!\n", name, zid, id); error_flag = 1; }

extern FILE *magic_frontend_in;

int
magic_init(char *conffile) // must be called after itemdb initialisation
{
        int error_flag = 0;

        magic_conf.vars_nr = 0;
        magic_conf.var_name = (char **)malloc(1);
        magic_conf.vars = (val_t *)malloc(1);

        magic_conf.obscure_chance = 95;
        magic_conf.min_casttime = 100;

        magic_conf.spells_nr = 0;
        magic_conf.spells = (spell_t **)malloc(1);

        magic_conf.anchors_nr = 0;
        magic_conf.anchors = (teleport_anchor_t **)malloc(1);

        INTERN_ASSERT("min_casttime", VAR_MIN_CASTTIME);
        INTERN_ASSERT("obscure_chance", VAR_OBSCURE_CHANCE);
        INTERN_ASSERT("caster", VAR_CASTER);
        INTERN_ASSERT("spellpower", VAR_SPELLPOWER);
        INTERN_ASSERT("self_spell", VAR_SPELL);
        INTERN_ASSERT("self_invocation", VAR_INVOCATION);
        INTERN_ASSERT("target", VAR_TARGET);
        INTERN_ASSERT("script_target", VAR_SCRIPTTARGET);
        INTERN_ASSERT("location", VAR_LOCATION);

        magic_frontend_in = fopen(conffile, "r");
        if (!magic_frontend_in) {
                fprintf(stderr, "[magic-conf] Magic configuration file `%s' not found -> no magic.\n", conffile);
                return 0;
        }
        magic_frontend_parse();

        if (magic_conf.vars[VAR_MIN_CASTTIME].ty == TY_INT)
                magic_conf.min_casttime = magic_conf.vars[VAR_MIN_CASTTIME].v.v_int;

        if (magic_conf.vars[VAR_OBSCURE_CHANCE].ty == TY_INT)
                magic_conf.obscure_chance = magic_conf.vars[VAR_OBSCURE_CHANCE].v.v_int;

        printf("[magic-conf] Magic initialised; obscure at %d%%.  %d spells, %d teleport anchors.\n",
               magic_conf.obscure_chance, magic_conf.spells_nr, magic_conf.anchors_nr);

        if (procs)
                free(procs);
        return error_flag;
}

extern int magic_frontend_lineno;

static void
magic_frontend_error(const char *msg)
{
    fprintf(stderr, "[magic-conf] Parse error: %s at line %d\n", msg, magic_frontend_lineno);
    failed_flag = 1;
}

