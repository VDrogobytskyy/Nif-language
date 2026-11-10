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
        std::cerr << "[Semantic Error]\nIn line: " << line << "Undeclared variable!\nvariable: " << lexem << std::endl;
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

std::string semantic::useVar(std::string lexem, int line){
    VarSymb* s = findVar(lexem);

    if(!s){
        std::cerr << "[Semantic Error]\nIn line: " << line << ": Undeclared variable!\nvariable: " << lexem << std::endl;
        exit(1);
    }

    if(s->isAssigned == false){
        std::cerr << "[Semantic Error]\nIn line: " << line << ": Unassigned variable - cannot use: " << lexem << "\n" << std::endl;
        exit(1);
    }

    return s->VarType;
}

void semantic::enter_loop(){
    loop_depth++;
}

void semantic::exit_loop(){
    if(loop_depth > 0){
        loop_depth--;
    }
}

void semantic::loop_control(const std::string op, int line){
    if(loop_depth == 0){
        std::cerr << "[Semantic Error]\nIn line: " << line << ": IllegalLoopControl - operator: " << op << ", outside loop\n" << std::endl;
        exit(1); 
    }
}

void semantic::declareFunc(const std::string& name, const std::string& returnType, const std::vector<FuncParam>& params, int line) {
    if (tableOfFuncs.find(name) != tableOfFuncs.end()) {
        std::cerr << "[Semantic Error]\nIn line: " << line << ": Duplicate function declaration: " << name << std::endl;
        exit(1);
    }
    tableOfFuncs[name] = {name, returnType, params, line};
}
FuncSymb* semantic::findFunc(const std::string& name) {
    auto it = tableOfFuncs.find(name);
    if (it != tableOfFuncs.end()) {
        return &(it->second);
    }
    return nullptr;
}

void semantic::declareRecord(const std::string& recName, const std::unordered_map<std::string, std::string>& fields) {
    tableOfRecords[recName] = fields;
}
bool semantic::hasRecord(const std::string& recName) {
    return tableOfRecords.find(recName) != tableOfRecords.end();
}
bool semantic::hasRecordField(const std::string& recName, const std::string& fieldName) {
    auto it = tableOfRecords.find(recName);
    if (it != tableOfRecords.end()) {
        return it->second.find(fieldName) != it->second.end();
    }
    return false;
}
std::string semantic::getRecordFieldType(const std::string& recName, const std::string& fieldName) {
    return tableOfRecords[recName][fieldName];
}