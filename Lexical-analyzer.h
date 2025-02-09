#ifndef _LEXICAL_ANALYZER_H_
#define _LEXICAL_ANALYZER_H_

// 定义词法单元类型
typedef enum {
    // 关键字
    IF, ELSE, WHILE, RETURN, VOID, INT,
    // 标识符和常量
    ID, NUM,
    // 运算符
    PLUS, MINUS, TIMES, DIVIDE,
    LT, GT, EQ, NEQ, LEQ, GEQ, ASSIGN,
    // 分隔符
    SEMI, COMMA, LPAREN, RPAREN, LBRACE, RBRACE,
    LBRACKET, RBRACKET,
    // 特殊标记
    ENDFILE, ERROR
} TokenType;

// 定义词法单元结构
typedef struct {
    TokenType type;
    char lexeme[100];
    int lineNo;
} Token;

// 函数声明
Token getToken(void);
void ungetToken(void);

#endif // _LEXICAL_ANALYZER_H_ 