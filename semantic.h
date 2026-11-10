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


class semantic{
    private:
        std::vector<std::unordered_map<std::string, VarSymb>> scope;
        int loop_depth = 0;
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
};