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
    if (trace) std::cout << "\n>>> Початок семантичного аналізу <<<\n";
    parseProgram();
    if (trace) std::cout << "\n>>> Семантичний аналіз завершився успішно! <<<\n";
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
    SymbolRow aliasId = getSymb();
    parseToken("", "identifier");
    parseToken("=", "assign_op");
    std::string realType = parseType();
    
    sem.declareTypealias(aliasId.lexeme, realType);
    if (trace) predIndt();
}

// record Identifier { var ... }
void Parser::parseRecordDeclaration() {
    if (trace) std::cout << nextIndt() << "parseRecordDeclaration():" << std::endl;
    parseToken("record", "keyword");
    SymbolRow recId = getSymb();

    parseToken("", "identifier");
    parseToken("{", "brackets_op");

    std::unordered_map<std::string, std::string> fields;
    while (getSymb().lexeme == "var") {
        parseToken("var", "keyword");
        SymbolRow fId = getSymb();
        parseToken("", "identifier");
        parseToken(":", "punct");
        std::string fType = parseType();
        fields[fId.lexeme] = fType;
    }

    parseToken("}", "brackets_op");

    sem.declareRecord(recId.lexeme, fields);

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
    SymbolRow fn = getSymb();

    parseToken("", "identifier");
    parseToken("(", "brackets_op");

    pendingParams.clear();

    if (getSymb().lexeme != ")") {
        parseParameters();
    }
    parseToken(")", "brackets_op");
    parseToken(":", "punct");
    std::string return_type = parseReturnType();

    sem.declareFunc(fn.lexeme, return_type, pendingParams, fn.line);

    hasReturn = false;
    parseBlock();

    if (return_type != "Void" && !hasReturn) {
        std::cerr << "[Semantic Error]\nIn line: " << fn.line 
                  << ": MissingReturnStatement - function '" << fn.lexeme 
                  << "' with return type '" << return_type << "' must contain a return statement\n" << std::endl;
        exit(1);
    }

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
    bool isRef = false;

    if (getSymb().lexeme == "ref") {
        isRef = true;
        parseToken("ref", "keyword");
    }
    SymbolRow id = getSymb();
    parseToken("", "identifier");
    parseToken(":", "punct");
    std::string type = parseType();
    
    pendingParams.push_back({id.lexeme, type, isRef});;

    if (trace) predIndt();
}

// Type = ScalarType | ArrayType '[' Type ']' | UserType
std::string Parser::parseType() {
    if (trace) std::cout << nextIndt() << "parseType():" << std::endl;
    SymbolRow s = getSymb();

    std::string returned_type_value = "";

    if (s.lexeme == "[") {
        parseToken("[", "brackets_op");
        std::string innerType = parseType();
        parseToken("]", "brackets_op");
        returned_type_value = "[" + innerType + "]"; 
    } else if (s.token == "type_name") {
        returned_type_value = s.lexeme;
        parseToken(s.lexeme, "type_name");
    } else if (s.token == "identifier") {
        returned_type_value = s.lexeme;
        parseToken("", "identifier");
        
        if (sem.hasTypealias(returned_type_value)) {
            returned_type_value = sem.getTypealias(returned_type_value);
        }
    } else {
        failParse("Incorrect data type", "Int, Double, Bool, String, [Type] or user type");
    }
    if (trace) predIndt();

    return returned_type_value;
}

std::string Parser::parseReturnType() {
    if (trace) std::cout << nextIndt() << "parseReturnType():" << std::endl;
    std::string type = "Void";

    if (getSymb().lexeme == "Void") {
        parseToken("Void", "type_name");
        type = "Void";
    } else {
        type = parseType();
    }
    if (trace) predIndt();

    return type;
}

// var Identifier ':' Type [ '=' Expression ]
void Parser::parseVariableDeclaration() {
    if (trace) std::cout << nextIndt() << "parseVariableDeclaration():" << std::endl;
    parseToken("var", "keyword");

    SymbolRow id = getSymb();
    parseToken("", "identifier");
    parseToken(":", "punct");

    std::string delcared_type = parseType();

    sem.declareVar(id.lexeme, delcared_type, false, id.line);

    if (getSymb().lexeme == "=") {
        parseToken("=", "assign_op");
        std::string expr_Type = parseExpression();
        sem.checkAssignType(id.lexeme, expr_Type, id.line);
    }
    if (trace) predIndt();
}

// let Identifier ':' Type '=' Expression
void Parser::parseConstantDeclaration() {
    if (trace) std::cout << nextIndt() << "parseConstantDeclaration():" << std::endl;
    parseToken("let", "keyword");

    SymbolRow id = getSymb();
    parseToken("", "identifier");
    parseToken(":", "punct");

    std::string delcared_type = parseType();
    sem.declareVar(id.lexeme, delcared_type, true, id.line);
    parseToken("=", "assign_op");
    std::string expr_Type = parseExpression();
    sem.checkAssignType(id.lexeme, expr_Type, id.line);
    if (trace) predIndt();
}

// Block = '{' { BlockItem } '}'
void Parser::parseBlock() {
    if (trace) std::cout << nextIndt() << "parseBlock():" << std::endl;
    sem.enter_scope();

    for (const auto& p : pendingParams) {
        sem.declareVar(p.name, p.type, false, getSymb().line);
        VarSymb* v = sem.findVar(p.name);
        if (v) v->isAssigned = true;
    }

    pendingParams.clear();

    parseToken("{", "brackets_op");
    while (numRow < len_tableOfSymb && getSymb().lexeme != "}") {
        parseBlockItem();
    }
    parseToken("}", "brackets_op");
    sem.exit_scope();
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

    SymbolRow s = getSymb();
    parseToken("if", "keyword");

    std::string type = parseExpression();
    if(type != "Bool"){
        std::cerr << "[Semantic Error]\nIn line: " << s.line << ": NonBooleanCondition - require Bool type, got " << type << "\n" << std::endl;
        exit(1);
    }

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

    SymbolRow s = getSymb();
    parseToken("while", "keyword");
    sem.enter_loop();

    std::string type = parseExpression();
    if(type != "Bool"){
        std::cerr << "[Semantic Error]\nIn line: " << s.line << ": NonBooleanCondition - require Bool type, got " << type << "\n" << std::endl;
        exit(1);
    }

    parseBlock();
    sem.exit_loop();
    if (trace) predIndt();
}

// for Identifier in Expression '..' Expression Block
void Parser::parseForStatement() {
    if (trace) std::cout << nextIndt() << "parseForStatement():" << std::endl;
    parseToken("for", "keyword");
    sem.enter_loop();
    SymbolRow iter = getSymb();
    parseToken("", "identifier");
    parseToken("in", "keyword");
    parseExpression();
    parseToken("..", "range_op");
    parseExpression();

    pendingParams.push_back({iter.lexeme, "Int", false});
    parseBlock();
    sem.exit_loop();
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
    hasReturn = true;
    parseToken("return", "keyword");
    if (isStartOfExpression(getSymb())) {
        parseExpression();
    }
    if (trace) predIndt();
}

void Parser::parseBreakStatement() {
    if (trace) std::cout << nextIndt() << "parseBreakStatement():" << std::endl;
    SymbolRow s = getSymb();
    sem.loop_control(s.lexeme, s.line);
    parseToken("break", "keyword");
    if (trace) predIndt();
}

void Parser::parseContinueStatement() {
    if (trace) std::cout << nextIndt() << "parseContinueStatement():" << std::endl;
    SymbolRow s = getSymb();
    sem.loop_control(s.lexeme, s.line);
    parseToken("continue", "keyword");
    if (trace) predIndt();
}

// LValue '=' Expression  АБО  FunctionCall
void Parser::parseIdentifierStatement() {
    if (trace) std::cout << nextIndt() << "parseIdentifierStatement():" << std::endl;
    
    SymbolRow id = getSymb();

    VarSymb* varInfo = sem.findVar(id.lexeme);
    std::string currentType = varInfo ? varInfo->VarType : "";

    parseToken("", "identifier");

    // Обробка суфіксів доступу до масиву [expr] або поля .field
    while (getSymb().lexeme == "[" || getSymb().lexeme == ".") {
        if (getSymb().lexeme == "[") {
            SymbolRow bracket = getSymb();
            parseToken("[", "brackets_op");
            std::string type = parseExpression();
            if(type != "Int"){
                std::cerr << "[Semantic Error]\nIn line: " << bracket.line << ": InvalidIndexType - must be Int, got: " << type << "\n" << std::endl;
                exit(1);
            }
            parseToken("]", "brackets_op");

            if (currentType.size() > 2 && currentType.front() == '[' && currentType.back() == ']') {
                currentType = currentType.substr(1, currentType.size() - 2);
            }
        } else {
            SymbolRow dot = getSymb();
            parseToken(".", "punct");
            SymbolRow field = getSymb();
            parseToken("", "identifier");

            if (sem.hasRecord(currentType)) {
                if (!sem.hasRecordField(currentType, field.lexeme)) {
                    std::cerr << "[Semantic Error]\nIn line: " << field.line 
                              << ": NoSuchField - record '" << currentType 
                              << "' does not contain field '" << field.lexeme << "'\n" << std::endl;
                    exit(1);
                }
                currentType = sem.getRecordFieldType(currentType, field.lexeme);
            }
        }
    }

    if (getSymb().lexeme == "=") {
        parseToken("=", "assign_op");
        std::string expr_Type = parseExpression();
        if (currentType != "" && currentType != (varInfo ? varInfo->VarType : "")) {
            bool all_good = (currentType == expr_Type) || (currentType == "Double" && expr_Type == "Int");
            if (!all_good) {
                std::cerr << "[Semantic Error]\nIn line: " << id.line << ": TypeMismatch - cannot assign: " << expr_Type 
                          << " to element of type: " << currentType << "\n" << std::endl; 
                exit(1);
            }
            if (varInfo) varInfo->isAssigned = true;
        } else {
            sem.checkAssignType(id.lexeme, expr_Type, id.line);
        }
    } else if (getSymb().lexeme == "(") {
        SymbolRow paren = getSymb();

        parseToken("(", "brackets_op");
        int argCount = 0;
        if (getSymb().lexeme != ")") {
            argCount = parseArguments();
        }
        parseToken(")", "brackets_op");

        FuncSymb* fn = sem.findFunc(id.lexeme);
        if (fn) {
            if (argCount != fn->params.size()) {
                std::cerr << "[Semantic Error]\nIn line: " << paren.line 
                          << ": ArgumentCountMismatch - function '" << fn->name 
                          << "' expects " << fn->params.size() 
                          << " arguments, got " << argCount << "\n" << std::endl;
                exit(1);
            }
        }

    } else {
        failParse("unexpected token after identifier", "'=' (appropriation) або '(' (function call)");
    }
    if (trace) predIndt();
}

// Рівень 1: Expression -> LogicalOrExpr
std::string Parser::parseExpression() {
    if (trace) std::cout << nextIndt() << "parseExpression():" << std::endl;
    std::string type = parseLogicalOrExpr();
    if (trace) predIndt();

    return type;
}

// Рівень 2: LogicalOrExpr -> LogicalAndExpr { '||' LogicalAndExpr }
std::string Parser::parseLogicalOrExpr() {
    if (trace) std::cout << nextIndt() << "parseLogicalOrExpr():" << std::endl;
    std::string type = parseLogicalAndExpr();
    while (getSymb().lexeme == "||") {
        parseToken("||", "logical_op");
        parseLogicalAndExpr();
    }
    if (trace) predIndt();

    return type;
}

// Рівень 3: LogicalAndExpr -> EqualityExpr { '&&' EqualityExpr }
std::string Parser::parseLogicalAndExpr() {
    if (trace) std::cout << nextIndt() << "parseLogicalAndExpr():" << std::endl;
   std::string type = parseEqualityExpr();
    while (getSymb().lexeme == "&&") {
        parseToken("&&", "logical_op");
        parseEqualityExpr();
    }
    if (trace) predIndt();

    return type;
}

// Рівень 4: EqualityExpr -> RelationalExpr { ('==' | '!=') RelationalExpr }
std::string Parser::parseEqualityExpr() {
    if (trace) std::cout << nextIndt() << "parseEqualityExpr():" << std::endl;
    std::string type = parseRelationalExpr();
    while (getSymb().lexeme == "==" || getSymb().lexeme == "!=") {
        parseToken(getSymb().lexeme, "rel_op");
        parseRelationalExpr();
    }
    if (trace) predIndt();

    return type;
}

// Рівень 5: RelationalExpr -> AddExpr { ('<' | '<=' | '>' | '>=') AddExpr }
std::string Parser::parseRelationalExpr() {
    if (trace) std::cout << nextIndt() << "parseRelationalExpr():" << std::endl;
    std::string type = parseAddExpr();
    while (getSymb().lexeme == "<" || getSymb().lexeme == "<=" || 
           getSymb().lexeme == ">" || getSymb().lexeme == ">=") {
        parseToken(getSymb().lexeme, "rel_op");
        parseAddExpr();
    }
    if (trace) predIndt();

    return type;
}

// Рівень 6: AddExpr -> MultExpr { ('+' | '-') MultExpr }
std::string Parser::parseAddExpr() {
    if (trace) std::cout << nextIndt() << "parseAddExpr():" << std::endl;
    std::string type = parseMultExpr();
    while (getSymb().lexeme == "+" || getSymb().lexeme == "-") {
        parseToken(getSymb().lexeme, "add_op");
        parseMultExpr();
    }
    if (trace) predIndt();

    return type;
}

// Рівень 7: MultExpr -> PowerExpr { ('*' | '/' | '%') PowerExpr }
std::string Parser::parseMultExpr() {
    if (trace) std::cout << nextIndt() << "parseMultExpr():" << std::endl;
    std::string type = parsePowerExpr();
    while (getSymb().lexeme == "*" || getSymb().lexeme == "/" || getSymb().lexeme == "%") {
        parseToken(getSymb().lexeme, "mult_op");
        parsePowerExpr();
    }
    if (trace) predIndt();

    return type;
}

// Рівень 8: PowerExpr -> UnaryExpr [ '**' PowerExpr ] (правоасоціативне!)
std::string Parser::parsePowerExpr() {
    if (trace) std::cout << nextIndt() << "parsePowerExpr():" << std::endl;
    std::string type = parseUnaryExpr();
    if (getSymb().lexeme == "**") {
        parseToken("**", "exp_op");
        parsePowerExpr();
    }
    if (trace) predIndt();

    return type;
}

// Рівень 9: UnaryExpr -> [ '+' | '-' | '!' ] PostfixExpr
std::string Parser::parseUnaryExpr() {
    if (trace) std::cout << nextIndt() << "parseUnaryExpr():" << std::endl;
    SymbolRow s = getSymb();
    std::string type = "";

    if (s.lexeme == "+" || s.lexeme == "-") {
        parseToken(s.lexeme, "add_op");
        type = parseUnaryExpr();
    } else if (s.lexeme == "!") {
        parseToken("!", "logical_op");
        type = parseUnaryExpr();
    } else {
        type = parsePostfixExpr();
    }
    if (trace) predIndt();

    return type;
}

// Рівень 10: PostfixExpr -> PrimaryExpr { FunctionCall | ArrayAccess | FieldAccess }
std::string Parser::parsePostfixExpr() {
    if (trace) std::cout << nextIndt() << "parsePostfixExpr():" << std::endl;

    SymbolRow fnRow = getSymb();

    std::string type = parsePrimaryExpr();
    while (true) {
        if (getSymb().lexeme == "(") {
            SymbolRow paren = getSymb();
            parseToken("(", "brackets_op");
            int argCount = 0;
            if (getSymb().lexeme != ")") {
                argCount = parseArguments();
            }
            parseToken(")", "brackets_op");
            FuncSymb* fn = sem.findFunc(fnRow.lexeme);
            if (fn) {
                if (argCount != fn->params.size()) {
                    std::cerr << "[Semantic Error]\nIn line: " << paren.line 
                              << ": ArgumentCountMismatch - function '" << fn->name 
                              << "' expects " << fn->params.size() 
                              << " arguments, got " << argCount << "\n" << std::endl;
                    exit(1);
                }
            }
        } else if (getSymb().lexeme == "[") {
            SymbolRow bracket = getSymb();
            parseToken("[", "brackets_op");
            std::string idxType = parseExpression();
            if (idxType != "Int") {
                std::cerr << "[Semantic Error]\nIn line: " << bracket.line 
                          << ": InvalidIndexType - must be Int, got: " << idxType << "\n" << std::endl;
                exit(1);
            }
            parseToken("]", "brackets_op");
            if (type.size() > 2 && type.front() == '[' && type.back() == ']') {
                type = type.substr(1, type.size() - 2);
            }
        } else if (getSymb().lexeme == ".") {
            SymbolRow dot = getSymb();
            parseToken(".", "punct");
            SymbolRow field = getSymb();
            parseToken("", "identifier");
            if (sem.hasRecord(type)) {
                if (!sem.hasRecordField(type, field.lexeme)) {
                    std::cerr << "[Semantic Error]\nIn line: " << field.line 
                              << ": NoSuchField - record '" << type 
                              << "' does not contain field '" << field.lexeme << "'\n" << std::endl;
                    exit(1);
                }
                type = sem.getRecordFieldType(type, field.lexeme);
            }
        } else {
            break;
        }
    }
    if (trace) predIndt();

    return type;
}

// Рівень 11: PrimaryExpr -> Const | Identifier | '(' Expression ')'
std::string Parser::parsePrimaryExpr() {
    SymbolRow s = getSymb();
    if (trace) std::cout << nextIndt() << "parsePrimaryExpr():" << std::endl;

    std::string returned_value = "";

    if (s.token == "unsigned_int"){
        returned_value = "Int";
        parseToken(s.lexeme, s.token);
    } else if (s.token == "unsigned_double"){
        returned_value = "Double";
        parseToken(s.lexeme, s.token);
    } else if (s.token == "bool_const"){
        returned_value = "Bool";
        parseToken(s.lexeme, s.token);
    } else if(s.token == "string_const"){
        returned_value = "String";
        parseToken(s.lexeme, s.token);
    } else if (s.token == "identifier") {
        FuncSymb* fn = sem.findFunc(s.lexeme);
        if (fn) {
            returned_value = fn->returnType;
            parseToken("", "identifier");
        } else {
            returned_value = sem.useVar(s.lexeme, s.line);
            parseToken("", "identifier");
        }
    } else if (s.lexeme == "(") {
        parseToken("(", "brackets_op");
        returned_value = parseExpression();
        parseToken(")", "brackets_op");
    } else {
        failParse("unexpected element in an expression", "constant, identifier or '('");
    }
    if (trace) predIndt();

    return returned_value;
}

// Arguments = Argument { ',' Argument }
int Parser::parseArguments() {
    if (trace) std::cout << nextIndt() << "parseArguments():" << std::endl;
    int count = 1;
    parseArgument();
    while (getSymb().lexeme == ",") {
        parseToken(",", "punct");
        parseArgument();
        count++;
    }
    if (trace) predIndt();

    return count;
}

// Argument = [ '&' ] Expression
void Parser::parseArgument() {
    if (trace) std::cout << nextIndt() << "parseArgument():" << std::endl;
    SymbolRow s = getSymb();
    if (getSymb().lexeme == "&") {
        SymbolRow amp = getSymb();
        parseToken("&", "ref_op");

        SymbolRow next = getSymb();
        if (next.token != "identifier") {
            std::cerr << "[Semantic Error]\nIn line: " << amp.line 
                      << ": InvalidRefArgument - expected LValue variable for ref parameter, got " << next.token << "\n" << std::endl;
            exit(1);
        }

    }
    parseExpression();
    if (trace) predIndt();
}