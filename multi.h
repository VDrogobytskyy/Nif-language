#pragma once
#include <string>

struct SymbolRow {
    int line;            // номер рядка у файлі
    std::string lexeme;  // текст лексеми "x", "10", "var", "=="
    std::string token;   // тип токена
    std::string index;   // індекс у таблиці ідентифікаторів/констант (або пустий рядок)
};