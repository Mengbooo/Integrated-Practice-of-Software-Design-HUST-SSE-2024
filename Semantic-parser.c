#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Lexical-analyzer.h"

// 符号类型
typedef enum {
    TYPE_INT,
    TYPE_VOID,
    TYPE_ERROR
} DataType;

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

// 作用域
typedef struct Scope {
    SymbolEntry* table;        // 当前作用域的符号表
    struct Scope* parent;      // 父作用域
    int level;                 // 作用域层级
} Scope;

// 全局变量
static Scope* currentScope = NULL;  // 当前作用域
static int semanticErrorIndex = 0;       // 错误标志

// 创建新作用域
static Scope* newScope(void) {
    Scope* scope = (Scope*)malloc(sizeof(Scope));
    scope->table = NULL;
    scope->parent = currentScope;
    scope->level = currentScope ? currentScope->level + 1 : 0;
    return scope;
}

// 进入新作用域
static void enterScope(void) {
    currentScope = newScope();
}

// 退出当前作用域
static void exitScope(void) {
    // 释放符号表
    SymbolEntry* entry = currentScope->table;
    while (entry) {
        SymbolEntry* next = entry->next;
        if (entry->kind == KIND_FUNCTION && entry->attr.func.paramTypes)
            free(entry->attr.func.paramTypes);
        free(entry);
        entry = next;
    }
    
    Scope* parent = currentScope->parent;
    free(currentScope);
    currentScope = parent;
}

// 在当前作用域中查找符号
static SymbolEntry* lookupCurrentScope(const char* name) {
    SymbolEntry* entry = currentScope->table;
    while (entry) {
        if (strcmp(entry->name, name) == 0)
            return entry;
        entry = entry->next;
    }
    return NULL;
}

// 在所有作用域中查找符号
static SymbolEntry* lookup(const char* name) {
    Scope* scope = currentScope;
    while (scope) {
        SymbolEntry* entry = scope->table;
        while (entry) {
            if (strcmp(entry->name, name) == 0)
                return entry;
            entry = entry->next;
        }
        scope = scope->parent;
    }
    return NULL;
}

// 插入符号
static void insert(SymbolEntry* entry) {
    entry->next = currentScope->table;
    currentScope->table = entry;
}

// 语义错误处理
static void semanticError(const char* message, int lineNo) {
    fprintf(stderr, "\nSemantic error at line %d: %s\n", lineNo, message);
    semanticErrorIndex = 1;
}

// 类型检查函数
static DataType checkBinaryOp(DataType left, DataType right, TokenType op, int lineNo) {
    if (left == TYPE_ERROR || right == TYPE_ERROR)
        return TYPE_ERROR;
    
    if (left == TYPE_VOID || right == TYPE_VOID) {
        semanticError("Cannot perform operation on void type", lineNo);
        return TYPE_ERROR;
    }
    
    switch (op) {
        case PLUS:
        case MINUS:
        case TIMES:
        case DIVIDE:
            if (left == TYPE_INT && right == TYPE_INT)
                return TYPE_INT;
            semanticError("Type mismatch in arithmetic operation", lineNo);
            return TYPE_ERROR;
            
        case LT:
        case GT:
        case LEQ:
        case GEQ:
        case EQ:
        case NEQ:
            if (left == TYPE_INT && right == TYPE_INT)
                return TYPE_INT;
            semanticError("Type mismatch in comparison", lineNo);
            return TYPE_ERROR;
            
        default:
            semanticError("Invalid operator", lineNo);
            return TYPE_ERROR;
    }
}

// 检查函数调用
static DataType checkFunctionCall(const char* name, int paramCount, DataType* paramTypes, int lineNo) {
    SymbolEntry* entry = lookup(name);
    if (!entry) {
        semanticError("Undefined function", lineNo);
        return TYPE_ERROR;
    }
    
    if (entry->kind != KIND_FUNCTION) {
        semanticError("Not a function", lineNo);
        return TYPE_ERROR;
    }
    
    if (entry->attr.func.paramCount != paramCount) {
        semanticError("Wrong number of parameters", lineNo);
        return TYPE_ERROR;
    }
    
    for (int i = 0; i < paramCount; i++) {
        if (entry->attr.func.paramTypes[i] != paramTypes[i]) {
            semanticError("Parameter type mismatch", lineNo);
            return TYPE_ERROR;
        }
    }
    
    return entry->attr.func.returnType;
}

// 检查变量声明
static void checkVarDeclaration(const char* name, DataType type, int isArray, int arraySize, int lineNo) {
    if (lookupCurrentScope(name)) {
        semanticError("Variable already declared in current scope", lineNo);
        return;
    }
    
    SymbolEntry* entry = (SymbolEntry*)malloc(sizeof(SymbolEntry));
    strcpy(entry->name, name);
    entry->kind = KIND_VARIABLE;
    entry->type = type;
    entry->lineNo = lineNo;
    entry->attr.var.isArray = isArray;
    entry->attr.var.arraySize = arraySize;
    entry->next = NULL;
    
    insert(entry);
}

// 检查函数声明
static void checkFunctionDeclaration(const char* name, DataType returnType, int paramCount, 
                                   DataType* paramTypes, int lineNo) {
    if (lookupCurrentScope(name)) {
        semanticError("Function already declared", lineNo);
        return;
    }
    
    SymbolEntry* entry = (SymbolEntry*)malloc(sizeof(SymbolEntry));
    strcpy(entry->name, name);
    entry->kind = KIND_FUNCTION;
    entry->type = returnType;
    entry->lineNo = lineNo;
    entry->attr.func.returnType = returnType;
    entry->attr.func.paramCount = paramCount;
    entry->attr.func.paramTypes = (DataType*)malloc(paramCount * sizeof(DataType));
    memcpy(entry->attr.func.paramTypes, paramTypes, paramCount * sizeof(DataType));
    entry->next = NULL;
    
    insert(entry);
}

// 主函数
int main() {
    printf("请输入C--代码（换行输入'END'结束）：\n");
    
    // 初始化全局作用域
    enterScope();
    
    // 预定义输入输出函数
    DataType inputParamTypes[] = {TYPE_VOID};
    checkFunctionDeclaration("input", TYPE_INT, 1, inputParamTypes, 0);
    
    DataType outputParamTypes[] = {TYPE_INT};
    checkFunctionDeclaration("output", TYPE_VOID, 1, outputParamTypes, 0);
    
    // 进行语法和语义分析
    Token token;
    char input[1024];
    char* filename = "temp_input.txt";
    FILE* tempFile;
    
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
    
    // 进行语法分析，同时进行语义检查
    parse();
    
    // 清理临时文件
    fclose(stdin);
    remove(filename);
    
    // 清理全局作用域
    exitScope();
    
    if (!semanticErrorIndex)
        printf("\nSemantic analysis completed successfully!\n");
    else
        printf("\nSemantic analysis completed with errors.\n");
    
    return 0;
}
