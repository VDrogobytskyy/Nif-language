#include "semantic.h"

#include <iostream>


void semantic::declareVar(std::string lexem, std::string type, bool ifconst, int line){
    auto &curScope = scope.back();

    if(curScope.find(lexem) != curScope.end()){
        std::cerr << "[Semantic Error] Duplicated! lexem: " << lexem << " :already declared in line: " << line << std::endl;
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

void semantic::checkAssignType(std::string lexem, std::string exprType, int line){
    auto &curScope = scope.back();

    if(curScope.find(lexem) == curScope.end()){
        std::cerr << "[Semantic Error] Unassigned lexem! lexem: " << lexem << "in line: " << line << std::endl;
        exit(1);
    }

    VarSymb &s = curScope[lexem];
    bool all_good = (s.VarType == exprType) || (s.VarType == "Double" && exprType == "Int");

    if(!all_good){
        std::cerr << "[Semantic Error] In line: " << line << ": TypeMismatch - cannot assign: " << exprType 
                    << "\nto variable: " << lexem << " of type: " << s.VarType << "\n" << std::endl; 
        exit(1);
    }

    s.isAssigned = true;
}