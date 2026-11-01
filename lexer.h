#pragma once
#include <map>
#include <string>
#include <set>
#include <vector>

#include "multi.h"

class Lexer {
private:
    std::string sourceCode;
    int lenCode;
    int numChar;
    int numLine;
    int state;
    char currentChar;
    std::string lexeme;

    std::unordered_map<std::string, std::string> tokenTable;
    std::set<int> F;      
    std::set<int> Fstar;   
    std::set<int> Ferror;

    std::map<std::pair<int, std::string>, int> stf;

    std::vector<SymbolRow> tableOfSymb;
    std::map<std::string, int> tableOfId;
    std::map<std::string, std::pair<std::string, int>> tableOfConst;

    char nextChar();
    void putCharBack();
    char peekChar();
    std::string classOfChar(char c);
    int nextState(int curState, const std::string& cls);
    void processing();
    void fail(int errorState);

public:
    Lexer(const std::string& source);
    bool run();
    void printTables() const;
    const std::vector<SymbolRow> &getTableOfSymb() const { return tableOfSymb; }
};