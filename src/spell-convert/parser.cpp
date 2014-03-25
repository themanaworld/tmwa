/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison implementation for Yacc-like parsers in C

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
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         spell_converterparse
#define yylex           spell_converterlex
#define yyerror         spell_convertererror
#define yydebug         spell_converterdebug
#define yynerrs         spell_converternerrs

#define yylval          spell_converterlval
#define yychar          spell_converterchar

/* Copy the first part of user declarations.  */

#line 75 "src/spell-convert/parser.cpp" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "parser.hpp".  */
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
#line 2 "../src/spell-convert/parser.ypp" /* yacc.c:355  */

/* vim: set ft=yacc: */
#include "../strings/rstring.hpp"

#include "ast.hpp"

#undef YYERROR_VERBOSE
#define YYERROR_VERBOSE 1

#line 115 "src/spell-convert/parser.cpp" /* yacc.c:355  */

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
#line 27 "../src/spell-convert/parser.ypp" /* yacc.c:355  */

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

#line 203 "src/spell-convert/parser.cpp" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE spell_converterlval;

int spell_converterparse (void);

#endif /* !YY_SPELL_CONVERTER_SRC_SPELL_CONVERT_PARSER_HPP_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 218 "src/spell-convert/parser.cpp" /* yacc.c:358  */
/* Unqualified %code blocks.  */
#line 13 "../src/spell-convert/parser.ypp" /* yacc.c:359  */

//#include "parser.hpp"
#include "lexer.hpp"

#include "../io/cxxstdio.hpp"

#include "../sexpr/lexer.hpp"

void yyerror(const char *msg) { FPRINTF(stderr, "Fatal: %s\n", msg); abort(); }

#line 231 "src/spell-convert/parser.cpp" /* yacc.c:359  */

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
#else
typedef signed char yytype_int8;
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
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   990

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  75
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  32
/* YYNRULES -- Number of rules.  */
#define YYNRULES  111
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  248

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   309

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,    14,    23,     2,
      73,    74,    12,    10,    16,    11,    17,    13,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    18,    19,
       8,     7,     9,     2,    15,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    21,     2,    22,    24,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    20,     2,     2,     2,     2,     2,
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
       5,     6,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   167,   167,   171,   179,   183,   191,   195,   205,   211,
     222,   227,   232,   237,   242,   253,   257,   263,   269,   281,
     285,   295,   300,   310,   315,   320,   330,   335,   340,   345,
     350,   355,   360,   365,   370,   375,   380,   385,   390,   395,
     400,   405,   410,   415,   420,   425,   430,   436,   441,   446,
     451,   462,   466,   476,   482,   493,   503,   508,   513,   523,
     528,   538,   543,   554,   564,   570,   581,   586,   591,   602,
     606,   617,   621,   631,   636,   641,   651,   657,   668,   673,
     678,   683,   688,   698,   708,   714,   725,   730,   740,   745,
     755,   760,   765,   770,   775,   780,   790,   795,   800,   805,
     810,   815,   820,   825,   830,   835,   840,   845,   850,   856,
     867,   871
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "STRING", "ID", "DIR", "'='",
  "'<'", "'>'", "'+'", "'-'", "'*'", "'/'", "'%'", "'@'", "','", "'.'",
  "':'", "';'", "'|'", "'['", "']'", "'&'", "'^'", "CONST", "PROCEDURE",
  "CALL", "SILENT", "LOCAL", "NONMAGIC", "SHL", "SHR", "EQ", "NEQ", "GTE",
  "LTE", "ANDAND", "OROR", "SCRIPT_DATA", "TO", "TOWARDS",
  "TELEPORT_ANCHOR", "SPELL", "LET", "IN", "END", "DARROW", "STRING_TY",
  "REQUIRE", "CATALYSTS", "COMPONENTS", "MANA", "CASTTIME", "SKIP",
  "ABORT", "BREAK", "EFFECT_", "ATEND", "ATTRIGGER", "PC_F", "NPC_F",
  "MOB_F", "ENTITY_F", "TARGET_F", "IF", "THEN", "ELSE", "FOREACH", "FOR",
  "DO", "SLEEP", "OR", "'('", "')'", "$accept", "spellconf", "semicolons",
  "proc_formals_list", "proc_formals_list_ne", "spellconf_option",
  "spell_flags", "argopt", "arg_ty", "value", "expr", "arg_list",
  "arg_list_ne", "location", "area", "spelldef", "defs", "def",
  "spellbody_list", "spellbody", "maybe_trigger", "maybe_end",
  "spellguard", "spellguard_list", "prereq", "items", "item_list", "item",
  "item_name", "selection", "effect", "effect_list", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,    61,    60,    62,
      43,    45,    42,    47,    37,    64,    44,    46,    58,    59,
     124,    91,    93,    38,    94,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   306,
     307,   308,   309,    40,    41
};
# endif

#define YYPACT_NINF -171

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-171)))

#define YYTABLE_NINF -13

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -171,    19,  -171,   156,    17,  -171,    46,    60,    64,  -171,
     104,    91,    65,     2,    80,  -171,  -171,  -171,    73,  -171,
    -171,    27,  -171,    28,    91,  -171,   780,    39,  -171,    91,
      98,    91,    35,    91,    91,   230,    91,    91,    91,    91,
      91,    91,    91,    91,   111,    91,    91,    91,    91,    91,
      91,    91,    91,    91,    91,    91,    99,    91,   780,  -171,
      49,   106,   812,   115,   107,   780,    50,   114,   556,  -171,
      -1,   937,   937,   958,   958,   184,   184,   184,  -171,    -1,
      -1,    -1,    -1,    -1,   937,   937,   937,   937,   907,   876,
      68,   588,   142,   145,    91,   134,    91,  -171,    91,    91,
      91,    81,   163,  -171,   484,    14,   844,   780,   620,   652,
      91,    -2,   148,  -171,   136,   138,   139,   140,    91,   123,
     160,    91,   163,  -171,  -171,  -171,  -171,    93,     6,    91,
      91,   684,    91,    91,    96,  -171,  -171,  -171,  -171,   462,
    -171,  -171,  -171,  -171,  -171,  -171,   166,   165,   716,   100,
     124,  -171,  -171,    91,   152,   152,    91,    91,   163,    87,
    -171,   157,  -171,   -25,  -171,   262,   306,    91,   748,   102,
      91,   163,   149,    91,  -171,  -171,  -171,   177,     3,   780,
      11,  -171,  -171,   780,   780,   141,   -13,   -25,   -14,   172,
     172,    62,  -171,  -171,   338,  -171,   180,   129,   143,    91,
     522,   198,   172,  -171,   194,  -171,    48,  -171,  -171,   163,
     153,  -171,    62,  -171,   172,  -171,  -171,    62,  -171,  -171,
    -171,   193,   163,   382,    91,    91,   157,   177,    63,    11,
    -171,  -171,   163,  -171,   154,   154,  -171,  -171,   163,   422,
     780,  -171,  -171,  -171,  -171,  -171,   163,  -171
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     4,     1,    15,     0,     5,     0,     0,     0,     3,
       0,     0,     0,     0,     0,    18,    16,    17,     0,    24,
      25,    27,    23,     0,     0,    26,    10,    56,    28,     0,
       6,     0,    19,    51,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    11,     8,
       0,     7,     0,     0,     0,    53,     0,    52,     0,    49,
      46,    34,    35,    29,    30,    31,    33,    32,    50,    38,
      36,    37,    39,    40,    45,    47,    42,    41,    43,    44,
       0,     0,     0,     0,     0,     0,     0,    48,     0,     0,
       0,     0,   110,     9,    46,     0,     0,    54,     0,     0,
       0,     0,     0,   108,     0,     0,     0,     0,     0,     0,
       0,     0,   110,     4,    13,    22,    21,     0,     0,     0,
       0,     0,     0,    51,     0,    99,    97,    98,   100,     0,
      93,    90,    95,    91,    92,    94,     0,     0,     0,     0,
     110,    20,     4,     0,     0,     0,     0,     0,   110,     0,
      14,    59,    64,     0,    73,     0,     0,     0,     0,     0,
      51,     0,     0,     0,   106,    96,   111,    61,     0,    78,
       0,    79,    80,    81,    82,    69,     0,    76,     0,     0,
       0,     0,    55,    57,     0,   101,     0,     0,   105,     0,
       0,     0,     0,     4,    89,    88,     0,    84,    87,   110,
      71,    67,     0,    75,     0,    65,    66,     0,    74,    58,
     107,     0,     0,     0,     0,     0,    60,    62,     0,     0,
      83,    70,   110,    68,    77,    76,   109,   104,     0,     0,
      63,    89,    86,    85,    72,   102,     0,   103
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -171,  -171,  -119,  -171,  -171,  -171,  -171,  -171,  -171,  -171,
     -11,   -84,  -171,  -171,  -171,  -171,  -171,  -171,  -125,  -108,
    -171,  -171,   -86,  -171,  -171,    72,  -171,    -9,     5,  -171,
    -170,  -105
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     3,    60,    61,     9,    10,    64,   127,    25,
      65,    66,    67,    27,    28,   160,   178,   203,   186,   162,
     210,   233,   163,   188,   164,   181,   206,   207,   208,   146,
     123,   124
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      26,   198,   212,   161,   150,   132,    36,   189,   201,    19,
      20,    21,    22,    35,   204,   205,    44,   149,    58,     2,
      62,    23,   190,    68,    11,    70,    71,    72,    73,    74,
      75,    76,    77,   177,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,   176,    91,   191,   202,   169,
     152,    12,   237,   185,    56,   153,   154,   155,   156,   157,
     213,   211,   125,   158,   229,    13,   241,   205,   245,    14,
     230,   133,    29,   187,   126,    30,   247,   226,    32,   159,
      57,   215,   216,   104,   227,   106,   197,   107,   108,   109,
      19,    20,    21,    22,    19,    20,    21,    22,    31,   131,
      33,    34,    23,    59,   231,   218,    23,   139,    63,    90,
     148,   153,   154,   155,   156,   157,    78,    70,   165,   166,
      95,   168,    93,    92,    97,    96,   234,   244,   187,   111,
      98,   235,    15,    16,    17,   217,   153,   154,   155,   156,
     157,   100,   179,     5,   158,   183,   184,    18,    35,   102,
     103,   112,   105,   134,   110,   135,   194,   136,   137,   138,
     159,     4,   200,   113,    24,   147,   140,   151,   111,   170,
     114,   172,   173,   180,   175,     5,   196,   189,   115,   116,
     117,     6,     7,   141,   142,   143,   144,   145,   223,   118,
     112,    36,   119,   120,   199,   121,     5,   122,     8,   220,
     209,    44,   113,   221,    45,   225,   228,    46,    47,   114,
     222,   232,   236,   239,   240,    48,    49,   115,   116,   117,
     243,   153,   154,   155,   156,   157,   191,   182,   118,   158,
       0,   119,   120,   242,   121,     0,   122,    36,    37,    38,
      39,    40,    41,    42,    43,   214,     0,    44,     0,     0,
      45,     0,     0,    46,    47,     0,     0,     0,     0,     0,
       0,    48,    49,    50,    51,    52,    53,    54,    55,    36,
      37,    38,    39,    40,    41,    42,    43,     0,     0,    44,
       0,     0,    45,     0,     0,    46,    47,     0,     0,     0,
       0,     0,     0,    48,    49,    50,    51,    52,    53,    54,
      55,     0,     0,     0,    69,     0,     0,     0,     0,     0,
       0,     0,     0,    36,    37,    38,    39,    40,    41,    42,
      43,     0,     0,    44,     0,     0,    45,     0,     0,    46,
      47,     0,     0,     0,     0,     0,   192,    48,    49,    50,
      51,    52,    53,    54,    55,    36,    37,    38,    39,    40,
      41,    42,    43,     0,     0,    44,     0,     0,    45,     0,
       0,    46,    47,     0,     0,     0,     0,     0,     0,    48,
      49,    50,    51,    52,    53,    54,    55,     0,     0,     0,
     193,     0,     0,     0,     0,     0,     0,     0,     0,    36,
      37,    38,    39,    40,    41,    42,    43,     0,     0,    44,
       0,     0,    45,     0,     0,    46,    47,     0,     0,     0,
       0,     0,   219,    48,    49,    50,    51,    52,    53,    54,
      55,     0,     0,     0,     0,     0,     0,     0,     0,    36,
      37,    38,    39,    40,    41,    42,    43,     0,     0,    44,
       0,     0,    45,     0,     0,    46,    47,     0,     0,     0,
       0,     0,   238,    48,    49,    50,    51,    52,    53,    54,
      55,     0,     0,     0,     0,     0,     0,     0,     0,    36,
      37,    38,    39,    40,    41,    42,    43,     0,     0,    44,
       0,     0,    45,     0,   -12,    46,    47,     0,     0,   -12,
       0,    36,   246,    48,    49,    50,    51,    52,    53,    54,
      55,    44,     0,   -12,     0,     0,     0,     0,     0,   -12,
     -12,     0,   -12,   -12,   -12,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   -12,   -12,   171,    36,
      37,    38,    39,    40,    41,    42,    43,     0,     0,    44,
       0,     0,    45,     0,     0,    46,    47,     0,     0,     0,
       0,     0,     0,    48,    49,    50,    51,    52,    53,    54,
      55,     0,   224,    36,    37,    38,    39,    40,    41,    42,
      43,     0,    99,    44,     0,     0,    45,     0,     0,    46,
      47,     0,     0,     0,     0,     0,     0,    48,    49,    50,
      51,    52,    53,    54,    55,    36,    37,    38,    39,    40,
      41,    42,    43,     0,     0,    44,   101,     0,    45,     0,
       0,    46,    47,     0,     0,     0,     0,     0,     0,    48,
      49,    50,    51,    52,    53,    54,    55,    36,    37,    38,
      39,    40,    41,    42,    43,     0,   129,    44,     0,     0,
      45,     0,     0,    46,    47,     0,     0,     0,     0,     0,
       0,    48,    49,    50,    51,    52,    53,    54,    55,    36,
      37,    38,    39,    40,    41,    42,    43,     0,   130,    44,
       0,     0,    45,     0,     0,    46,    47,     0,     0,     0,
       0,     0,     0,    48,    49,    50,    51,    52,    53,    54,
      55,    36,    37,    38,    39,    40,    41,    42,    43,     0,
     167,    44,     0,     0,    45,     0,     0,    46,    47,     0,
       0,     0,     0,     0,     0,    48,    49,    50,    51,    52,
      53,    54,    55,    36,    37,    38,    39,    40,    41,    42,
      43,     0,     0,    44,     0,   174,    45,     0,     0,    46,
      47,     0,     0,     0,     0,     0,     0,    48,    49,    50,
      51,    52,    53,    54,    55,    36,    37,    38,    39,    40,
      41,    42,    43,     0,     0,    44,     0,   195,    45,     0,
       0,    46,    47,     0,     0,     0,     0,     0,     0,    48,
      49,    50,    51,    52,    53,    54,    55,    36,    37,    38,
      39,    40,    41,    42,    43,     0,     0,    44,     0,     0,
      45,     0,     0,    46,    47,     0,     0,     0,     0,     0,
       0,    48,    49,    50,    51,    52,    53,    54,    55,    94,
      37,    38,    39,    40,    41,    42,    43,     0,     0,    44,
       0,     0,    45,     0,     0,    46,    47,     0,     0,     0,
       0,     0,     0,    48,    49,    50,    51,    52,    53,    54,
      55,   128,    37,    38,    39,    40,    41,    42,    43,     0,
       0,    44,     0,     0,    45,     0,     0,    46,    47,     0,
       0,     0,     0,     0,     0,    48,    49,    50,    51,    52,
      53,    54,    55,    36,    37,    38,    39,    40,    41,    42,
      43,     0,     0,    44,     0,     0,    45,     0,     0,    46,
      47,     0,     0,     0,     0,     0,     0,    48,    49,    50,
      51,    52,    53,    54,    36,    37,    38,    39,    40,    41,
      42,    43,     0,     0,    44,     0,     0,    45,     0,     0,
      46,    47,     0,     0,     0,     0,     0,     0,    48,    49,
      50,    51,    52,    53,    36,     0,     0,    39,    40,    41,
      42,    43,     0,     0,    44,     0,     0,    45,     0,     0,
      46,    47,     0,     0,     0,    36,     0,     0,    48,    49,
      41,    42,    43,     0,     0,    44,     0,     0,    45,     0,
       0,    46,    47,     0,     0,     0,     0,     0,     0,    48,
      49
};

static const yytype_int16 yycheck[] =
{
      11,   171,    16,   128,   123,     7,     7,    20,     5,     3,
       4,     5,     6,    24,     3,     4,    17,   122,    29,     0,
      31,    15,    47,    34,     7,    36,    37,    38,    39,    40,
      41,    42,    43,   152,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,   150,    57,    72,    45,   133,
      44,     5,   222,   158,    15,    49,    50,    51,    52,    53,
      74,    74,    48,    57,    16,     5,     3,     4,   238,     5,
      22,    73,     7,   159,    60,    73,   246,   202,     5,    73,
      41,   189,   190,    94,   203,    96,   170,    98,    99,   100,
       3,     4,     5,     6,     3,     4,     5,     6,    18,   110,
      73,    73,    15,     5,   209,   191,    15,   118,    73,    10,
     121,    49,    50,    51,    52,    53,     5,   128,   129,   130,
       5,   132,    16,    74,    74,    18,   212,   232,   214,     5,
      16,   217,    28,    29,    30,    73,    49,    50,    51,    52,
      53,    73,   153,    19,    57,   156,   157,    43,   159,     7,
       5,    27,    18,     5,    73,    19,   167,    19,    19,    19,
      73,     5,   173,    39,    73,     5,    43,    74,     5,    73,
      46,     5,     7,    21,    74,    19,    74,    20,    54,    55,
      56,    25,    26,    60,    61,    62,    63,    64,   199,    65,
      27,     7,    68,    69,    45,    71,    19,    73,    42,    19,
      59,    17,    39,    74,    20,     7,    12,    23,    24,    46,
      67,    58,    19,   224,   225,    31,    32,    54,    55,    56,
     229,    49,    50,    51,    52,    53,    72,   155,    65,    57,
      -1,    68,    69,   228,    71,    -1,    73,     7,     8,     9,
      10,    11,    12,    13,    14,    73,    -1,    17,    -1,    -1,
      20,    -1,    -1,    23,    24,    -1,    -1,    -1,    -1,    -1,
      -1,    31,    32,    33,    34,    35,    36,    37,    38,     7,
       8,     9,    10,    11,    12,    13,    14,    -1,    -1,    17,
      -1,    -1,    20,    -1,    -1,    23,    24,    -1,    -1,    -1,
      -1,    -1,    -1,    31,    32,    33,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    74,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     7,     8,     9,    10,    11,    12,    13,
      14,    -1,    -1,    17,    -1,    -1,    20,    -1,    -1,    23,
      24,    -1,    -1,    -1,    -1,    -1,    74,    31,    32,    33,
      34,    35,    36,    37,    38,     7,     8,     9,    10,    11,
      12,    13,    14,    -1,    -1,    17,    -1,    -1,    20,    -1,
      -1,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    31,
      32,    33,    34,    35,    36,    37,    38,    -1,    -1,    -1,
      74,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     7,
       8,     9,    10,    11,    12,    13,    14,    -1,    -1,    17,
      -1,    -1,    20,    -1,    -1,    23,    24,    -1,    -1,    -1,
      -1,    -1,    74,    31,    32,    33,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     7,
       8,     9,    10,    11,    12,    13,    14,    -1,    -1,    17,
      -1,    -1,    20,    -1,    -1,    23,    24,    -1,    -1,    -1,
      -1,    -1,    70,    31,    32,    33,    34,    35,    36,    37,
      38,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     7,
       8,     9,    10,    11,    12,    13,    14,    -1,    -1,    17,
      -1,    -1,    20,    -1,     0,    23,    24,    -1,    -1,     5,
      -1,     7,    70,    31,    32,    33,    34,    35,    36,    37,
      38,    17,    -1,    19,    -1,    -1,    -1,    -1,    -1,    25,
      26,    -1,    28,    29,    30,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    42,    43,    66,     7,
       8,     9,    10,    11,    12,    13,    14,    -1,    -1,    17,
      -1,    -1,    20,    -1,    -1,    23,    24,    -1,    -1,    -1,
      -1,    -1,    -1,    31,    32,    33,    34,    35,    36,    37,
      38,    -1,    40,     7,     8,     9,    10,    11,    12,    13,
      14,    -1,    16,    17,    -1,    -1,    20,    -1,    -1,    23,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    31,    32,    33,
      34,    35,    36,    37,    38,     7,     8,     9,    10,    11,
      12,    13,    14,    -1,    -1,    17,    18,    -1,    20,    -1,
      -1,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    31,
      32,    33,    34,    35,    36,    37,    38,     7,     8,     9,
      10,    11,    12,    13,    14,    -1,    16,    17,    -1,    -1,
      20,    -1,    -1,    23,    24,    -1,    -1,    -1,    -1,    -1,
      -1,    31,    32,    33,    34,    35,    36,    37,    38,     7,
       8,     9,    10,    11,    12,    13,    14,    -1,    16,    17,
      -1,    -1,    20,    -1,    -1,    23,    24,    -1,    -1,    -1,
      -1,    -1,    -1,    31,    32,    33,    34,    35,    36,    37,
      38,     7,     8,     9,    10,    11,    12,    13,    14,    -1,
      16,    17,    -1,    -1,    20,    -1,    -1,    23,    24,    -1,
      -1,    -1,    -1,    -1,    -1,    31,    32,    33,    34,    35,
      36,    37,    38,     7,     8,     9,    10,    11,    12,    13,
      14,    -1,    -1,    17,    -1,    19,    20,    -1,    -1,    23,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    31,    32,    33,
      34,    35,    36,    37,    38,     7,     8,     9,    10,    11,
      12,    13,    14,    -1,    -1,    17,    -1,    19,    20,    -1,
      -1,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    31,
      32,    33,    34,    35,    36,    37,    38,     7,     8,     9,
      10,    11,    12,    13,    14,    -1,    -1,    17,    -1,    -1,
      20,    -1,    -1,    23,    24,    -1,    -1,    -1,    -1,    -1,
      -1,    31,    32,    33,    34,    35,    36,    37,    38,     7,
       8,     9,    10,    11,    12,    13,    14,    -1,    -1,    17,
      -1,    -1,    20,    -1,    -1,    23,    24,    -1,    -1,    -1,
      -1,    -1,    -1,    31,    32,    33,    34,    35,    36,    37,
      38,     7,     8,     9,    10,    11,    12,    13,    14,    -1,
      -1,    17,    -1,    -1,    20,    -1,    -1,    23,    24,    -1,
      -1,    -1,    -1,    -1,    -1,    31,    32,    33,    34,    35,
      36,    37,    38,     7,     8,     9,    10,    11,    12,    13,
      14,    -1,    -1,    17,    -1,    -1,    20,    -1,    -1,    23,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    31,    32,    33,
      34,    35,    36,    37,     7,     8,     9,    10,    11,    12,
      13,    14,    -1,    -1,    17,    -1,    -1,    20,    -1,    -1,
      23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    31,    32,
      33,    34,    35,    36,     7,    -1,    -1,    10,    11,    12,
      13,    14,    -1,    -1,    17,    -1,    -1,    20,    -1,    -1,
      23,    24,    -1,    -1,    -1,     7,    -1,    -1,    31,    32,
      12,    13,    14,    -1,    -1,    17,    -1,    -1,    20,    -1,
      -1,    23,    24,    -1,    -1,    -1,    -1,    -1,    -1,    31,
      32
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    76,     0,    77,     5,    19,    25,    26,    42,    80,
      81,     7,     5,     5,     5,    28,    29,    30,    43,     3,
       4,     5,     6,    15,    73,    84,    85,    88,    89,     7,
      73,    18,     5,    73,    73,    85,     7,     8,     9,    10,
      11,    12,    13,    14,    17,    20,    23,    24,    31,    32,
      33,    34,    35,    36,    37,    38,    15,    41,    85,     5,
      78,    79,    85,    73,    82,    85,    86,    87,    85,    74,
      85,    85,    85,    85,    85,    85,    85,    85,     5,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      10,    85,    74,    16,     7,     5,    18,    74,    16,    16,
      73,    18,     7,     5,    85,    18,    85,    85,    85,    85,
      73,     5,    27,    39,    46,    54,    55,    56,    65,    68,
      69,    71,    73,   105,   106,    48,    60,    83,     7,    16,
      16,    85,     7,    73,     5,    19,    19,    19,    19,    85,
      43,    60,    61,    62,    63,    64,   104,     5,    85,   106,
      77,    74,    44,    49,    50,    51,    52,    53,    57,    73,
      90,    93,    94,    97,    99,    85,    85,    16,    85,    86,
      73,    66,     5,     7,    19,    74,   106,    77,    91,    85,
      21,   100,   100,    85,    85,   106,    93,    97,    98,    20,
      47,    72,    74,    74,    85,    19,    74,    86,   105,    45,
      85,     5,    45,    92,     3,     4,   101,   102,   103,    59,
      95,    74,    16,    74,    73,    94,    94,    73,    97,    74,
      19,    74,    67,    85,    40,     7,    93,    77,    12,    16,
      22,   106,    58,    96,    97,    97,    19,   105,    70,    85,
      85,     3,   103,   102,   106,   105,    70,   105
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    75,    76,    76,    77,    77,    78,    78,    79,    79,
      80,    80,    80,    80,    80,    81,    81,    81,    81,    82,
      82,    83,    83,    84,    84,    84,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    86,    86,    87,    87,    88,    89,    89,    89,    90,
      90,    91,    91,    92,    93,    93,    94,    94,    94,    95,
      95,    96,    96,    97,    97,    97,    98,    98,    99,    99,
      99,    99,    99,   100,   101,   101,   102,   102,   103,   103,
     104,   104,   104,   104,   104,   104,   105,   105,   105,   105,
     105,   105,   105,   105,   105,   105,   105,   105,   105,   105,
     106,   106
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     3,     0,     2,     0,     1,     1,     3,
       3,     4,     6,     7,     8,     0,     2,     2,     2,     0,
       5,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     4,     3,
       3,     0,     1,     1,     3,     8,     1,     8,     9,     1,
       4,     1,     3,     3,     1,     3,     3,     3,     4,     0,
       2,     0,     2,     1,     3,     3,     1,     3,     2,     2,
       2,     2,     2,     3,     1,     3,     3,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     2,     2,     2,
       2,     4,     7,     8,     6,     4,     3,     5,     1,     6,
       0,     3
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

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
#ifndef YYINITDEPTH
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
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

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
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
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
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:
#line 172 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyvsp[0].top)->dump();
}
#line 1633 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 6:
#line 191 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vs) = new std::vector<RString>();
}
#line 1641 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 7:
#line 196 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vs) = (yyvsp[0].vs);
}
#line 1649 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 8:
#line 206 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vs) = new std::vector<RString>();
    (yyval.vs)->push_back(*(yyvsp[0].s));
}
#line 1658 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 9:
#line 212 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vs) = (yyvsp[-2].vs);
    (yyval.vs)->push_back(*(yyvsp[0].s));
}
#line 1667 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 10:
#line 223 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.top) = new Assignment{*(yyvsp[-2].s), (yyvsp[0].expr)};
}
#line 1675 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 11:
#line 228 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.top) = new Constant{*(yyvsp[-2].s), (yyvsp[0].expr)};
}
#line 1683 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 12:
#line 233 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.top) = new Teleport{*(yyvsp[-4].s), (yyvsp[-2].expr), (yyvsp[0].expr)};
}
#line 1691 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 13:
#line 238 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.top) = new Procedure{*(yyvsp[-5].s), (yyvsp[-3].vs), (yyvsp[0].ve)};
}
#line 1699 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 14:
#line 243 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.top) = new Spell{(yyvsp[-7].vs), *(yyvsp[-5].s), (yyvsp[-4].spellarg), (yyvsp[-2].expr), (yyvsp[0].spelldef)};
}
#line 1707 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 15:
#line 253 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vs) = new std::vector<RString>();
}
#line 1715 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 16:
#line 258 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vs) = (yyvsp[-1].vs);
    (yyval.vs)->push_back("LOCAL");
}
#line 1724 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 17:
#line 264 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vs) = (yyvsp[-1].vs);
    (yyval.vs)->push_back("NONMAGIC");
}
#line 1733 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 18:
#line 270 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vs) = (yyvsp[-1].vs);
    (yyval.vs)->push_back("SILENT");
}
#line 1742 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 19:
#line 281 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.spellarg) = new SpellArg{};
}
#line 1750 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 20:
#line 286 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.spellarg) = new SpellArg{*(yyvsp[-3].s), *(yyvsp[-1].s)};
}
#line 1758 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 21:
#line 296 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.s) = new RString("PC");
}
#line 1766 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 22:
#line 301 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.s) = new RString("STRING");
}
#line 1774 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 23:
#line 311 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.s) = (yyvsp[0].s);
}
#line 1782 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 24:
#line 316 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.s) = (yyvsp[0].s);
}
#line 1790 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 25:
#line 321 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.s) = (yyvsp[0].s);
}
#line 1798 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 26:
#line 331 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new SimpleExpr{*(yyvsp[0].s)};
}
#line 1806 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 27:
#line 336 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new SimpleExpr{*(yyvsp[0].s)};
}
#line 1814 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 28:
#line 341 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = (yyvsp[0].expr);
}
#line 1822 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 29:
#line 346 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "+", (yyvsp[0].expr)};
}
#line 1830 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 30:
#line 351 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "-", (yyvsp[0].expr)};
}
#line 1838 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 31:
#line 356 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "*", (yyvsp[0].expr)};
}
#line 1846 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 32:
#line 361 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "%", (yyvsp[0].expr)};
}
#line 1854 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 33:
#line 366 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "/", (yyvsp[0].expr)};
}
#line 1862 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 34:
#line 371 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "<", (yyvsp[0].expr)};
}
#line 1870 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 35:
#line 376 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), ">", (yyvsp[0].expr)};
}
#line 1878 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 36:
#line 381 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "&", (yyvsp[0].expr)};
}
#line 1886 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 37:
#line 386 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "^", (yyvsp[0].expr)};
}
#line 1894 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 38:
#line 391 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "|", (yyvsp[0].expr)};
}
#line 1902 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 39:
#line 396 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "<<", (yyvsp[0].expr)};
}
#line 1910 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 40:
#line 401 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), ">>", (yyvsp[0].expr)};
}
#line 1918 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 41:
#line 406 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "<=", (yyvsp[0].expr)};
}
#line 1926 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 42:
#line 411 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), ">=", (yyvsp[0].expr)};
}
#line 1934 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 43:
#line 416 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "&&", (yyvsp[0].expr)};
}
#line 1942 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 44:
#line 421 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "||", (yyvsp[0].expr)};
}
#line 1950 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 45:
#line 426 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "==", (yyvsp[0].expr)};
}
#line 1958 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 46:
#line 431 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    // convert to ==
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "==", (yyvsp[0].expr)};
}
#line 1967 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 47:
#line 437 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), "!=", (yyvsp[0].expr)};
}
#line 1975 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 48:
#line 442 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new CallExpr{*(yyvsp[-3].s), (yyvsp[-1].vx)};
}
#line 1983 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 49:
#line 447 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = (yyvsp[-1].expr);
}
#line 1991 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 50:
#line 452 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new BinExpr{(yyvsp[-2].expr), ".", new SimpleExpr(*(yyvsp[0].s))};
}
#line 1999 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 51:
#line 462 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vx) = new std::vector<Expression *>();
}
#line 2007 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 52:
#line 467 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vx) = (yyvsp[0].vx);
}
#line 2015 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 53:
#line 477 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vx) = new std::vector<Expression *>();
    (yyval.vx)->push_back((yyvsp[0].expr));
}
#line 2024 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 54:
#line 483 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vx) = (yyvsp[-2].vx);
    (yyval.vx)->push_back((yyvsp[0].expr));
}
#line 2033 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 55:
#line 494 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.loc) = new Location{(yyvsp[-5].expr), (yyvsp[-3].expr), (yyvsp[-1].expr)};
}
#line 2041 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 56:
#line 504 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new AreaLoc{(yyvsp[0].loc)};
}
#line 2049 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 57:
#line 509 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new AreaRect{(yyvsp[-7].loc), (yyvsp[-3].expr), (yyvsp[-1].expr)};
}
#line 2057 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 58:
#line 514 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.expr) = new AreaBar{(yyvsp[-8].loc), (yyvsp[-6].expr), (yyvsp[-3].expr), (yyvsp[-1].expr)};
}
#line 2065 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 59:
#line 524 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.spelldef) = new SpellDef{new std::vector<Assignment *>{}, (yyvsp[0].vb)};
}
#line 2073 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 60:
#line 529 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.spelldef) = new SpellDef{(yyvsp[-2].va), (yyvsp[0].vb)};
}
#line 2081 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 61:
#line 539 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.va) = new std::vector<Assignment *>();
}
#line 2089 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 62:
#line 544 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.va) = (yyvsp[-2].va);
    (yyval.va)->push_back((yyvsp[-1].a));
}
#line 2098 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 63:
#line 555 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.a) = new Assignment{*(yyvsp[-2].s), (yyvsp[0].expr)};
}
#line 2106 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 64:
#line 565 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vb) = new std::vector<SpellBod *>();
    (yyval.vb)->push_back((yyvsp[0].b));
}
#line 2115 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 65:
#line 571 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vb) = (yyvsp[-2].vb);
    (yyval.vb)->push_back((yyvsp[0].b));
}
#line 2124 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 66:
#line 582 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.b) = new SpellBodGuarded{(yyvsp[-2].g), (yyvsp[0].b)};
}
#line 2132 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 67:
#line 587 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.b) = new SpellBodList{(yyvsp[-1].vb)};
}
#line 2140 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 68:
#line 592 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.b) = new SpellBodEffect{(yyvsp[-2].ve), (yyvsp[-1].ve), (yyvsp[0].ve)};
}
#line 2148 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 69:
#line 602 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.ve) = nullptr;
}
#line 2156 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 70:
#line 607 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.ve) = (yyvsp[0].ve);
}
#line 2164 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 71:
#line 617 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.ve) = nullptr;
}
#line 2172 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 72:
#line 622 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.ve) = (yyvsp[0].ve);
}
#line 2180 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 73:
#line 632 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.g) = (yyvsp[0].g);
}
#line 2188 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 74:
#line 637 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.g) = new SpellGuardOr((yyvsp[-2].g), (yyvsp[0].g));
}
#line 2196 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 75:
#line 642 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.g) = new SpellGuardList{(yyvsp[-1].vg)};
}
#line 2204 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 76:
#line 652 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vg) = new std::vector<SpellGuard *>();
    (yyval.vg)->push_back((yyvsp[0].g));
}
#line 2213 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 77:
#line 658 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vg) = (yyvsp[-2].vg);
    (yyval.vg)->push_back((yyvsp[0].g));
}
#line 2222 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 78:
#line 669 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.g) = new SpellGuardRequire{(yyvsp[0].expr)};
}
#line 2230 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 79:
#line 674 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.g) = new SpellGuardCatalysts{(yyvsp[0].vit)};
}
#line 2238 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 80:
#line 679 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.g) = new SpellGuardComponents{(yyvsp[0].vit)};
}
#line 2246 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 81:
#line 684 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.g) = new SpellGuardMana{(yyvsp[0].expr)};
}
#line 2254 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 82:
#line 689 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.g) = new SpellGuardCasttime{(yyvsp[0].expr)};
}
#line 2262 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 83:
#line 699 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vit) = (yyvsp[-1].vit);
}
#line 2270 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 84:
#line 709 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vit) = new std::vector<Item *>();
    (yyval.vit)->push_back((yyvsp[0].it));
}
#line 2279 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 85:
#line 715 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.vit) = (yyvsp[-2].vit);
    (yyval.vit)->push_back((yyvsp[0].it));
}
#line 2288 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 86:
#line 726 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.it) = new Item{*(yyvsp[-2].s), *(yyvsp[0].s)};
}
#line 2296 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 87:
#line 731 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.it) = new Item{RString(), *(yyvsp[0].s)};
}
#line 2304 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 88:
#line 741 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.s) = (yyvsp[0].s);
}
#line 2312 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 89:
#line 746 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.s) = (yyvsp[0].s);
}
#line 2320 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 90:
#line 756 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.s) = new RString{"PC"};
}
#line 2328 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 91:
#line 761 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.s) = new RString{"MOB"};
}
#line 2336 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 92:
#line 766 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.s) = new RString{"ENTITY"};
}
#line 2344 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 93:
#line 771 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.s) = new RString{"SPELL"};
}
#line 2352 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 94:
#line 776 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.s) = new RString{"TARGET"};
}
#line 2360 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 95:
#line 781 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.s) = new RString{"NPC"};
}
#line 2368 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 96:
#line 791 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.e) = new EffectList{(yyvsp[-1].ve)};
}
#line 2376 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 97:
#line 796 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.e) = new SimpleEffect{"SKIP"};
}
#line 2384 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 98:
#line 801 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.e) = new SimpleEffect{"ABORT"};
}
#line 2392 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 99:
#line 806 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.e) = new SimpleEffect{"END"};
}
#line 2400 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 100:
#line 811 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.e) = new SimpleEffect{"BREAK"};
}
#line 2408 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 101:
#line 816 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.e) = new Assignment(*(yyvsp[-3].s), (yyvsp[-1].expr));
}
#line 2416 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 102:
#line 821 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.e) = new ForeachEffect{*(yyvsp[-5].s), *(yyvsp[-4].s), (yyvsp[-2].expr), (yyvsp[0].e)};
}
#line 2424 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 103:
#line 826 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.e) = new ForEffect{*(yyvsp[-6].s), (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].e)};
}
#line 2432 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 104:
#line 831 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.e) = new IfEffect{(yyvsp[-4].expr), (yyvsp[-2].e), (yyvsp[0].e)};
}
#line 2440 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 105:
#line 836 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.e) = new IfEffect{(yyvsp[-2].expr), (yyvsp[0].e)};
}
#line 2448 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 106:
#line 841 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.e) = new SleepEffect{(yyvsp[-1].expr)};
}
#line 2456 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 107:
#line 846 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.e) = new CallExpr{*(yyvsp[-4].s), (yyvsp[-2].vx)};
}
#line 2464 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 108:
#line 851 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    AString tmp = sexpr::escape(*(yyvsp[0].s));
    (yyval.e) = new ScriptEffect{RString(tmp)};
}
#line 2473 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 109:
#line 857 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.e) = new ExplicitCallEffect{*(yyvsp[-4].s), (yyvsp[-2].vx)};
}
#line 2481 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 110:
#line 867 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    (yyval.ve) = new std::deque<Effect *>();
}
#line 2489 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;

  case 111:
#line 872 "../src/spell-convert/parser.ypp" /* yacc.c:1646  */
    {
    // because of grammar problems, doing this right generates reduce/reduce conflicts
    (yyval.ve) = (yyvsp[0].ve);
    (yyval.ve)->push_front((yyvsp[-2].e));
}
#line 2499 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
    break;


#line 2503 "src/spell-convert/parser.cpp" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
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
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
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

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
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
  return yyresult;
}
#line 881 "../src/spell-convert/parser.ypp" /* yacc.c:1906  */

// Nothing to see here, move along
