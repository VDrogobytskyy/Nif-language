#include "parser.h"

// Конструктор: приймає вектор карток-токенів від Lexer
Parser::Parser(const std::vector<SymbolRow>& symbols, bool enableTrace) 
    : tableOfSymb(symbols), len_tableOfSymb(symbols.size()), numRow(0), indt(0), trace(enableTrace) {}

// Збільшити відступ
std::string Parser::nextIndt() {
    indt += stepIndt;
    return std::string(indt, ' ');
}

// Зменшити відступ
std::string Parser::predIndt() {
    indt = std::max(0, indt - stepIndt);
    return std::string(indt, ' ');
}

// Прочитати поточний токен під курсором без зсуву
SymbolRow Parser::getSymb() {
    if (numRow < len_tableOfSymb) {
        return tableOfSymb[numRow];
    }
    return {numRow > 0 ? tableOfSymb.back().line : 1, "EOF", "EOF", ""};
}

// Перевірка, чи може токен бути початком виразу
bool Parser::isStartOfExpression(const SymbolRow& s) {
    if (s.token == "unsigned_int" || s.token == "unsigned_double" || 
        s.token == "bool_const" || s.token == "string_const" || 
        s.token == "identifier" || s.lexeme == "(" || 
        s.lexeme == "+" || s.lexeme == "-" || s.lexeme == "!") {
        return true;
    }
    return false;
}

void Parser::failParse(const std::string& errorType, const std::string& expected) {
    SymbolRow s = getSymb();
    std::cerr << "\n[Parser ERROR] In line " << s.line << ": " << errorType << "\n";
    std::cerr << "\"Found: '" << s.lexeme << "' (token: " << s.token << ")\n";
    if (!expected.empty()) {
        std::cerr << "\tExpected: " << expected << "\n";
    }
    std::cerr << "\tPosition in table of symbol: record № " << (numRow + 1) << " of " << len_tableOfSymb << "\n";
    exit(1);
}

// Перевірка та споживання очікуваного термінала
bool Parser::parseToken(const std::string& lexeme, const std::string& token) {
    if (numRow >= len_tableOfSymb) {
        std::cerr << "\n[Parser ERROR] Unexpected program end!\n";
        std::cerr << "\tWaited: '" << lexeme << "' (" << token << ")\n";
        exit(1001);
    }

    SymbolRow s = tableOfSymb[numRow];
    bool match = false;
    if (!lexeme.empty()) {
        match = (s.lexeme == lexeme && s.token == token);
    } else {
        match = (s.token == token);
    }

    if (match) {
        if (trace) {
            std::string in = nextIndt();
            std::cout << in << "parseToken: In line " << s.line 
                      << " found ('" << s.lexeme << "', " << s.token << ")\n";
            predIndt();
        }
        numRow++; // зсуваємо курсор на наступний токен
        return true;
    } else {
        std::string exp = (!lexeme.empty() ? ("'" + lexeme + "' (token: " + token + ")") : ("token " + token));
        failParse("token mismatch", exp);
        return false;
    }
}

// Головна функція запуску
bool Parser::run() {
    if (trace) std::cout << "\n>>> Початок синтаксичного аналізу (рекурсивний спуск) <<<\n";
    parseProgram();
    if (trace) std::cout << "\n>>> Синтаксичний аналіз завершився успішно! <<<\n";
    return true;
}

// Program = { ModuleItem }
void Parser::parseProgram() {
    if (trace) std::cout << nextIndt() << "parseProgram():" << std::endl;
    while (numRow < len_tableOfSymb) {
        parseModuleItem();
    }
    if (trace) predIndt();
}

// ModuleItem = Import | TypeAlias | Record | Fun | Var | Let | Statement
void Parser::parseModuleItem() {
    SymbolRow s = getSymb();
    if (trace) std::cout << nextIndt() << "parseModuleItem():" << std::endl;

    if (s.lexeme == "import") parseImportDeclaration();
    else if (s.lexeme == "typealias") parseTypeAliasDeclaration();
    else if (s.lexeme == "record") parseRecordDeclaration();
    else if (s.lexeme == "fun") parseFunctionDeclaration();
    else if (s.lexeme == "var") parseVariableDeclaration();
    else if (s.lexeme == "let") parseConstantDeclaration();
    else parseStatement(); // Топ-левел інструкції (script style)

    if (trace) predIndt();
}

// import Identifier
void Parser::parseImportDeclaration() {
    if (trace) std::cout << nextIndt() << "parseImportDeclaration():" << std::endl;
    parseToken("import", "keyword");
    parseToken("", "identifier");
    if (trace) predIndt();
}

// typealias Identifier = Type
void Parser::parseTypeAliasDeclaration() {
    if (trace) std::cout << nextIndt() << "parseTypeAliasDeclaration():" << std::endl;
    parseToken("typealias", "keyword");
    parseToken("", "identifier");
    parseToken("=", "assign_op");
    parseType();
    if (trace) predIndt();
}

// record Identifier { var ... }
void Parser::parseRecordDeclaration() {
    if (trace) std::cout << nextIndt() << "parseRecordDeclaration():" << std::endl;
    parseToken("record", "keyword");
    parseToken("", "identifier");
    parseToken("{", "brackets_op");
    while (getSymb().lexeme == "var") {
        parseFieldDeclaration();
    }
    parseToken("}", "brackets_op");
    if (trace) predIndt();
}

void Parser::parseFieldDeclaration() {
    if (trace) std::cout << nextIndt() << "parseFieldDeclaration():" << std::endl;
    parseToken("var", "keyword");
    parseToken("", "identifier");
    parseToken(":", "punct");
    parseType();
    if (trace) predIndt();
}

// fun Identifier( [Parameters] ) : ReturnType Block
void Parser::parseFunctionDeclaration() {
    if (trace) std::cout << nextIndt() << "parseFunctionDeclaration():" << std::endl;
    parseToken("fun", "keyword");
    parseToken("", "identifier");
    parseToken("(", "brackets_op");
    if (getSymb().lexeme != ")") {
        parseParameters();
    }
    parseToken(")", "brackets_op");
    parseToken(":", "punct");
    parseReturnType();
    parseBlock();
    if (trace) predIndt();
}

void Parser::parseParameters() {
    if (trace) std::cout << nextIndt() << "parseParameters():" << std::endl;
    parseParameter();
    while (getSymb().lexeme == ",") {
        parseToken(",", "punct");
        parseParameter();
    }
    if (trace) predIndt();
}

// [ 'ref' ] Identifier ':' Type
void Parser::parseParameter() {
    if (trace) std::cout << nextIndt() << "parseParameter():" << std::endl;
    if (getSymb().lexeme == "ref") {
        parseToken("ref", "keyword");
    }
    parseToken("", "identifier");
    parseToken(":", "punct");
    parseType();
    if (trace) predIndt();
}

// Type = ScalarType | ArrayType '[' Type ']' | UserType
void Parser::parseType() {
    if (trace) std::cout << nextIndt() << "parseType():" << std::endl;
    SymbolRow s = getSymb();
    if (s.lexeme == "[") {
        parseToken("[", "brackets_op");
        parseType();
        parseToken("]", "brackets_op");
    } else if (s.token == "type_name") {
        parseToken(s.lexeme, "type_name");
    } else if (s.token == "identifier") {
        parseToken("", "identifier"); // Користувацький тип (наприклад Circle)
    } else {
        failParse("Incorrect data type", "Int, Double, Bool, String, [Type] or user type");
    }
    if (trace) predIndt();
}

void Parser::parseReturnType() {
    if (trace) std::cout << nextIndt() << "parseReturnType():" << std::endl;
    if (getSymb().lexeme == "Void") {
        parseToken("Void", "type_name");
    } else {
        parseType();
    }
    if (trace) predIndt();
}

// var Identifier ':' Type [ '=' Expression ]
void Parser::parseVariableDeclaration() {
    if (trace) std::cout << nextIndt() << "parseVariableDeclaration():" << std::endl;
    parseToken("var", "keyword");
    parseToken("", "identifier");
    parseToken(":", "punct");
    parseType();
    if (getSymb().lexeme == "=") {
        parseToken("=", "assign_op");
        parseExpression();
    }
    if (trace) predIndt();
}

// let Identifier ':' Type '=' Expression
void Parser::parseConstantDeclaration() {
    if (trace) std::cout << nextIndt() << "parseConstantDeclaration():" << std::endl;
    parseToken("let", "keyword");
    parseToken("", "identifier");
    parseToken(":", "punct");
    parseType();
    parseToken("=", "assign_op");
    parseExpression();
    if (trace) predIndt();
}

// Block = '{' { BlockItem } '}'
void Parser::parseBlock() {
    if (trace) std::cout << nextIndt() << "parseBlock():" << std::endl;
    parseToken("{", "brackets_op");
    while (numRow < len_tableOfSymb && getSymb().lexeme != "}") {
        parseBlockItem();
    }
    parseToken("}", "brackets_op");
    if (trace) predIndt();
}

// BlockItem = Declaration | Statement
void Parser::parseBlockItem() {
    SymbolRow s = getSymb();
    if (trace) std::cout << nextIndt() << "parseBlockItem():" << std::endl;
    if (s.lexeme == "var") parseVariableDeclaration();
    else if (s.lexeme == "let") parseConstantDeclaration();
    else if (s.lexeme == "typealias") parseTypeAliasDeclaration();
    else if (s.lexeme == "record") parseRecordDeclaration();
    else parseStatement();
    if (trace) predIndt();
}

void Parser::parseStatement() {
    SymbolRow s = getSymb();
    if (trace) std::cout << nextIndt() << "parseStatement():" << std::endl;

    if (s.lexeme == "if") parseIfStatement();
    else if (s.lexeme == "while") parseWhileStatement();
    else if (s.lexeme == "for") parseForStatement();
    else if (s.lexeme == "read") parseInputStatement();
    else if (s.lexeme == "print") parseOutputStatement();
    else if (s.lexeme == "return") parseReturnStatement();
    else if (s.lexeme == "break") parseBreakStatement();
    else if (s.lexeme == "continue") parseContinueStatement();
    else if (s.lexeme == "{") parseBlock();
    else if (s.token == "identifier") parseIdentifierStatement();
    else {
        failParse("unexpected start of the instructions", "if, while, for, print, read, return, { or identifier");
    }
    if (trace) predIndt();
}

// if Expression Block [ else ( Block | IfStatement ) ]
void Parser::parseIfStatement() {
    if (trace) std::cout << nextIndt() << "parseIfStatement():" << std::endl;
    parseToken("if", "keyword");
    parseExpression();
    parseBlock();
    if (numRow < len_tableOfSymb && getSymb().lexeme == "else") {
        parseToken("else", "keyword");
        if (getSymb().lexeme == "if") {
            parseIfStatement();
        } else {
            parseBlock();
        }
    }
    if (trace) predIndt();
}

// while Expression Block
void Parser::parseWhileStatement() {
    if (trace) std::cout << nextIndt() << "parseWhileStatement():" << std::endl;
    parseToken("while", "keyword");
    parseExpression();
    parseBlock();
    if (trace) predIndt();
}

// for Identifier in Expression '..' Expression Block
void Parser::parseForStatement() {
    if (trace) std::cout << nextIndt() << "parseForStatement():" << std::endl;
    parseToken("for", "keyword");
    parseToken("", "identifier");
    parseToken("in", "keyword");
    parseExpression();
    parseToken("..", "range_op");
    parseExpression();
    parseBlock();
    if (trace) predIndt();
}

// read ( & LValue )
void Parser::parseInputStatement() {
    if (trace) std::cout << nextIndt() << "parseInputStatement():" << std::endl;
    parseToken("read", "keyword");
    parseToken("(", "brackets_op");
    parseToken("&", "ref_op");
    parseToken("", "identifier");
    while (getSymb().lexeme == "[" || getSymb().lexeme == ".") {
        if (getSymb().lexeme == "[") {
            parseToken("[", "brackets_op");
            parseExpression();
            parseToken("]", "brackets_op");
        } else {
            parseToken(".", "punct");
            parseToken("", "identifier");
        }
    }
    parseToken(")", "brackets_op");
    if (trace) predIndt();
}

// print ( [ Arguments ] )
void Parser::parseOutputStatement() {
    if (trace) std::cout << nextIndt() << "parseOutputStatement():" << std::endl;
    parseToken("print", "keyword");
    parseToken("(", "brackets_op");
    if (getSymb().lexeme != ")") {
        parseArguments();
    }
    parseToken(")", "brackets_op");
    if (trace) predIndt();
}

// return [ Expression ]
void Parser::parseReturnStatement() {
    if (trace) std::cout << nextIndt() << "parseReturnStatement():" << std::endl;
    parseToken("return", "keyword");
    if (isStartOfExpression(getSymb())) {
        parseExpression();
    }
    if (trace) predIndt();
}

void Parser::parseBreakStatement() {
    if (trace) std::cout << nextIndt() << "parseBreakStatement():" << std::endl;
    parseToken("break", "keyword");
    if (trace) predIndt();
}

void Parser::parseContinueStatement() {
    if (trace) std::cout << nextIndt() << "parseContinueStatement():" << std::endl;
    parseToken("continue", "keyword");
    if (trace) predIndt();
}

// LValue '=' Expression  АБО  FunctionCall
void Parser::parseIdentifierStatement() {
    if (trace) std::cout << nextIndt() << "parseIdentifierStatement():" << std::endl;
    parseToken("", "identifier");

    // Обробка суфіксів доступу до масиву [expr] або поля .field
    while (getSymb().lexeme == "[" || getSymb().lexeme == ".") {
        if (getSymb().lexeme == "[") {
            parseToken("[", "brackets_op");
            parseExpression();
            parseToken("]", "brackets_op");
        } else {
            parseToken(".", "punct");
            parseToken("", "identifier");
        }
    }

    if (getSymb().lexeme == "=") {
        parseToken("=", "assign_op");
        parseExpression();
    } else if (getSymb().lexeme == "(") {
        parseToken("(", "brackets_op");
        if (getSymb().lexeme != ")") {
            parseArguments();
        }
        parseToken(")", "brackets_op");
    } else {
        failParse("unexpected token after identifier", "'=' (appropriation) або '(' (function call)");
    }
    if (trace) predIndt();
}


// Рівень 1: Expression -> LogicalOrExpr
void Parser::parseExpression() {
    if (trace) std::cout << nextIndt() << "parseExpression():" << std::endl;
    parseLogicalOrExpr();
    if (trace) predIndt();
}

// Рівень 2: LogicalOrExpr -> LogicalAndExpr { '||' LogicalAndExpr }
void Parser::parseLogicalOrExpr() {
    if (trace) std::cout << nextIndt() << "parseLogicalOrExpr():" << std::endl;
    parseLogicalAndExpr();
    while (getSymb().lexeme == "||") {
        parseToken("||", "logical_op");
        parseLogicalAndExpr();
    }
    if (trace) predIndt();
}

// Рівень 3: LogicalAndExpr -> EqualityExpr { '&&' EqualityExpr }
void Parser::parseLogicalAndExpr() {
    if (trace) std::cout << nextIndt() << "parseLogicalAndExpr():" << std::endl;
    parseEqualityExpr();
    while (getSymb().lexeme == "&&") {
        parseToken("&&", "logical_op");
        parseEqualityExpr();
    }
    if (trace) predIndt();
}

// Рівень 4: EqualityExpr -> RelationalExpr { ('==' | '!=') RelationalExpr }
void Parser::parseEqualityExpr() {
    if (trace) std::cout << nextIndt() << "parseEqualityExpr():" << std::endl;
    parseRelationalExpr();
    while (getSymb().lexeme == "==" || getSymb().lexeme == "!=") {
        parseToken(getSymb().lexeme, "rel_op");
        parseRelationalExpr();
    }
    if (trace) predIndt();
}

// Рівень 5: RelationalExpr -> AddExpr { ('<' | '<=' | '>' | '>=') AddExpr }
void Parser::parseRelationalExpr() {
    if (trace) std::cout << nextIndt() << "parseRelationalExpr():" << std::endl;
    parseAddExpr();
    while (getSymb().lexeme == "<" || getSymb().lexeme == "<=" || 
           getSymb().lexeme == ">" || getSymb().lexeme == ">=") {
        parseToken(getSymb().lexeme, "rel_op");
        parseAddExpr();
    }
    if (trace) predIndt();
}

// Рівень 6: AddExpr -> MultExpr { ('+' | '-') MultExpr }
void Parser::parseAddExpr() {
    if (trace) std::cout << nextIndt() << "parseAddExpr():" << std::endl;
    parseMultExpr();
    while (getSymb().lexeme == "+" || getSymb().lexeme == "-") {
        parseToken(getSymb().lexeme, "add_op");
        parseMultExpr();
    }
    if (trace) predIndt();
}

// Рівень 7: MultExpr -> PowerExpr { ('*' | '/' | '%') PowerExpr }
void Parser::parseMultExpr() {
    if (trace) std::cout << nextIndt() << "parseMultExpr():" << std::endl;
    parsePowerExpr();
    while (getSymb().lexeme == "*" || getSymb().lexeme == "/" || getSymb().lexeme == "%") {
        parseToken(getSymb().lexeme, "mult_op");
        parsePowerExpr();
    }
    if (trace) predIndt();
}

// Рівень 8: PowerExpr -> UnaryExpr [ '**' PowerExpr ] (правоасоціативне!)
void Parser::parsePowerExpr() {
    if (trace) std::cout << nextIndt() << "parsePowerExpr():" << std::endl;
    parseUnaryExpr();
    if (getSymb().lexeme == "**") {
        parseToken("**", "exp_op");
        parsePowerExpr();
    }
    if (trace) predIndt();
}

// Рівень 9: UnaryExpr -> [ '+' | '-' | '!' ] PostfixExpr
void Parser::parseUnaryExpr() {
    if (trace) std::cout << nextIndt() << "parseUnaryExpr():" << std::endl;
    SymbolRow s = getSymb();
    if (s.lexeme == "+" || s.lexeme == "-") {
        parseToken(s.lexeme, "add_op");
        parseUnaryExpr();
    } else if (s.lexeme == "!") {
        parseToken("!", "logical_op");
        parseUnaryExpr();
    } else {
        parsePostfixExpr();
    }
    if (trace) predIndt();
}

// Рівень 10: PostfixExpr -> PrimaryExpr { FunctionCall | ArrayAccess | FieldAccess }
void Parser::parsePostfixExpr() {
    if (trace) std::cout << nextIndt() << "parsePostfixExpr():" << std::endl;
    parsePrimaryExpr();
    while (true) {
        if (getSymb().lexeme == "(") {
            parseToken("(", "brackets_op");
            if (getSymb().lexeme != ")") {
                parseArguments();
            }
            parseToken(")", "brackets_op");
        } else if (getSymb().lexeme == "[") {
            parseToken("[", "brackets_op");
            parseExpression();
            parseToken("]", "brackets_op");
        } else if (getSymb().lexeme == ".") {
            parseToken(".", "punct");
            parseToken("", "identifier");
        } else {
            break;
        }
    }
    if (trace) predIndt();
}

// Рівень 11: PrimaryExpr -> Const | Identifier | '(' Expression ')'
void Parser::parsePrimaryExpr() {
    SymbolRow s = getSymb();
    if (trace) std::cout << nextIndt() << "parsePrimaryExpr():" << std::endl;

    if (s.token == "unsigned_int" || s.token == "unsigned_double" || 
        s.token == "bool_const" || s.token == "string_const") {
        parseToken(s.lexeme, s.token);
    } else if (s.token == "identifier") {
        parseToken("", "identifier");
    } else if (s.lexeme == "(") {
        parseToken("(", "brackets_op");
        parseExpression();
        parseToken(")", "brackets_op");
    } else {
        failParse("unexpected element in an expression", "constant, identifier or '('");
    }
    if (trace) predIndt();
}

// Arguments = Argument { ',' Argument }
void Parser::parseArguments() {
    if (trace) std::cout << nextIndt() << "parseArguments():" << std::endl;
    parseArgument();
    while (getSymb().lexeme == ",") {
        parseToken(",", "punct");
        parseArgument();
    }
    if (trace) predIndt();
}

// Argument = [ '&' ] Expression
void Parser::parseArgument() {
    if (trace) std::cout << nextIndt() << "parseArgument():" << std::endl;
    if (getSymb().lexeme == "&") {
        parseToken("&", "ref_op");
    }
    parseExpression();
    if (trace) predIndt();
}