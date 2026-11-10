#include "semantic.h"

#include <iostream>


void semantic::declareVar(std::string lexem, std::string type, bool ifconst, int line){
    auto &curScope = scope.back();

    if(curScope.find(lexem) != curScope.end()){
        std::cout << "Duplicated! lexem: " << lexem << "already declared in line: " << line << std::endl;
        exit(1);
    }

    VarSymb s;
    s.VarName = lexem;
    s.isConst = ifconst;
    s.VarType = type;
    s.isAssigned = false;
    s.lineNum = line;

    curScope[lexem] = s;
}