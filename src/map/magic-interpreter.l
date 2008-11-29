%{
#include "magic-interpreter.h"
#include "magic-interpreter-parser.h"

#ifdef HEADING
#  undef HEADING
#endif

#define FIXLOC magic_frontend_lloc.first_line = magic_frontend_lineno

#define HEADING(dir) { magic_frontend_lval.i = dir; FIXLOC; return DIR; }

%}

%option yylineno
%option noyywrap
%option prefix="magic_frontend_"
%option nounput
%option noinput
%option bison-bridge bison-locations

%%

"S"			HEADING(0);
"SW"			HEADING(1);
"W"			HEADING(2);
"NW"			HEADING(3);
"N"			HEADING(4);
"NE"			HEADING(5);
"E"			HEADING(6);
"SE"			HEADING(7);
"="			{FIXLOC; return '=';}
"=="			{FIXLOC; return EQ;}
"<>"			{FIXLOC; return NEQ;}
"!="			{FIXLOC; return NEQ;}
">"			{FIXLOC; return '>';}
"<"			{FIXLOC; return '<';}
">="			{FIXLOC; return GTE;}
"<="			{FIXLOC; return LTE;}
"("			{FIXLOC; return '(';}
")"			{FIXLOC; return ')';}
"+"			{FIXLOC; return '+';}
"-"			{FIXLOC; return '-';}
"*"			{FIXLOC; return '*';}
"/"			{FIXLOC; return '/';}
"%"			{FIXLOC; return '%';}
"&&"			{FIXLOC; return ANDAND;}
"||"			{FIXLOC; return OROR;}
";"			{FIXLOC; return ';';}
":"			{FIXLOC; return ':';}
","			{FIXLOC; return ',';}
"@"			{FIXLOC; return '@';}
"|"			{FIXLOC; return '|';}
"["			{FIXLOC; return '[';}
"]"			{FIXLOC; return ']';}
"&"			{FIXLOC; return '&';}
"^"			{FIXLOC; return '^';}
"."			{FIXLOC; return '.';}
"<<"			{FIXLOC; return SHL;}
">>"			{FIXLOC; return SHR;}
"PROCEDURE"		{FIXLOC; return PROCEDURE;}
"CALL"			{FIXLOC; return CALL;}
"OR"			{FIXLOC; return OR;}
"TO"			{FIXLOC; return TO;}
"TOWARDS"		{FIXLOC; return TOWARDS;}
"TELEPORT-ANCHOR"	{FIXLOC; return TELEPORT_ANCHOR;}
"SILENT"		{FIXLOC; return SILENT;}
"LOCAL"			{FIXLOC; return LOCAL;}
"NONMAGIC"		{FIXLOC; return NONMAGIC;}
"SPELL"			{FIXLOC; return SPELL;}
"LET"			{FIXLOC; return LET;}
"IN"			{FIXLOC; return IN;}
"END"			{FIXLOC; return END;}
"=>"			{FIXLOC; return DARROW;}
"STRING"		{FIXLOC; return STRING_TY;}
"REQUIRE"		{FIXLOC; return REQUIRE;}
"CATALYSTS"		{FIXLOC; return CATALYSTS;}
"COMPONENTS"		{FIXLOC; return COMPONENTS;}
"MANA"			{FIXLOC; return MANA;}
"CASTTIME"		{FIXLOC; return CASTTIME;}
"SKIP"			{FIXLOC; return SKIP;}
"ABORT"			{FIXLOC; return ABORT;}
"BREAK"			{FIXLOC; return BREAK;}
"EFFECT"		{FIXLOC; return EFFECT;}
"ATEND"			{FIXLOC; return ATEND;}
"ATTRIGGER"		{FIXLOC; return ATTRIGGER;}
"CONST"			{FIXLOC; return CONST;}
"PC"			{FIXLOC; return PC_F;}
"NPC"			{FIXLOC; return NPC_F;}
"MOB"			{FIXLOC; return MOB_F;}
"ENTITY"		{FIXLOC; return ENTITY_F;}
"TARGET"		{FIXLOC; return TARGET_F;}
"IF"			{FIXLOC; return IF;}
"THEN"			{FIXLOC; return THEN;}
"ELSE"			{FIXLOC; return ELSE;}
"FOREACH"		{FIXLOC; return FOREACH;}
"FOR"			{FIXLOC; return FOR;}
"DO"			{FIXLOC; return DO;}
"WAIT"			{FIXLOC; return SLEEP;}

\{([^\}]|\\.)*\} { char *string = strdup(yytext);
                   magic_frontend_lval.s = string;
		   FIXLOC;
                   return SCRIPT_DATA;
                   }

\"([^\"]|\\.)*\" { char *string = strdup(yytext + 1);
                   char *src = string;
                   char *dst = string;
                   while (*src && *src != '"')
                       if (*src == '\\') {
                           *dst++ = src[1];
                           src += 2;
                       } else
                           *dst++ = *src++;
                   *dst = '\0'; /* terminate */
                   magic_frontend_lval.s = string;
		   FIXLOC;
                   return STRING;
                   }

"-"?[0-9]+	    { magic_frontend_lval.i = atoi(yytext);
		      FIXLOC;
                      return INT; }

"0x"[0-9a-fA-F]+    { magic_frontend_lval.i = strtol(yytext + 2, NULL, 16);
		      FIXLOC;
                      return INT; }

[a-zA-Z][-_a-zA-Z0-9?!]*  { magic_frontend_lval.s = strdup(yytext);
		            FIXLOC;
                            return ID; }

"#".*$		/* Ignore comments */
"//".*$		/* Ignore comments */
[ \n\t\r]	/* ignore whitespace */	
.       	fprintf(stderr, "%s: Unexpected character in line %d\n", MAGIC_CONFIG_FILE, magic_frontend_lineno);


%%
