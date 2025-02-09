#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "Lexical-analyzer.h"

// 全局变量
static char lineBuf[1024];  // 行缓冲区
static int linePos = 0;     // 当前位置
static int bufSize = 0;     // 缓冲区大小
static int lineNo = 1;      // 行号
static int EOF_flag = 0;    // 文件结束标志
static Token currentToken;   // 当前token
static Token savedToken;     // 保存的token
static int hasToken = 0;     // 是否有保存的token

// 关键字表
static struct {
    char* str;
    TokenType tok;
} keywords[] = {
    {"if", IF},
    {"else", ELSE},
    {"while", WHILE},
    {"return", RETURN},
    {"void", VOID},
    {"int", INT},
    {"input", ID},
    {"output", ID},
    {NULL, 0}
};

// 定义一个缓冲区来存储所有的token
#define MAX_TOKENS 1000
Token tokenBuffer[MAX_TOKENS];
int tokenCount = 0;

// 获取下一个字符
static char getNextChar() {
    if (linePos >= bufSize) {
        if (fgets(lineBuf, sizeof(lineBuf), stdin) != NULL) {
            bufSize = strlen(lineBuf);
            linePos = 0;
            lineNo++;
            return lineBuf[linePos++];
        } else {
            EOF_flag = 1;
            return EOF;
        }
    }
    return lineBuf[linePos++];
}

// 回退一个字符
static void ungetNextChar() {
    if (!EOF_flag) linePos--;
}

// 跳过空白字符
static void skipWhitespace() {
    char c;
    while ((c = getNextChar()) != EOF) {
        if (!isspace(c)) {
            ungetNextChar();
            break;
        }
    }
}

// 跳过注释
static void skipComment() {
    char c;
    int state = 0;
    
    while ((c = getNextChar()) != EOF) {
        switch (state) {
            case 0:
                if (c == '*') state = 1;
                break;
            case 1:
                if (c == '/') return;
                else if (c != '*') state = 0;
                break;
        }
    }
}

// 查找关键字
static TokenType lookupKeyword(char* s) {
    int i = 0;
    while (keywords[i].str != NULL) {
        if (strcmp(keywords[i].str, s) == 0)
            return keywords[i].tok;
        i++;
    }
    return ID;
}

// 实现ungetToken函数
void ungetToken(void) {
    hasToken = 1;
    savedToken = currentToken;
}

// 获取下一个词法单元
Token getToken() {
    if (hasToken) {
        hasToken = 0;
        return savedToken;
    }
    
    Token token = {ERROR, "", lineNo};
    char tokenString[100] = "";
    int tokenStringIndex = 0;
    int save;
    
    // 状态机的状态
    enum StateType {START, INNUM, INID, INASSIGN, INLT, INGT, INNEQ} state = START;
    
    while (1) {
        char c = getNextChar();
        save = 1;
        
        switch (state) {
            case START:
                if (isdigit(c))
                    state = INNUM;
                else if (isalpha(c))
                    state = INID;
                else if (c == '=')
                    state = INASSIGN;
                else if (c == '<')
                    state = INLT;
                else if (c == '>')
                    state = INGT;
                else if (c == '!')
                    state = INNEQ;
                else {
                    save = 0;
                    switch (c) {
                        case EOF:
                            token.type = ENDFILE;
                            return token;
                        case '+':
                            token.type = PLUS;
                            break;
                        case '-':
                            token.type = MINUS;
                            break;
                        case '*':
                            token.type = TIMES;
                            break;
                        case '/':
                            c = getNextChar();
                            if (c == '*') {
                                save = 0;
                                skipComment();
                                continue;
                            } else {
                                ungetNextChar();
                                token.type = DIVIDE;
                            }
                            break;
                        case ';':
                            token.type = SEMI;
                            break;
                        case ',':
                            token.type = COMMA;
                            break;
                        case '(':
                            token.type = LPAREN;
                            break;
                        case ')':
                            token.type = RPAREN;
                            break;
                        case '{':
                            token.type = LBRACE;
                            break;
                        case '}':
                            token.type = RBRACE;
                            break;
                        case '[':
                            token.type = LBRACKET;
                            break;
                        case ']':
                            token.type = RBRACKET;
                            break;
                        default:
                            if (!isspace(c)) {
                                token.type = ERROR;
                                save = 1;
                            }
                            break;
                    }
                    if (token.type != ERROR) {
                        tokenString[tokenStringIndex++] = c;
                        tokenString[tokenStringIndex] = '\0';
                        strcpy(token.lexeme, tokenString);
                        return token;
                    }
                }
                break;
                
            case INNUM:
                if (!isdigit(c)) {
                    ungetNextChar();
                    save = 0;
                    token.type = NUM;
                    strcpy(token.lexeme, tokenString);
                    return token;
                }
                break;
                
            case INID:
                if (!isalnum(c)) {
                    ungetNextChar();
                    save = 0;
                    token.type = lookupKeyword(tokenString);
                    strcpy(token.lexeme, tokenString);
                    return token;
                }
                break;
                
            case INASSIGN:
                if (c == '=') {
                    token.type = EQ;
                } else {
                    ungetNextChar();
                    save = 0;
                    token.type = ASSIGN;
                }
                strcpy(token.lexeme, tokenString);
                return token;
                
            case INLT:
                if (c == '=') {
                    token.type = LEQ;
                } else {
                    ungetNextChar();
                    save = 0;
                    token.type = LT;
                }
                strcpy(token.lexeme, tokenString);
                return token;
                
            case INGT:
                if (c == '=') {
                    token.type = GEQ;
                } else {
                    ungetNextChar();
                    save = 0;
                    token.type = GT;
                }
                strcpy(token.lexeme, tokenString);
                return token;
                
            case INNEQ:
                if (c == '=') {
                    token.type = NEQ;
                    strcpy(token.lexeme, "!=");
                    return token;
                } else {
                    ungetNextChar();
                    save = 0;
                    token.type = ERROR;
                    return token;
                }
        }
        
        if (save && tokenStringIndex < sizeof(tokenString) - 1) {
            tokenString[tokenStringIndex++] = c;
            tokenString[tokenStringIndex] = '\0';
        }
    }
}

// 修改打印函数以符合要求的格式
void printToken(Token token) {
    char* typeStr;
    switch (token.type) {
        case IF:
            typeStr = "T_IF";
            break;
        case ELSE:
            typeStr = "T_ELSE";
            break;
        case WHILE:
            typeStr = "T_WHILE";
            break;
        case RETURN:
            typeStr = "T_RETURN";
            break;
        case VOID:
            typeStr = "T_VOID";
            break;
        case INT:
            typeStr = "T_INT";
            break;
        case ID:
            typeStr = "T_IDENTIFIER";
            break;
        case NUM:
            typeStr = "T_NUMBER";
            break;
        case PLUS:
            typeStr = "T_OPERATOR";
            break;
        case MINUS:
            typeStr = "T_OPERATOR";
            break;
        case TIMES:
            typeStr = "T_OPERATOR";
            break;
        case DIVIDE:
            typeStr = "T_OPERATOR";
            break;
        case LT:
            typeStr = "T_OPERATOR";
            break;
        case GT:
            typeStr = "T_OPERATOR";
            break;
        case EQ:
            typeStr = "T_OPERATOR";
            break;
        case NEQ:
            typeStr = "T_OPERATOR";
            break;
        case LEQ:
            typeStr = "T_OPERATOR";
            break;
        case GEQ:
            typeStr = "T_OPERATOR";
            break;
        case ASSIGN:
            typeStr = "T_OPERATOR";
            break;
        case SEMI:
        case COMMA:
        case LPAREN:
        case RPAREN:
        case LBRACE:
        case RBRACE:
            typeStr = "T_DELIMITER";
            break;
        case ENDFILE:
            typeStr = "T_EOF";
            break;
        case ERROR:
            typeStr = "T_ERROR";
            break;
        default:
            typeStr = "T_UNKNOWN";
    }
    printf("Token Type: %s, Value: '%s'\n", typeStr, token.lexeme);
}

// 修改主函数以支持完整代码输入
int main() {
    Token token;
    char input[1024];
    char* filename = "temp_input.txt";
    FILE* tempFile;
    
    printf("请输入C--代码（换行输入'END'结束）：\n");
    
    // 创建临时文件存储输入
    tempFile = fopen(filename, "w");
    if (tempFile == NULL) {
        printf("无法创建临时文件\n");
        return 1;
    }
    
    // 读取输入直到遇到"END"
    while (fgets(input, sizeof(input), stdin)) {
        if (strncmp(input, "END", 3) == 0) {
            break;
        }
        fputs(input, tempFile);
    }
    fclose(tempFile);
    
    // 重定向标准输入到临时文件
    freopen(filename, "r", stdin);
    
    // 进行词法分析并存储结果
    printf("\n词法分析结果：\n");
    do {
        token = getToken();
        if (token.type != ERROR && tokenCount < MAX_TOKENS) {
            tokenBuffer[tokenCount++] = token;
        }
    } while (token.type != ENDFILE);
    
    // 打印所有token
    for (int i = 0; i < tokenCount; i++) {
        printToken(tokenBuffer[i]);
    }
    
    // 清理临时文件
    fclose(stdin);
    remove(filename);
    
    return 0;
}
