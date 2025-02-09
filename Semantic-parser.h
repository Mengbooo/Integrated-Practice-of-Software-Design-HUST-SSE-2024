#ifndef _SEMANTIC_PARSER_H_
#define _SEMANTIC_PARSER_H_

#include "Lexical-analyzer.h"

// 前向声明
// struct SymbolEntry;

// 符号种类
typedef enum {
    KIND_VARIABLE,
    KIND_FUNCTION,
    KIND_PARAMETER
} SymbolKind;

// 符号表项
typedef struct SymbolEntry {
    char name[100];          // 符号名
    SymbolKind kind;         // 符号种类
    DataType type;           // 数据类型
    int lineNo;             // 定义行号
    union {
        struct {
            DataType returnType;    // 返回类型
            int paramCount;         // 参数个数
            DataType* paramTypes;   // 参数类型列表
        } func;
        struct {
            int isArray;            // 是否是数组
            int arraySize;          // 数组大小
        } var;
    } attr;
    struct SymbolEntry* next;  // 链表指针
} SymbolEntry;

// 类型定义
typedef enum {
    TYPE_INT,
    TYPE_VOID,
    TYPE_ERROR
} DataType;

// 函数声明
void enterScope(void);
void exitScope(void);
void checkVarDeclaration(const char* name, DataType type, int isArray, int arraySize, int lineNo);
void checkFunctionDeclaration(const char* name, DataType returnType, int paramCount, DataType* paramTypes, int lineNo);
DataType checkBinaryOp(DataType left, DataType right, TokenType op, int lineNo);
void semanticError(const char* message, int lineNo);
struct SymbolEntry* lookup(const char* name);

#endif // _SEMANTIC_PARSER_H_ 