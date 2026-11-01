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
#include <iomanip>
#include <algorithm>

#include "lexer.h"
#include "parser.h"
#include "multi.h"


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "incorrect amount of argc" << std::endl;
        return 1;
    }

    std::string source;
    try {
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            throw std::runtime_error("ERROR! Couldn`t find file: " + std::string(argv[1]));
        }

        std::string file_name = argv[1];
        if (file_name.find(".nif") == std::string::npos) {
            throw std::runtime_error("incorrect file type! Expected .nif file.");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        source = buffer.str();

        if (source.empty()) {
            std::cout << "File is empty" << std::endl;
            return 0;
        }
        std::cout << "File: " << argv[1] << " opened succesfully! And contained " << source.length() << " bytes!\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    Lexer lexer(source);
    if (lexer.run()) {
        std::cout << "\nLexical analyse finished succesfully\n";
        lexer.printTables();
    }

    Parser parser(lexer.getTableOfSymb());
    
    parser.run();

    return 0;
}