#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "multi.h"
#include "semantic.h"

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
        void parseArguments();
        void parseArgument();

        semantic sem;

    public:
        Parser(const std::vector<SymbolRow>& symbols, bool enableTrace = true);
        bool run();
};