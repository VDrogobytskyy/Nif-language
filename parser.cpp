#include "parser.h"


// Конструктор: приймає вектор карток-токенів від Lexer
Parser::Parser(const std::vector<SymbolRow>& symbols, bool enableTrace) 
    : tableOfSymb(symbols), len_tableOfSymb(symbols.size()), numRow(0), indt(0), trace(enableTrace) {
        modules["main"] = CodeModule{"main", "void", 0};
        curMod = &modules["main"];
    }

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
    if (modules.count("main") && !modules["main"].exitLabel.empty()) {
        CodeModule* saved = curMod;
        curMod = &modules["main"];
        placeLabel(modules["main"].exitLabel);
        curMod = saved;
    }
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

    // ===== КП4: перемикання активного модуля на модуль функції =====
    std::string fnName = fn.lexeme;
    bool isMain = (fnName == "main");
    CodeModule* prevMod = curMod;
    if (!isMain) {
        CodeModule& newMod = modules[fnName];
        newMod.name = fnName;
        newMod.returnType = toPsmType(return_type, fn.line);
        newMod.arity = static_cast<int>(pendingParams.size());
        newMod.exitLabel = "";
        curMod = &newMod;
    }
    hasReturn = false;
    parseBlock();
    if (!isMain) {
        // Якщо функція типу Void не завершилась інструкцією RET — додаємо її автоматично
        if (curMod->postfixCode.empty() || curMod->postfixCode.back().first != "RET") {
            if (return_type == "Void") {
                postfixCodeGen("RET", "RET");
            }
        }
        if (!curMod->exitLabel.empty()) {
            placeLabel(curMod->exitLabel);
            if (return_type == "Void" && (curMod->postfixCode.empty() || curMod->postfixCode.back().first != "RET")) {
                postfixCodeGen("RET", "RET");
            }
        }
        curMod = prevMod; // повертаємо генерацію назад у головний модуль (main)
    }

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
        if (v) {
            v->isAssigned = true;
            // КП4: прив'язка PSM-імені для параметра функції або ітератора циклу for
            if (curMod) {
                auto it = curMod->pendingPsmNames.find(p.name);
                if (it != curMod->pendingPsmNames.end()) {
                    v->psmName = it->second;
                    curMod->pendingPsmNames.erase(it);
                } else {
                    std::string pName = registerPsmVar(p.name, p.type, getSymb().line);
                    v->psmName = pName;
                }
            }
        }
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

    SymbolRow id = getSymb();
    parseToken("", "identifier");
    VarSymb* v = sem.findVar(id.lexeme);
    if (v) v->isAssigned = true;

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
    if (curMod && curMod->name != "main") {
        if (isStartOfExpression(getSymb())) {
            parseExpression();
        }
        postfixCodeGen("RET", "RET");
    } else {
        if (isStartOfExpression(getSymb())) {
            parseExpression();
            postfixCodeGen("POP", "stack_op");
        }
        if (curMod) {
            if (curMod->exitLabel.empty()) curMod->exitLabel = createLabel();
            postfixCodeGen(curMod->exitLabel, "label");
            postfixCodeGen("JMP", "jump");
        }
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
        genUnsupported("array element / record field assignment", id.line);   // КП4
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
        postfixCodeGen(psmName(id.lexeme), "l-val");          // КП4
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
        if (currentType == "Double" && expr_Type == "Int")    // КП4: Int -> Double
            postfixCodeGen("i2f", "conv");
        postfixCodeGen("=", "assign_op");                     // КП4
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

        // КП4: генерація виклику функції
        postfixCodeGen(id.lexeme, "CALL");
        if (curMod) curMod->calledFuncs.insert(id.lexeme);
        if (fn && fn->returnType != "Void") {
            postfixCodeGen("POP", "stack_op");
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
        SymbolRow op = getSymb();
        parseToken("||", "logical_op");
        std::string rType = parseLogicalAndExpr();
        type = checkBinaryOp(type, op.lexeme, rType, op.line);
    }
    if (trace) predIndt();
    return type;
}

// Рівень 3: LogicalAndExpr -> EqualityExpr { '&&' EqualityExpr }
std::string Parser::parseLogicalAndExpr() {
    if (trace) std::cout << nextIndt() << "parseLogicalAndExpr():" << std::endl;
    std::string type = parseEqualityExpr();
    while (getSymb().lexeme == "&&") {
        SymbolRow op = getSymb();
        parseToken("&&", "logical_op");
        std::string rType = parseEqualityExpr();
        type = checkBinaryOp(type, op.lexeme, rType, op.line);
    }
    if (trace) predIndt();
    return type;
}

// Рівень 4: EqualityExpr -> RelationalExpr { ('==' | '!=') RelationalExpr }
std::string Parser::parseEqualityExpr() {
    if (trace) std::cout << nextIndt() << "parseEqualityExpr():" << std::endl;
    std::string type = parseRelationalExpr();
    while (getSymb().lexeme == "==" || getSymb().lexeme == "!=") {
        SymbolRow op = getSymb();
        parseToken(op.lexeme, "rel_op");
        std::string rType = parseRelationalExpr();
        type = checkBinaryOp(type, op.lexeme, rType, op.line);
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
        SymbolRow op = getSymb();
        parseToken(op.lexeme, "rel_op");
        std::string rType = parseAddExpr();
        type = checkBinaryOp(type, op.lexeme, rType, op.line);
    }
    if (trace) predIndt();
    return type;
}

// Рівень 6: AddExpr -> MultExpr { ('+' | '-') MultExpr }
std::string Parser::parseAddExpr() {
    if (trace) std::cout << nextIndt() << "parseAddExpr():" << std::endl;
    std::string type = parseMultExpr();
    while (getSymb().lexeme == "+" || getSymb().lexeme == "-") {
        SymbolRow op = getSymb();
        parseToken(op.lexeme, "add_op");
        std::string rType = parseMultExpr();
        type = checkBinaryOp(type, op.lexeme, rType, op.line);
    }
    if (trace) predIndt();
    return type;
}

// Рівень 7: MultExpr -> PowerExpr { ('*' | '/' | '%') PowerExpr }
std::string Parser::parseMultExpr() {
    if (trace) std::cout << nextIndt() << "parseMultExpr():" << std::endl;
    std::string type = parsePowerExpr();
    while (getSymb().lexeme == "*" || getSymb().lexeme == "/" || getSymb().lexeme == "%") {
        SymbolRow op = getSymb();
        parseToken(op.lexeme, "mult_op");
        std::string rType = parsePowerExpr();
        type = checkBinaryOp(type, op.lexeme, rType, op.line);
    }
    if (trace) predIndt();
    return type;
}

// Рівень 8: PowerExpr -> UnaryExpr [ '**' PowerExpr ] (правоасоціативне!)
std::string Parser::parsePowerExpr() {
    if (trace) std::cout << nextIndt() << "parsePowerExpr():" << std::endl;
    std::string type = parseUnaryExpr();
    if (getSymb().lexeme == "**") {
        SymbolRow op = getSymb();
        parseToken("**", "exp_op");
        std::string rType = parsePowerExpr();
        type = checkBinaryOp(type, op.lexeme, rType, op.line);
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
            // КП4: виклик функції у виразі (результат залишається на стеку)
            postfixCodeGen(fnRow.lexeme, "CALL");
            if (curMod) curMod->calledFuncs.insert(fnRow.lexeme);
        } else if (getSymb().lexeme == "[") {
            SymbolRow bracket = getSymb();
            genUnsupported("array element access", bracket.line);  // КП4
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
            genUnsupported("record field access", dot.line);       // КП4
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


std::string Parser::checkBinaryOp(const std::string& lType, const std::string& op, const std::string& rType, int line) {
    bool lNum = (lType == "Int" || lType == "Double");
    bool rNum = (rType == "Int" || rType == "Double");

    // 1. Додавання '+'
    if (op == "+") {
        if (lNum && rNum) {
            return (lType == "Double" || rType == "Double") ? "Double" : "Int";
        }
        if (lType == "String" && rType == "String") {
            return "String";
        }
    }
    // 2. Віднімання '-', множення '*', ділення '/', степінь '**'
    else if (op == "-" || op == "*" || op == "/" || op == "**") {
        if (lNum && rNum) {
            return (lType == "Double" || rType == "Double") ? "Double" : "Int";
        }
    }
    // 3. Остача від ділення '%'
    else if (op == "%") {
        if (lType == "Int" && rType == "Int") {
            return "Int";
        }
    }
    // 4. Порівняння '<', '<=', '>', '>='
    else if (op == "<" || op == "<=" || op == ">" || op == ">=") {
        if ((lNum && rNum) || (lType == "String" && rType == "String")) {
            return "Bool";
        }
    }
    // 5. Рівність '==', '!='
    else if (op == "==" || op == "!=") {
        if ((lNum && rNum) || (lType == rType)) {
            return "Bool";
        }
    }
    // 6. Логічні '&&', '||'
    else if (op == "&&" || op == "||") {
        if (lType == "Bool" && rType == "Bool") {
            return "Bool";
        }
    }

    std::cerr << "[Semantic Error]\nIn line: " << line 
              << ": TypeMismatch - cannot apply operator '" << op 
              << "' to types: " << lType << " and " << rType << "\n" << std::endl;
    exit(1);
}

// 1. Додати інструкцію до ПОЛІЗу активного модуля
void Parser::postfixCodeGen(const std::string& lex, const std::string& tok) {
    if (curMod) curMod->postfixCode.push_back({lex, tok});
}
// 2. Згенерувати унікальну мітку m1, m2, ...
std::string Parser::createLabel() {
    return "m" + std::to_string(++labelCounter);
}
// 3. Зафіксувати адресу (індекс) мітки у postfixCode
void Parser::setValLabel(const std::string& label) {
    if (curMod) curMod->tableOfLabels[label] = static_cast<int>(curMod->postfixCode.size());
}
// 4. Поставити мітку у код: m label : colon
void Parser::placeLabel(const std::string& label) {
    if (!curMod) return;
    setValLabel(label);
    postfixCodeGen(label, "label");
    postfixCodeGen(":", "colon");
}
// 5. Відповідність типів Nif -> PSM v0.3
std::string Parser::toPsmType(const std::string& nifType, int line) {
    if (nifType == "Int")    return "int";
    if (nifType == "Double") return "float";
    if (nifType == "Bool")   return "bool";
    if (nifType == "String") return "string";
    if (nifType == "Void")   return "void";
    genUnsupported("type " + nifType, line);
    return "";
}
// 6. Зареєструвати змінну для секції .vars; повертає унікальне psm-ім'я (x, x_1, ...)
std::string Parser::registerPsmVar(const std::string& name, const std::string& nifType, int line) {
    if (!curMod) return name;
    std::string type = toPsmType(nifType, line);
    std::string candidate = name;
    int k = 0;
    while (curMod->psmVarTypes.count(candidate)) {
        candidate = name + "_" + std::to_string(++k);
    }
    curMod->psmVarTypes[candidate] = type;
    curMod->psmVars.push_back({candidate, type});
    return candidate;
}
// 7. Отримати psm-ім'я змінної з урахуванням поточної області видимості
std::string Parser::psmName(const std::string& name) {
    VarSymb* v = sem.findVar(name);
    if (v && !v->psmName.empty()) return v->psmName;
    return name;
}
// 8. Конструкція, не підтримувана PSM
void Parser::genUnsupported(const std::string& what, int line) {
    std::cerr << "[CodeGen Error]\nIn line: " << line << ": " << what
              << " is not supported by postfix translation (PSM v0.3)\n" << std::endl;
    exit(1);
}
// 9. Вставити i2f одразу після лівого операнда (позиція lEnd)
void Parser::genIntToDoubleAt(size_t pos) {
    if (!curMod) return;
    curMod->postfixCode.insert(curMod->postfixCode.begin() + pos, {"i2f", "conv"});
}
// 10. Семантична перевірка + генерація бінарного оператора з урахуванням PSM v0.3
std::string Parser::genBinaryOp(const std::string& lType, const std::string& op,
                                const std::string& rType, size_t lEnd, int line) {
    std::string resType = checkBinaryOp(lType, op, rType, line);
    if (!curMod) return resType;
    bool lNum = (lType == "Int" || lType == "Double");
    bool rNum = (rType == "Int" || rType == "Double");
    bool isRel = (op == "<" || op == "<=" || op == ">" || op == ">=" || op == "==" || op == "!=");
    // Логічні операції
    if (op == "&&") { postfixCodeGen("AND", "bool_op"); return resType; }
    if (op == "||") { postfixCodeGen("OR",  "bool_op"); return resType; }
    // Конкатенація рядків
    if (op == "+" && lType == "String") { postfixCodeGen("CAT", "cat_op"); return resType; }
    // PSM: rel_op не підтримує рядки
    if (isRel && (lType == "String" || rType == "String")) {
        genUnsupported("string comparison '" + op + "'", line);
    }
    // Степінь: PSM вимагає float для ^
    if (op == "**") {
        if (rType == "Int") postfixCodeGen("i2f", "conv");
        if (lType == "Int") genIntToDoubleAt(lEnd);
        postfixCodeGen("^", "pow_op");
        if (resType == "Int") postfixCodeGen("f2i", "conv");
        return resType;
    }
    // Змішані Int/Double -> приводимо Int до float
    if (lNum && rNum && lType != rType) {
        if (lType == "Int") genIntToDoubleAt(lEnd);
        else                postfixCodeGen("i2f", "conv");
    }
    if (isRel) {
        postfixCodeGen(op, "rel_op");
        return resType;
    }
    // Арифметичні операції + - * / %
    postfixCodeGen(op, "math_op");
    // У PSM int / int дає float, а в Nif Int / Int = Int -> додаємо f2i
    if (op == "/" && resType == "Int") postfixCodeGen("f2i", "conv");
    return resType;
}
// 11. Вивести таблиці та ПОЛІЗ усіх модулів у консоль
void Parser::printPostfixCode() {
    std::cout << "\n========== КП4: Трансляція у ПОЛІЗ ==========\n";
    for (const auto& m : modules) {
        const CodeModule& mod = m.second;
        std::cout << "\n----------------------------------------\n";
        std::cout << "Модуль: " << mod.name << " (тип повернення: " << mod.returnType 
                  << ", параметрів: " << mod.arity << ")\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Таблиця змінних (.vars):\n";
        for (const auto& v : mod.psmVars) {
            std::cout << "  " << std::left << std::setw(16) << v.first << v.second << "\n";
        }
        std::cout << "Таблиця міток (.labels):\n";
        for (const auto& l : mod.tableOfLabels) {
            std::cout << "  " << std::left << std::setw(8) << l.first << l.second << "\n";
        }
        std::cout << "Код у постфіксній формі (.code):\n";
        for (size_t i = 0; i < mod.postfixCode.size(); ++i) {
            std::cout << "  " << std::right << std::setw(4) << i << "  ('"
                      << mod.postfixCode[i].first << "', '" << mod.postfixCode[i].second << "')\n";
        }
    }
}
// 12. Зберегти один окремий модуль у файл формату PSM v0.3
void Parser::saveModule(const CodeModule& mod, const std::string& filePath, const std::string& baseName) {
    std::ofstream f(filePath);
    if (!f.is_open()) {
        std::cerr << "[CodeGen Error] cannot open file: " << filePath << std::endl;
        return;
    }
    f << ".target: Postfix Machine\n";
    f << ".version: 0.3\n\n";
    f << ".vars(\n";
    for (const auto& v : mod.psmVars) {
        f << "\t" << v.first << "\t" << v.second << "\n";
    }
    f << ")\n\n";
    // Якщо це допоміжна функція, і в main є змінні — передаємо їх у .globVarList
    if (mod.name != "main" && modules.count("main") && !modules.at("main").psmVars.empty()) {
        f << ".globVarList(\n";
        for (const auto& g : modules.at("main").psmVars) {
            f << "\t" << g.first << "\n";
        }
        f << ")\n\n";
    }
    // Секція .funcs: записуємо всі оголошені функції
    const auto& allFuncs = sem.getTableOfFuncs();
    if (!allFuncs.empty()) {
        f << ".funcs(\n";
        for (const auto& entry : allFuncs) {
            const FuncSymb& fn = entry.second;
            if (fn.name == "main") continue;
            std::string retType = toPsmType(fn.returnType, fn.line);
            f << "\t" << fn.name << "\t" << retType << "\t" << fn.params.size() << "\n";
        }
        f << ")\n\n";
    }
    f << ".labels(\n";
    for (const auto& l : mod.tableOfLabels) {
        f << "\t" << l.first << "\t" << l.second << "\n";
    }
    f << ")\n\n";
    f << ".code(\n";
    for (const auto& c : mod.postfixCode) {
        if (c.first == "RET") {
            f << "\tRET\n";
        } else {
            f << "\t" << c.first << "\t" << c.second << "\n";
        }
    }
    f << ")\n";
}
// 13. Зберегти всі модулі програми (main -> <base>.postfix, функції -> <base>$<func>.postfix)
bool Parser::savePostfixCode(const std::string& fileName) {
    std::string baseName = fileName;
    size_t dotPos = baseName.rfind(".postfix");
    if (dotPos != std::string::npos) {
        baseName = baseName.substr(0, dotPos);
    }
    // 1. Головний модуль програми: <baseName>.postfix
    if (modules.count("main")) {
        saveModule(modules.at("main"), fileName, baseName);
    }
    // 2. Допоміжні функції: <baseName>$<func>.postfix
    for (const auto& pair : modules) {
        if (pair.first == "main") continue;
        std::string fnFileName = baseName + "$" + pair.first + ".postfix";
        saveModule(pair.second, fnFileName, baseName);
        std::cout << "Module saved: " << fnFileName << "\n";
    }
    return true;
}