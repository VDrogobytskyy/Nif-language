#pragma once

#include <vector>
#include <string>
#include <unordered_map>

struct VarSymb{
    std::string VarName;
    std::string VarType;
    bool isConst;
    bool isAsigned;
    int lineNum;
};


class semantic{
    private:
        std::vector<std::unordered_map<std::string, VarSymb>> scope;
    public:
        semantic(){
            scope.push_back({});
        }

        void declareVar();
};