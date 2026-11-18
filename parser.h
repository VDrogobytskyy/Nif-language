#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "multi.h"
#include "semantic.h"
#include <map>
#include <utility>
#include <fstream>
#include <iomanip>

class Parser {
    private:
        std::vector<SymbolRow> tableOfSymb;
        int len_tableOfSymb;
        int numRow;          // поточний номер запису в таблиці
        int indt;            // поточна глибина відступу для консольного дерева
        const int stepIndt = 2; // крок відступу
        bool trace;          // чи друкувати красиве дерево розбору

        std::vector<FuncParam> pendingParams;
        bool hasReturn = false;
        // Допоміжні методи навігації та відступів
        std::string nextIndt();
        std::string predIndt();
        SymbolRow getSymb();
        bool isStartOfExpression(const SymbolRow& s);
        void failParse(const std::string& errorType, const std::string& expected = "");
        bool parseToken(const std::string& lexeme, const std::string& token);

        // 1. Верхній рівень та оголошення
        void parseProgram();
        void parseModuleItem();
        void parseImportDeclaration();
        void parseTypeAliasDeclaration();
        void parseRecordDeclaration();
        void parseFieldDeclaration();
        void parseFunctionDeclaration();
        void parseParameters();
        void parseParameter();
        std::string parseType();
        std::string parseReturnType();
        void parseVariableDeclaration();
        void parseConstantDeclaration();

        // 2. Блоки та інструкції керування
        void parseBlock();
        void parseBlockItem();
        void parseStatement();
        void parseIfStatement();
        void parseWhileStatement();
        void parseForStatement();
        void parseInputStatement();
        void parseOutputStatement();
        void parseReturnStatement();
        void parseBreakStatement();
        void parseContinueStatement();
        void parseIdentifierStatement(); // Присвоювання LValue = Expr або виклик функції

        std::string parseExpression();
        std::string parseLogicalOrExpr();
        std::string parseLogicalAndExpr();
        std::string parseEqualityExpr();
        std::string parseRelationalExpr();
        std::string parseAddExpr();
        std::string parseMultExpr();
        std::string parsePowerExpr();
        std::string parseUnaryExpr();
        std::string parsePostfixExpr();
        std::string parsePrimaryExpr();
        int parseArguments();
        void parseArgument();

        semantic sem;

        std::string checkBinaryOp(const std::string& lType, const std::string& op, const std::string& rType, int line);

        std::vector<std::pair<std::string, std::string>> postfixCode;   // (лексема, токен)
        std::map<std::string, int> tableOfLabels;                       // мітка -> індекс у .code
        std::vector<std::pair<std::string, std::string>> psmVars;       // (ім'я, тип PSM)
        std::map<std::string, std::string> psmVarTypes;                 // для унікальності імен
        std::unordered_map<std::string, std::string> pendingPsmNames;   // psm-ім'я ітератора for
        std::vector<std::pair<std::string, std::string>> loopLabels;    // (мітка continue, мітка break)

        int labelCounter = 0;
        bool genEnabled = true;       // false - всередині функцій, відмінних від main

        std::string exitLabel;        // мітка кінця програми (для return у main)
        void postfixCodeGen(const std::string& lex, const std::string& tok);
        std::string createLabel();
        void setValLabel(const std::string& label);
        void placeLabel(const std::string& label);
        std::string toPsmType(const std::string& nifType, int line);
        std::string registerPsmVar(const std::string& name, const std::string& nifType, int line);
        std::string psmName(const std::string& name);
        void genUnsupported(const std::string& what, int line);
        void genIntToDoubleAt(size_t pos);
        std::string genBinaryOp(const std::string& lType, const std::string& op, const std::string& rType, size_t lEnd, int line);

    public:
        Parser(const std::vector<SymbolRow>& symbols, bool enableTrace = true);
        bool run();

        void printPostfixCode();
        bool savePostfixCode(const std::string& fileName);
};