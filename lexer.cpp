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
};

Lexer::Lexer(const std::string &source) {
    sourceCode = source;
    lenCode = source.length();
    numChar = -1;
    numLine = 1;
    state = 0;
    currentChar = '\0';
    lexeme.clear();

    // 27 фінальних станів
    F = {
        2, 5, 6, 11, 13, 14, 15, 17, 18, 20, 22, 24, 25, 27, 
        29, 30, 32, 34, 36, 37, 40, 41, 42, 
        101, 103, 104, 105
    };

    // 11 станів із зірочкою (стан 40 більше НЕ потребує повернення символу)
    Fstar = {2, 5, 6, 13, 18, 25, 29, 32, 36, 41, 42};

    Ferror = {101, 103, 104, 105};

    stf = {
        // Пробільні символи та кінець рядка
        {{0, "ws"}, 0},
        {{0, "nl"}, 27},

        // Ідентифікатори та ключові слова
        {{0, "Letter"}, 1},
        {{0, "_"}, 1},
        {{1, "Letter"}, 1},
        {{1, "Digit"}, 1},
        {{1, "_"}, 1},
        {{1, "other"}, 2},

        // Числові константи (цілі та дійсні)
        {{0, "Digit"}, 3},
        {{3, "Digit"}, 3},
        {{3, "_"}, 3},
        {{3, "other"}, 6},
        {{3, "."}, 4},
        {{4, "Digit"}, 26},
        {{26, "Digit"}, 26},
        {{26, "_"}, 26},
        {{26, "other"}, 5},
        {{4, "other"}, 105},

        // Оператор діапазону (..) та одиночна крапка (.) — ЧИСТА АВТОНОМНА ГІЛКА
        {{0, "."}, 39},
        {{39, "."}, 40},
        {{39, "other"}, 41},

        // Рядкові літерали
        {{0, "\""}, 10},
        {{10, "other"}, 10},
        {{10, "\""}, 11},
        {{10, "nl"}, 104},

        // Ділення та однорядкові коментарі
        {{0, "/"}, 31},
        {{31, "/"}, 33},
        {{33, "other"}, 33},
        {{33, "nl"}, 34},
        {{31, "other"}, 32},

        // Оператор присвоювання та рівності
        {{0, "="}, 12},
        {{12, "="}, 14},
        {{12, "other"}, 13},

        // Множення та піднесення до степеня
        {{0, "*"}, 16},
        {{16, "*"}, 17},
        {{16, "other"}, 18},

        // Оператор взяття посилання та логічне AND
        {{0, "&"}, 19},
        {{19, "&"}, 20},
        {{19, "other"}, 42},

        // Логічне OR
        {{0, "|"}, 21},
        {{21, "|"}, 22},
        {{21, "other"}, 103},

        // Заперечення та нерівність
        {{0, "!"}, 23},
        {{23, "="}, 24},
        {{23, "other"}, 25},

        // Оператори порівняння
        {{0, "<"}, 28},
        {{28, "="}, 30},
        {{28, "other"}, 29},
        {{0, ">"}, 35},
        {{35, "="}, 37},
        {{35, "other"}, 36},

        // Односимвольні розділювачі та операції
        {{0, "+"}, 15},
        {{0, "-"}, 15},
        {{0, "%"}, 15},
        {{0, "("}, 15},
        {{0, ")"}, 15},
        {{0, "{"}, 15},
        {{0, "}"}, 15},
        {{0, "["}, 15},
        {{0, "]"}, 15},
        {{0, ":"}, 15},
        {{0, ","}, 15},

        {{0, "other"}, 101}
    };

    tokenTable = {
        {"var", "keyword"}, {"let", "keyword"}, {"fun", "keyword"},
        {"if", "keyword"}, {"else", "keyword"}, {"while", "keyword"},
        {"for", "keyword"}, {"in", "keyword"}, {"return", "keyword"},
        {"break", "keyword"}, {"continue", "keyword"}, {"import", "keyword"},
        {"record", "keyword"}, {"typealias", "keyword"}, {"ref", "keyword"},
        {"print", "keyword"}, {"read", "keyword"},
        {"Int", "type_name"}, {"Double", "type_name"}, 
        {"Bool", "type_name"}, {"String", "type_name"}, {"Void", "type_name"},
        {"true", "bool_const"}, {"false", "bool_const"},
        {"+", "add_op"}, {"-", "add_op"},
        {"*", "mult_op"}, {"/", "mult_op"}, {"%", "mult_op"},
        {"**", "exp_op"},
        {"=", "assign_op"},
        {"==", "rel_op"}, {"!=", "rel_op"},
        {"<", "rel_op"}, {"<=", "rel_op"},
        {">", "rel_op"}, {">=", "rel_op"},
        {"&&", "logical_op"}, {"||", "logical_op"}, {"!", "logical_op"},
        {"..", "range_op"}, {"&", "ref_op"},
        {"(", "brackets_op"}, {")", "brackets_op"},
        {"{", "brackets_op"}, {"}", "brackets_op"},
        {"[", "brackets_op"}, {"]", "brackets_op"},
        {":", "punct"}, {",", "punct"}, {".", "punct"}
    };
}

char Lexer::nextChar() {
    if (sourceCode.length() == 0) return '\0';
    numChar++;
    if (numChar < lenCode) return sourceCode[numChar];
    return '\0';
}

void Lexer::putCharBack() {
    if (numChar >= 0) numChar--;
}

char Lexer::peekChar() {
    if (numChar + 1 < lenCode) return sourceCode[numChar + 1];
    return '\0';
}

std::string Lexer::classOfChar(char c) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        return "Letter";
    }
    if (c >= '0' && c <= '9') {
        return "Digit";
    }

    const static std::string spec = "_.\"\\=+-*/%<>!&|(){}[]:,";
    if (spec.find(c) != std::string::npos) {
        return std::string(1, c);
    }

    switch (c) {
        case ' ':
        case '\t':
        case '\f':
            return "ws";
        case '\n':
        case '\r':
            return "nl";
        default:
            return "other";
    }
}

int Lexer::nextState(int curState, const std::string& cls) {
    auto it = stf.find({curState, cls});
    if (it != stf.end()) return it->second;

    it = stf.find({curState, "other"});
    if (it != stf.end()) return it->second;

    return 101;
}

void Lexer::fail(int errorState) {
    std::cerr << "\n[Lexer ERROR] У рядку " << numLine << ": ";
    if (errorState == 101) {
        std::cerr << "неочікуваний символ '" << currentChar << "'\n";
    } else if (errorState == 103) {
        std::cerr << "очікувався символ '|', знайдено '" << currentChar << "'\n";
    } else if (errorState == 104) {
        std::cerr << "незакритий рядковий літерал (рядок перервано)\n";
    } else if (errorState == 105) {
        std::cerr << "некоректний запис дійсного числа біля '" << lexeme << "'\n";
    } else {
        std::cerr << "невідома лексична помилка (стан " << errorState << ")\n";
    }
    exit(errorState);
}

void Lexer::processing() {
    // 1. Новий рядок (стан 27)
    if (state == 27) {
        numLine++;
        state = 0;
        lexeme.clear();
        return;
    }
    // 2. Однорядковий коментар (стан 34)
    if (state == 34) {
        numLine++;
        state = 0;
        lexeme.clear();
        return;
    }
    // 3. Помилкові стани
    if (Ferror.count(state)) {
        fail(state);
    }
    // 4. Ідентифікатори та ключові слова (стан 2*)
    if (state == 2) {
        putCharBack();
        auto it = tokenTable.find(lexeme);
        if (it != tokenTable.end()) {
            std::string tok = it->second;
            if (tok == "bool_const") {
                if (tableOfConst.find(lexeme) == tableOfConst.end()) {
                    int idx = tableOfConst.size() + 1;
                    tableOfConst[lexeme] = {"Bool", idx};
                }
                int idx = tableOfConst[lexeme].second;
                tableOfSymb.push_back({numLine, lexeme, tok, std::to_string(idx)});
            } else {
                tableOfSymb.push_back({numLine, lexeme, tok, ""});
            }
        } else {
            if (tableOfId.find(lexeme) == tableOfId.end()) {
                int idx = tableOfId.size() + 1;
                tableOfId[lexeme] = idx;
            }
            int idx = tableOfId[lexeme];
            tableOfSymb.push_back({numLine, lexeme, "identifier", std::to_string(idx)});
        }
        state = 0;
        lexeme.clear();
        return;
    }
    // 5. Ціле число (стан 6*)
    if (state == 6) {
        putCharBack();
        std::string cleanVal = lexeme;
        cleanVal.erase(std::remove(cleanVal.begin(), cleanVal.end(), '_'), cleanVal.end());
        if (tableOfConst.find(cleanVal) == tableOfConst.end()) {
            int idx = tableOfConst.size() + 1;
            tableOfConst[cleanVal] = {"Int", idx};
        }
        int idx = tableOfConst[cleanVal].second;
        tableOfSymb.push_back({numLine, lexeme, "unsigned_int", std::to_string(idx)});
        state = 0;
        lexeme.clear();
        return;
    }
    // 6. Дійсне число (стан 5*)
    if (state == 5) {
        putCharBack();
        std::string cleanVal = lexeme;
        cleanVal.erase(std::remove(cleanVal.begin(), cleanVal.end(), '_'), cleanVal.end());
        if (tableOfConst.find(cleanVal) == tableOfConst.end()) {
            int idx = tableOfConst.size() + 1;
            tableOfConst[cleanVal] = {"Double", idx};
        }
        int idx = tableOfConst[cleanVal].second;
        tableOfSymb.push_back({numLine, lexeme, "unsigned_double", std::to_string(idx)});
        state = 0;
        lexeme.clear();
        return;
    }
    // 7. Рядковий літерал (стан 11)
    if (state == 11) {
        if (tableOfConst.find(lexeme) == tableOfConst.end()) {
            int idx = tableOfConst.size() + 1;
            tableOfConst[lexeme] = {"String", idx};
        }
        int idx = tableOfConst[lexeme].second;
        tableOfSymb.push_back({numLine, lexeme, "string_const", std::to_string(idx)});
        state = 0;
        lexeme.clear();
        return;
    }
    // 8. Оператор діапазону '..' (стан 40) — чистий фінальний стан без зірочки
    if (state == 40) {
        tableOfSymb.push_back({numLine, "..", "range_op", ""});
        state = 0;
        lexeme.clear();
        return;
    }
    // 9. Одиночна крапка '.' (стан 41*)
    if (state == 41) {
        putCharBack();
        tableOfSymb.push_back({numLine, ".", "punct", ""});
        state = 0;
        lexeme.clear();
        return;
    }
    // 10. Одиночний '&' (стан 42*)
    if (state == 42) {
        putCharBack();
        tableOfSymb.push_back({numLine, "&", "ref_op", ""});
        state = 0;
        lexeme.clear();
        return;
    }
    // 11. Решта операторів та роздільників
    if (Fstar.count(state)) {
        putCharBack();
    }
    std::string tok = "unknown";
    auto it = tokenTable.find(lexeme);
    if (it != tokenTable.end()) {
        tok = it->second;
    }
    tableOfSymb.push_back({numLine, lexeme, tok, ""});
    state = 0;
    lexeme.clear();
}

bool Lexer::run() {
    lenCode = sourceCode.length();
    while (numChar < lenCode - 1) {
        currentChar = nextChar();
        std::string cls = classOfChar(currentChar);

        // Dragon Book (передбачуваний розбір для розділення чисел та '..')
        // Якщо автомат у стані 3 (читання цілого числа) і зустрічає '.', перевіряємо чи наступний символ також '.'
        if (state == 3 && currentChar == '.' && peekChar() == '.') {
            // Наступні два символи є оператором діапазону '..'. Поточна крапка НЕ належить числу!
            // Завершуємо ціле число переходом у стан 6* (other)
            cls = "other";
        }

        state = nextState(state, cls);

        if (F.count(state)) {
            // Якщо стан фінальний і НЕ є зірочкою — поточний символ належить лексемі
            if (!Fstar.count(state) && state != 27 && state != 34 && !Ferror.count(state)) {
                lexeme += currentChar;
            }
            processing();
        } else if (state == 0) {
            lexeme.clear();
        } else {
            lexeme += currentChar;
        }
    }

    if (state != 0 && state != 27) {
        state = nextState(state, "other");
        if (F.count(state)) {
            processing();
        } else {
            fail(104);
        }
    }
    return true;
}

void Lexer::printTables() const {
    std::cout << "\n===========================================================================\n";
    std::cout << "                      ТАБЛИЦЯ РОЗБОРУ (tableOfSymb)                        \n";
    std::cout << "===========================================================================\n";
    std::cout << std::left << std::setw(6) << "№"
              << std::setw(8) << "Рядок"
              << std::setw(22) << "Лексема"
              << std::setw(20) << "Токен"
              << std::setw(12) << "Індекс (Id/Const)" << "\n";
    std::cout << "---------------------------------------------------------------------------\n";
    int rowNum = 1;
    for (const auto& row : tableOfSymb) {
        std::cout << std::left << std::setw(6) << rowNum++
                  << std::setw(8) << row.line
                  << std::setw(22) << row.lexeme
                  << std::setw(20) << row.token
                  << std::setw(12) << row.index << "\n";
    }
    std::cout << "\n=======================================================\n";
    std::cout << "          ТАБЛИЦЯ ІДЕНТИФІКАТОРІВ (tableOfId)          \n";
    std::cout << "=======================================================\n";
    std::cout << std::left << std::setw(10) << "Індекс"
              << std::setw(25) << "Ідентифікатор" << "\n";
    std::cout << "-------------------------------------------------------\n";
    for (const auto& pair : tableOfId) {
        std::cout << std::left << std::setw(10) << pair.second
                  << std::setw(25) << pair.first << "\n";
    }
    std::cout << "\n=======================================================\n";
    std::cout << "             ТАБЛИЦЯ КОНСТАНТ (tableOfConst)           \n";
    std::cout << "=======================================================\n";
    std::cout << std::left << std::setw(10) << "Індекс"
              << std::setw(15) << "Тип"
              << std::setw(25) << "Значення" << "\n";
    std::cout << "-------------------------------------------------------\n";
    for (const auto& pair : tableOfConst) {
        std::cout << std::left << std::setw(10) << pair.second.second
                  << std::setw(15) << pair.second.first
                  << std::setw(25) << pair.first << "\n";
    }
    std::cout << "=======================================================\n";
}

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
        std::cout << "\n>>> Лексичний аналіз завершено успішно! <<<\n";
        lexer.printTables();
    }
    return 0;
}