#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdlib>

struct VarSymb{
    std::string VarName;
    std::string VarType;
    bool isConst;
    bool isAssigned;
    int lineNum;
};

struct FuncParam {
    std::string name;
    std::string type;
    bool isRef;
};
struct FuncSymb {
    std::string name;
    std::string returnType;
    std::vector<FuncParam> params;
    int line;
};

class semantic{
    private:
        std::vector<std::unordered_map<std::string, VarSymb>> scope;
        int loop_depth = 0;

        std::unordered_map<std::string, FuncSymb> tableOfFuncs;

        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> tableOfRecords;
    public:
        semantic(){
            scope.push_back({});
        }

        VarSymb* findVar(const std::string& lexem);
        void enter_scope();
        void exit_scope();

        void enter_loop();
        void exit_loop();
        void loop_control(const std::string op, int line);

        void declareVar(std::string lexem, std::string type, bool ifconst, int line);
        void checkAssignType(std::string lexem, std::string exprType, int line);

        std::string useVar(std::string lexem, int line);

        void declareFunc(const std::string& name, const std::string& returnType, const std::vector<FuncParam>& params, int line);
        FuncSymb* findFunc(const std::string& name);

        void declareRecord(const std::string& recName, const std::unordered_map<std::string, std::string>& fields);
        bool hasRecord(const std::string& recName);
        bool hasRecordField(const std::string& recName, const std::string& fieldName);
        std::string getRecordFieldType(const std::string& recName, const std::string& fieldName);
};