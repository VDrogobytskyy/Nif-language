#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <stdexcept>
#include <sstream>
#include <set>
#include <map>
#include <vector>
#include <unordered_map>

struct SymbolRow {
    int line;            // номер рядка у файлі
    std::string lexeme;  // текст лексеми "x", "10", "var", "=="
    std::string token;   // тип токена
    std::string index;   // індекс у таблиці ідентифікаторів/констант (або пустий рядок)
};

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

        std::vector<SymbolRow> tableOfSymb;
        std::map<std::string, int> tableOfId;
        std::map<std::string, std::pair<std::string, int>> tableOfConst;


        char nextChar();

        void putCharBack();

        std::string classOfChar(char c);

        int nextState(int curState, const std::string& cls);

        void processing();

        void fail(int errorState);

    public:
        Lexer(const std::string& source);

        bool run();

        void printTables() const;
};


int main(int argc, char* argv[]){

    if(argc != 2){
        std::cerr << "incorrect amount of argc" << std::endl;
        return 1;
    }

    std::string source;

    try{
        std::ifstream file(argv[1]); 

        if(!file.is_open()){
            throw std::runtime_error("ERROR! Couldn`t find file: " + std::string(argv[1]));
        }

        std::string file_name = argv[1];
        std::string required_file_type = ".nif";

        int type_check = file_name.find(required_file_type);
        if(type_check != std::string::npos){
            std::cout << "File type confirmed!" << std::endl;
        }else{
            throw std::runtime_error("incorrect file type! Expected .nif file.");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        source = buffer.str();

        if(source.length() != 0){
            std::cout << "File: " << argv[1] << " opened succesfully! And contained " << source.length() << " bytes!" << std::endl;
        }else{
            std::cout << "File is empty" << std::endl;
            return 0;
        }

    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
        
        return 1;
    }

    return 0;
}