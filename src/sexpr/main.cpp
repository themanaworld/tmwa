#include <stack>
#include <map>

#include "../io/cxxstdio.hpp"

#include "lexer.hpp"
#include "parser.hpp"

#include "../poison.hpp"

enum Spacing
{
    LINES,
    SIMPLE,
    SPACES,
    SPACES_1,
    SPACES_2,
    SPACES_3,
    SPACES_4,
};

static
void do_spacing(bool& first, Spacing& sp, int depth)
{
    if (first)
    {
        first = false;
        return;
    }
    switch (sp)
    {
    case LINES:
        PRINTF("\n%*s", (depth - 1) * 4, "");
        return;
    case SPACES:
    case SIMPLE:
        PRINTF(" ");
        return;
    case SPACES_1:
        PRINTF(" ");
        sp = LINES;
        return;
    case SPACES_2:
        PRINTF(" ");
        sp = SPACES_1;
        return;
    case SPACES_3:
        PRINTF(" ");
        sp = SPACES_2;
        return;
    case SPACES_4:
        PRINTF(" ");
        sp = SPACES_3;
        return;
    }
}

static
void adjust_spacing(Spacing& sp, ZString val)
{
    std::map<ZString, Spacing> spaces =
    {
        {"BLOCK", LINES},
        {"GUARD", LINES},
        {"DISABLED", LINES},
        {"PROCEDURE", SPACES_2},
        {"SPELL", SPACES_4},
        {"IF", SPACES_1},
        {"set_script_variable", SPACES_2},
    };
    auto it = spaces.find(val);
    if (it != spaces.end())
        sp = it->second;
}

int main()
{
    if (1 == 1)
    {
        sexpr::Lexer lexer("/dev/stdin");
        sexpr::SExpr sexpr;
        while (sexpr::parse(lexer, sexpr))
        {
            PRINTF("");
        }
        if (lexer.peek() != sexpr::TOK_EOF)
        {
            lexer.span().error(STRPRINTF("Incomplete: %s: %s\n",
                        sexpr::token_name(lexer.peek()), lexer.val_string()));
        }
        return 0;
    }

    std::stack<Spacing> spacing;
    spacing.push(LINES);
    sexpr::Lexer lexer("/dev/stdin");
    bool first = true;
    while (sexpr::Lexeme tok = lexer.peek())
    {
        switch (tok)
        {
        case sexpr::TOK_OPEN:
            if (spacing.top() == SIMPLE)
                spacing.top() = LINES;
            do_spacing(first, spacing.top(), spacing.size());
            PRINTF("(");
            spacing.push(SIMPLE);
            first = true;
            break;
        case sexpr::TOK_CLOSE:
            PRINTF(")");
            spacing.pop();
            first = false;
            break;
        case sexpr::TOK_STRING:
            do_spacing(first, spacing.top(), spacing.size());
            PRINTF("%s", sexpr::escape(lexer.val_string()));
            break;
        case sexpr::TOK_TOKEN:
            do_spacing(first, spacing.top(), spacing.size());
            PRINTF("%s", lexer.val_string());
            adjust_spacing(spacing.top(), lexer.val_string());
            break;
        default:
            abort();
        }
        lexer.adv();
    }
    PRINTF("\n");
}
