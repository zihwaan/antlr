// B2CMain.cpp
#include <iostream>
#include <map>
#include <stack>
#include "antlr4-runtime.h"
#include "antlr4-cpp/BBaseVisitor.h"
#include "antlr4-cpp/BLexer.h"
#include "antlr4-cpp/BParser.h"

using namespace std;
using namespace antlr4;
using namespace antlr4::tree;

enum Types {tyAUTO, tyINT, tyDOUBLE, tySTRING, tyBOOL, tyCHAR, tyFUNCTION};
string mnemonicTypes[] = {"auto", "int", "double", "string", "bool", "char", "function"};

struct SymbolAttributes {
   Types type; // int, double, bool, char, string, function --- auto if unknown yet
	int scope;  // 추가: 0 - global, 1 - local
   // if type == "function"
   vector<Types> retArgTypes; // first element is a return_type
};

class SymbolTable {
private:
    map<string, SymbolAttributes> table;  // symbol-name: string, symbol-typeInfo: SymbolAttributes

public:
    // Add a new symbol 
    void addSymbol(const string& name, const SymbolAttributes& attributes, int scope) {
    	table[name] = attributes;
    	table[name].scope = scope;  // 추가
	}

    // Check if a symbol exists
    bool symbolExists(const string& name) const {
        return table.find(name) != table.end();
    }

    // Get attributes of a symbol
    SymbolAttributes getSymbolAttributes(const string& name) const {
        if (symbolExists(name)) {
            return table.at(name);
        } else {
            cout << "Error: Symbol " << name << " not found" << endl;
        }
    }

    // Remove a symbol from the table
    void removeSymbol(const string& name) {
        table.erase(name);
    }

    // Print all symbols in the table (for debugging purposes)
    void printSymbols() const {
        for (const auto& pair : table) {
            cout << "(name) " << pair.first << ", (type) " << mnemonicTypes[pair.second.type];
			if (pair.second.type == tyFUNCTION) {
				cout << "| ";
				int n = pair.second.retArgTypes.size();
				if (n > 0) {
					cout << mnemonicTypes[pair.second.retArgTypes[0]] << "("; // return type
				}
				for (int i = 1; i < n-1; i++)
					cout << mnemonicTypes[pair.second.retArgTypes[i]] << ", ";
				if (n > 1) {
					cout << mnemonicTypes[pair.second.retArgTypes[n-1]]; // last arg type
				}
				cout << ")";
			} 
	    	cout << endl;
        }
    }
	// SymbolTable 클래스 내에 추가
	void updateFunctionSignature(const string& name, const vector<Types>& retArgTypes) {
		table[name].type = tyFUNCTION;
		table[name].retArgTypes = retArgTypes;
	}

};



/*
 * STEP 1. build symbol table
 */
const string _GlobalFuncName_ = "$_global_$";

// collection of per-function symbol tables accessed by function name
// symbol table in global scope can be accessed with special name defined in _GlobalFuncName_
map<string, SymbolTable*> symTabs;


class SymbolTableVisitor : public BBaseVisitor {
private:
   int scopeLevel;
   string curFuncName;
public:
	// Building symbol tables by visiting tree
	
	any visitProgram(BParser::ProgramContext *ctx) override {
		scopeLevel = 0; // global scope	
			
		// prepare symbol table for global scope
		SymbolTable* globalSymTab = new SymbolTable();
		curFuncName = _GlobalFuncName_;
    	symTabs[curFuncName] = globalSymTab;

		// visit children
    	for (int i=0; i< ctx->children.size(); i++) {
    		visit(ctx->children[i]);
    	}

		// print all symbol tables
		for (auto& pair : symTabs) {
	    	cout << "--- Symbol Table --- " << pair.first << endl; // function name
	    	pair.second->printSymbols();					   // per-function symbol table
			cout << "";
		}

    	return nullptr;
	}

    any visitDefinition(BParser::DefinitionContext *ctx) override {
		visit(ctx->children[0]);
        return nullptr;
	}

    any visitAutostmt(BParser::AutostmtContext *ctx) override {
        SymbolTable *stab = symTabs[curFuncName];

        for (int i=0, j=0; i < ctx->name().size(); i++) {
            string varName = ctx->name(i)->getText();
            enum Types varType = tyAUTO;

            int idx_assn = 1 + i*2 + j*2 + 1;
            if (ctx->children[idx_assn]->getText().compare("=") == 0) { 
                if (ctx->constant(j)) {  
                    varType = any_cast<Types>( visit(ctx->constant(j)) );
                    j++;
                }
            }

            int varScope = (curFuncName == _GlobalFuncName_) ? 0 : 1;  // 추가
            stab->addSymbol(varName, {varType}, varScope);  // 수정
        }
        return nullptr;
    }
    
    any visitFuncdef(BParser::FuncdefContext *ctx) override {
        string functionName = ctx->name(0)->getText();
        
        SymbolTable *newStab = new SymbolTable();
        curFuncName = functionName;
        symTabs[curFuncName] = newStab;
        
        vector<Types> signature;
        signature.push_back(tyAUTO);
        
        for(int i = 1; i < ctx->name().size(); i++) {
            string paramName = ctx->name(i)->getText();
            newStab->addSymbol(paramName, {tyAUTO}, 1);  // 로컬 변수로 추가
            signature.push_back(tyAUTO);
        }
        
        symTabs[_GlobalFuncName_]->updateFunctionSignature(functionName, signature);
        
        visit(ctx->blockstmt());
        
        curFuncName = _GlobalFuncName_;
        
        return nullptr;
    }


    any visitConstant(BParser::ConstantContext *ctx) override {
        
		if (ctx->INT()) return tyINT;
		else if (ctx->REAL()) return tyDOUBLE;
		else if (ctx->STRING()) return tySTRING;
		else if (ctx->BOOL()) return tyBOOL;
		else if (ctx->CHAR()) return tyCHAR;

		cout << "[ERROR] unrecognizable constant is used for initialization: " << ctx->children[0]->getText() << endl;
		exit(-1);
        return nullptr;
    }

};

/*
 * STEP 2. infer type
 */   
class TypeAnalysisVisitor : public BBaseVisitor {
private:
    string curFuncName;
public:
    any visitName(BParser::NameContext *ctx) override {
        string varName = ctx->NAME()->getText();
        SymbolTable *localStab = symTabs[curFuncName];
        
        if (localStab->symbolExists(varName)) {
            // 로컬 범위에서 변수를 찾음
            return localStab->getSymbolAttributes(varName).type;
        } else {
            // 글로벌 범위에서 변수를 찾음
            SymbolTable *globalStab = symTabs[_GlobalFuncName_];
            if (globalStab->symbolExists(varName)) {
                return globalStab->getSymbolAttributes(varName).type;
            } else {
                cerr << "Error: variable " << varName << " not declared" << endl;
                exit(1);
            }
        }
    }

    any visitFuncdef(BParser::FuncdefContext *ctx) override {
        string functionName = ctx->name(0)->getText();
        curFuncName = functionName;

        // 함수 본문 방문
        visit(ctx->blockstmt());

        curFuncName = _GlobalFuncName_;

        return nullptr;
    }
};

/*
 * STEP 3. print code
 */
class PrintTreeVisitor : public BBaseVisitor {
public:
    any visitProgram(BParser::ProgramContext *ctx) override {
    	// Perform some actions when visiting the program
    	for (int i=0; i< ctx->children.size(); i++) {
      	    visit(ctx->children[i]);
    	}
    	return nullptr;
    }
    
    any visitDirective(BParser::DirectiveContext *ctx) override {
		cout << ctx->SHARP_DIRECTIVE()->getText();
		cout << endl;
        return nullptr;
    }

    any visitDefinition(BParser::DefinitionContext *ctx) override {
		visit(ctx->children[0]);
        return nullptr;
    }

    any visitFuncdef(BParser::FuncdefContext *ctx) override {
		// Handle function definition
        string functionName = ctx->name(0)->getText();
		cout << "auto " << functionName << "(" ;
        // You can retrieve and visit the parameter list using ctx->name(i)
		for (int i=1; i < ctx->name().size(); i++) {
			if (i != 1) cout << ", ";
			cout << "auto " << ctx->name(i)->getText();		
		}
		cout << ")";

		// visit blockstmt
		visit(ctx->blockstmt());
        return nullptr;
    }

    any visitStatement(BParser::StatementContext *ctx) override {
		visit(ctx->children[0]);
        return nullptr;
    }

    any visitAutostmt(BParser::AutostmtContext *ctx) override {
    	// You can retrieve the variable names and constants using ctx->name(i) and ctx->constant(i)
		cout << "auto ";
		for (int i=0, j=0; i < ctx->name().size(); i++) {
			if (i != 0) cout << " ,";
			cout << ctx->name(i)->getText();

			int idx_assn = 1 + i*2 + j*2 + 1;  // auto name (= const)?, name (= const)?, ...
			if (ctx->children[idx_assn]->getText().compare("=") == 0) { 
				if (ctx->constant(j)) {
					cout << " = ";    
					visit(ctx->constant(j));
					j++;
				}
			}
		}
		cout << ";" << endl;
    	return nullptr;
    }

    any visitDeclstmt(BParser::DeclstmtContext *ctx) override {
		// Handle function declaration
        string functionName = ctx->name()->getText();
		cout << "auto " << functionName << "(" ;
        
		// You can retrieve and visit the parameter type list
		for (int i=1; i < ctx->AUTO().size(); i++) {
			if (i != 1) cout << ", ";
			cout << "auto ";		
		}
		cout << ");" << endl;
        return nullptr;
    }

    any visitBlockstmt(BParser::BlockstmtContext *ctx) override {
    	// Perform some actions when visiting a block statement
		cout << "{" << endl;
    	for (auto stmt : ctx->statement()) {
      	    visit(stmt);
    	}
		cout << "}" << endl;
    	return nullptr;
    }

    any visitIfstmt(BParser::IfstmtContext *ctx) override {
		cout << "if (";
		visit(ctx->expr());
		cout << ") " ;

		visit(ctx->statement(0));
		if (ctx->ELSE()) {
	   		cout << endl << "else ";
	   		visit(ctx->statement(1));
		}
        return nullptr;
    }

    any visitWhilestmt(BParser::WhilestmtContext *ctx) override {
        cout << "while (";
        visit(ctx->expr());
        cout << ") ";
        visit(ctx->statement());
        return nullptr;
    }

    any visitExpressionstmt(BParser::ExpressionstmtContext *ctx) override {
		visit(ctx->expression());
		cout << ";" << endl;
        return nullptr;
    }

    any visitReturnstmt(BParser::ReturnstmtContext *ctx) override {
		cout << "return";
		if (ctx->expression()) {
			cout << " (";
			visit(ctx->expression());
			cout << ")";
		}
		cout << ";" << endl;
        return nullptr;
    }

    any visitNullstmt(BParser::NullstmtContext *ctx) override {
		cout << ";" << endl;
        return nullptr;
    }

    any visitExpr(BParser::ExprContext *ctx) override {
		// unary operator
        if(ctx->atom()) {
            if (ctx->PLUS()) cout << "+";
            else if (ctx->MINUS()) cout << "-";
	    	else if (ctx->NOT()) cout << "!";
	    	visit(ctx->atom()); 
        }
		// binary operator
		else if (ctx->MUL() || ctx->DIV() || ctx->PLUS() || ctx->MINUS() || 
		 		ctx->GT() || ctx->GTE() || ctx->LT() || ctx->LTE() || ctx->EQ() || ctx->NEQ() ||
		 		ctx->AND() || ctx->OR() ) {
	    	visit(ctx->expr(0));
	    	cout << " " << ctx->children[1]->getText() << " "; // print binary operator
	    	visit(ctx->expr(1));
		}
		// ternary operator
		else if (ctx->QUEST()) {
			visit(ctx->expr(0));
			cout << " ? ";
			visit(ctx->expr(1));
			cout << " : ";
			visit(ctx->expr(2));
		}
		else {
			int lineNum = ctx->getStart()->getLine();
			cerr << endl << "[ERROR] visitExpr: unrecognized ops in Line " << lineNum << " --" << ctx->children[1]->getText() << endl;
			exit(-1); // error
        }	
        return nullptr;
    }
   
    any visitAtom(BParser::AtomContext *ctx) override {
		if (ctx->expression()) { // ( expression )
			cout << "(";
			visit(ctx->expression());
			cout << ")";
		}
		else	// name | constant | funcinvocation
			visit(ctx->children[0]);
        return nullptr;
    }
    
    any visitExpression(BParser::ExpressionContext *ctx) override {
        if (ctx->ASSN()) { // assignment
	   		visit(ctx->name());
	  		 cout << " = ";
		}
		visit(ctx->expr());
        return nullptr;
    }

    any visitFuncinvocation(BParser::FuncinvocationContext *ctx) override {
		cout << ctx->name()->getText() << "(";
		for (int i=0; i < ctx->expr().size(); i++) {
			if (i != 0) cout << ", ";
			visit(ctx->expr(i));
		}
		cout << ")";
        return nullptr;
    }
    
    any visitConstant(BParser::ConstantContext *ctx) override {
        cout << ctx->children[0]->getText();
        return nullptr;
    }
    
    any visitName(BParser::NameContext *ctx) override {
		cout << ctx->NAME()->getText();
        return nullptr;
    }
};

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        cerr << "[Usage] " << argv[0] << "  <input-file>\n";
        exit(0);
    }
    std::ifstream stream;
    stream.open(argv[1]);
    if (stream.fail()) {
        cerr << argv[1] << " : file open fail\n";
        exit(0);
    }

	//cout << "/*-- B2C ANTLR visitor --*/\n";

	ANTLRInputStream inputStream(stream);
	BLexer lexer(&inputStream);
	CommonTokenStream tokenStream(&lexer);
	BParser parser(&tokenStream);
	ParseTree* tree = parser.program();

	// STEP 1. visit parse tree and build symbol tables for functions (PA#1)
	cout << endl << "/*** STEP 1. BUILD SYM_TABS *************" << endl;
	SymbolTableVisitor SymtabTree;
	SymtabTree.visit(tree);
	cout <<         " ---    end of step 1       ------------*/" << endl;

	// STEP 2. visit parse tree and perform type inference for 'auto' typed variables and functions (PA#2)
	cout << endl << "/*** STEP 2. ANALYZE TYPES  *************" << endl;
	TypeAnalysisVisitor AnalyzeTree;
	AnalyzeTree.curFuncName = _GlobalFuncName_; // 추가
	AnalyzeTree.visit(tree);
	cout <<         " ---    end of step 2       ------------*/" << endl;

	// STEP 3. visit parse tree and print out C code with correct types
	cout << endl << "/*** STEP 3. TRANSFORM to C *************/" << endl;
	PrintTreeVisitor PrintTree;
	PrintTree.visit(tree);

	return 0;
}
