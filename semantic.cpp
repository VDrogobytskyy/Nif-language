#include "semantic.h"

#include <iostream>

VarSymb* semantic::findVar(const std::string& lexem) {
    for (auto it = scope.rbegin(); it != scope.rend(); ++it) {
        auto found = it->find(lexem);
        if (found != it->end()) {
            return &(found->second);
        }
    }
    return nullptr;
}

void semantic::enter_scope(){
    scope.push_back({});
}

void semantic::exit_scope(){
    if(scope.size() > 1){
        scope.pop_back();
    }
}

void semantic::declareVar(std::string lexem, std::string type, bool ifconst, int line){
    auto &curScope = scope.back();

    if(curScope.find(lexem) != curScope.end()){
        std::cerr << "[Semantic Error]\nDuplicated! lexem: " << lexem << " :already declared in line: " << line << std::endl;
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

    VarSymb* s = findVar(lexem);

    if(!s){
        std::cerr << "[Semantic Error]\nUndeclared variable!\nvariable: " << lexem << " in line: " << line << std::endl;
        exit(1);
    }

    if(s->isConst && s->isAssigned == true){
        std::cerr << "[Semantic Error]\nIn line: " << line << ": ConstantModification - cannor reassign const variable: " << lexem << std::endl;
        exit(1);
    }

    bool all_good = (s->VarType == exprType) || (s->VarType == "Double" && exprType == "Int");

    if(!all_good){
        std::cerr << "[Semantic Error]\nIn line: " << line << ": TypeMismatch - cannot assign: " << exprType 
                    << "\nto variable: " << lexem << " of type: " << s->VarType << "\n" << std::endl; 
        exit(1);
    }

    s->isAssigned = true;
}