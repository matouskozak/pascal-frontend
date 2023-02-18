//
// Created by matous on 5.5.19.
//
#include "AbstractSyntaxTree.h"

#include <iostream>
#include <map>
#include <string>
#include <memory>


#define _DEBUG_PARSER_

class Parser
{
public:
	Parser(const std::string & file_name);

	std::unique_ptr<ASTProgram> start();

	std::unique_ptr<ASTExpression> parseExpression();
	std::unique_ptr<ASTExpression> parseStatement();
	std::unique_ptr<ASTNumber> parseNumberExpr();
	std::unique_ptr<ASTString> parseStringExpr();

	std::unique_ptr<ASTExpression> parseParenthesisExpr();
	std::unique_ptr<ASTExpression> parseIdentifierExpr();
	std::unique_ptr<ASTExpression> parsePrimaryExpr();
	std::unique_ptr<ASTExpression> parseBinaryOperatorRHS(int precedence, std::unique_ptr<ASTExpression> LHS);



	std::shared_ptr<ASTVariableType> parseVarType();
	std::vector<std::unique_ptr<ASTVariable>> parseVarDecl ();
	std::vector<std::unique_ptr<ASTConstVariable>> parseConstVarDecl ();



	// Procedures and Functions
/*	std::unique_ptr<ASTProcedurePrototype> parseProcedurePrototype ();
	std::unique_ptr<ASTProcedure> parseProcedure();*/

	std::unique_ptr<ASTExpression> parseContentLine();
	std::unique_ptr<ASTBody> parseBody();
	std::unique_ptr<ASTFunctionPrototype> parseFunctionPrototype ();
	std::unique_ptr<ASTFunction> parseFunction();


	std::unique_ptr<ASTIf> parseIfExpr();
	std::unique_ptr<ASTFor> parseForExpr();
	std::unique_ptr<ASTWhile> parseWhileExpr();

	std::unique_ptr<ASTBreak> parseBreak();
	std::unique_ptr<ASTExit> parseExit();

	// std::unique_ptr<ASTReference> parseSingleVarReference();
	std::unique_ptr<ASTArrayReference> parseArrayReference(const std::string & name);

	std::unique_ptr<ASTAssignOp> parseAssign(const std::string & var_name);
	std::unique_ptr<ASTAssignOp> parseAssign(std::unique_ptr<ASTReference> var_ref);

private:
	Lexan lexan;
	std::map<Token, int> bin_op_precedence;
	Token current_token;
	int getTokenPrecedence();
	Token getNextToken();
	bool validateToken (Token correct);

	std::unique_ptr<ASTExpression> logError(const char * str) {
		fprintf(stderr, "Error: %s\n", str);
		return NULL;
	}
	/*
	std::unique_ptr<ASTProcedurePrototype> logErrorP(const char * str) {
		logError(str);
		return NULL;
	}
	std::unique_ptr<ASTVariableType> logErrorV(const char * str) {
		logError(str);
		return NULL;
	}*/
};

