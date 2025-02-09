#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 包含词法分析器的头文件
#include "Lexical-analyzer.h"

// 包含语义分析器的头文件
#include "Semantic-parser.h"

// 全局变量
static Token currentToken;  // 当前token
static FILE* sourceFile;    // 源文件
static int syntaxErrorIndex = 0; // 语法错误标志

// 函数声明
static void program(void);
static void declaration_list(void);
static void declaration(void);
static void var_declaration(void);

static void fun_declaration(void);
static void params(void);
static void param_list(void);
static void param(void);
static void compound_stmt(void);
static void local_declarations(void);
static void statement_list(void);
static void statement(void);
static void expression_stmt(void);
static void selection_stmt(void);
static void iteration_stmt(void);
static void return_stmt(void);
static void expression(void);
static void var(void);
static void simple_expression(void);
static void additive_expression(void);
static void term(void);
static void factor(void);
static void call(void);
static void args(void);
static void arg_list(void);

// 错误处理
static void syntaxError(const char* message) {
    fprintf(stderr, "\nSyntax error at line %d: %s\n", currentToken.lineNo, message);
    syntaxErrorIndex = 1;
}


// 获取下一个token
static Token* getNextToken(void) {
    currentToken = getToken();
    return &currentToken;
}

// 匹配token
static void match(TokenType expected) {
    if (currentToken.type == expected)
        getNextToken();
    else {
        char message[100];
        sprintf(message, "Unexpected token -> %s", currentToken.lexeme);
        syntaxError(message);
    }
}

// 语法分析主函数
void parse(void) {
    currentToken = *getNextToken();  // 获取第一个token
    program();       // 开始分析
    if (!syntaxErrorIndex)
        printf("\nParsing completed successfully!\n");
    else
        printf("\nParsing completed with errors.\n");
}

// 程序 -> 声明列表
static void program(void) {
    declaration_list();
}

// 声明列表 -> 声明 声明列表 | ε
static void declaration_list(void) {
    while (currentToken.type != ENDFILE) {
        declaration();
    }
}

// 声明 -> 变量声明 | 函数声明
static void declaration(void) {
    TokenType type = currentToken.type;
    if (type != INT && type != VOID) {
        syntaxError("Expected type specifier");
        return;
    }
    
    match(type);
    char* name = currentToken.lexeme;
    match(ID);
    
    // 转换类型
    DataType dataType = (type == INT) ? TYPE_INT : TYPE_VOID;
    
    if (currentToken.type == LPAREN) {
        // 函数声明
        enterScope();  // 进入新的作用域
        fun_declaration();
        exitScope();   // 退出作用域
    } else {
        // 变量声明
        checkVarDeclaration(name, dataType, 0, 0, currentToken.lineNo);
        var_declaration();
    }
}

// 变量声明 -> ; | [NUM] ;
static void var_declaration(void) {
    if (currentToken.type == LBRACKET) {
        match(LBRACKET);
        match(NUM);
        match(RBRACKET);
    }
    match(SEMI);
}

// 函数声明 -> (params) compound_stmt
static void fun_declaration(void) {
    match(LPAREN);
    params();
    match(RPAREN);
    compound_stmt();
}

// 参数列表 -> void | param_list
static void params(void) {
    if (currentToken.type == VOID) {
        match(VOID);
        if (currentToken.type != RPAREN)
            param_list();
    } else
        param_list();
}

// 复合语句 -> { local_declarations statement_list }
static void compound_stmt(void) {
    match(LBRACE);
    local_declarations();
    statement_list();
    match(RBRACE);
}

// 语句列表 -> statement statement_list | ε
static void statement_list(void) {
    while (currentToken.type != RBRACE && currentToken.type != ENDFILE) {
        statement();
    }
}

// 语句 -> expression_stmt | compound_stmt | selection_stmt | iteration_stmt | return_stmt
static void statement(void) {
    switch (currentToken.type) {
        case IF:
            selection_stmt();
            break;
        case WHILE:
            iteration_stmt();
            break;
        case RETURN:
            return_stmt();
            break;
        case LBRACE:
            compound_stmt();
            break;
        default:
            expression_stmt();
            break;
    }
}

// 表达式语句 -> expression ; | ;
static void expression_stmt(void) {
    if (currentToken.type != SEMI)
        expression();
    match(SEMI);
}

// 选择语句 -> if (expression) statement | if (expression) statement else statement
static void selection_stmt(void) {
    match(IF);
    match(LPAREN);
    expression();
    match(RPAREN);
    statement();
    if (currentToken.type == ELSE) {
        match(ELSE);
        statement();
    }
}

// 循环语句 -> while (expression) statement
static void iteration_stmt(void) {
    match(WHILE);
    match(LPAREN);
    expression();
    match(RPAREN);
    statement();
}

// 返回语句 -> return ; | return expression ;
static void return_stmt(void) {
    match(RETURN);
    if (currentToken.type != SEMI)
        expression();
    match(SEMI);
}

// 表达式 -> var = expression | simple_expression
static void expression(void) {
    if (currentToken.type == ID) {
        TokenType nextType = getToken().type;
        ungetToken();
        if (nextType == ASSIGN) {
            var();
            match(ASSIGN);
            // 检查赋值类型
            DataType leftType = lookup(currentToken.lexeme)->type;
            expression();
            DataType rightType = TYPE_INT;  // 简化处理，假设表达式返回int
            if (leftType != rightType) {
                semanticError("Type mismatch in assignment", currentToken.lineNo);
            }
        } else
            simple_expression();
    } else
        simple_expression();
}

// 简单表达式 -> additive_expression relop additive_expression | additive_expression
static void simple_expression(void) {
    additive_expression();
    TokenType type = currentToken.type;
    if (type == LT || type == GT || type == LEQ || type == GEQ || type == EQ || type == NEQ) {
        match(type);
        additive_expression();
    }
}

// 加法表达式 -> term { addop term }
static void additive_expression(void) {
    term();
    while (currentToken.type == PLUS || currentToken.type == MINUS) {
        TokenType type = currentToken.type;
        match(type);
        term();
    }
}

// 项 -> factor { mulop factor }
static void term(void) {
    factor();
    while (currentToken.type == TIMES || currentToken.type == DIVIDE) {
        TokenType type = currentToken.type;
        match(type);
        factor();
    }
}

// 因子 -> (expression) | var | call | NUM
static void factor(void) {
    switch (currentToken.type) {
        case LPAREN:
            match(LPAREN);
            expression();
            match(RPAREN);
            break;
        case ID:
            {
                TokenType nextType = getToken().type;
                ungetToken();
                if (nextType == LPAREN)
                    call();
                else
                    var();
            }
            break;
        case NUM:
            match(NUM);
            break;
        default:
            syntaxError("Unexpected token in factor");
            break;
    }
}

// 主函数
int main() {
    printf("请输入C--代码（换行输入'END'结束）：\n");
    parse();
    return 0;
}
