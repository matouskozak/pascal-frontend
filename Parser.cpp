///
// Created by matous on 5.5.19.
//



#include "Parser.h"

static std::string tokenToStr( Token tok )
{
	switch ( tok ) {
		case tok_identifier :
			return "identifier";
		case tok_number :
			return "number";
		case tok_eof :
			return "EOF";
		case tok_error :
			return "ERROR";
		case tok_string :
			return "string";


		case tok_plus :
			return "+";
		case tok_minus :
			return "-";
		case tok_multiply :
			return "*";
		case tok_equal :
			return "=";
		case tok_notEqual :
			return "<>";
		case tok_less :
			return "<";
		case tok_lessEqual :
			return "<=";
		case tok_greater :
			return ">";
		case tok_greaterEqual :
			return ">=";
		case tok_leftParenthesis :
			return "(";
		case tok_rightParenthesis :
			return ")";
		case tok_leftBracket :
			return "[";
		case tok_rightBracket :
			return "]";
		case tok_assign :
			return ":=";
		case tok_comma :
			return ",";
		case tok_semicolon :
			return ";";
		case tok_dot :
			return ".";
		case tok_colon :
			return ":";

		case tok_kwProgram :
			return "program";
		case tok_kwConst :
			return "const";
		case tok_kwVar :
			return "var";
		case tok_kwBegin :
			return "begin";
		case tok_kwEnd :
			return "end";
		case tok_kwDiv :
			return "DIV";
		case tok_kwMod :
			return "MOD";
		case tok_kwInteger :
			return "integer";
		case tok_kwArray :
			return "array";
		case tok_kwOf :
			return "of";
		case tok_kwIf :
			return "if";
		case tok_kwThen :
			return "then";
		case tok_kwElse :
			return "else";
		case tok_kwWhile :
			return "while";
		case tok_kwDo :
			return "dow";
		case tok_kwFor :
			return "for";
		case tok_kwTo :
			return "to";
		case tok_kwDownTo :
			return "downto";
		case tok_kwOr :
			return "or";
		case tok_kwAnd :
			return "and";

		case tok_kwProcedure :
			return "procedure";
		case tok_kwFunction :
			return "function";
		case tok_kwForward :
			return "forward";
		case tok_kwBreak :
			return "break";
		case tok_kwExit :
			return "exit";

		case tok_kwWrite :
			return "writeln";
		case tok_kwRead :
			return "readln";

		default:
			return "UNDEFINED TOKEN";
	}
}

Parser::Parser (const std::string & file_name) : lexan(file_name)
{
	// Lowest priority
	//bin_op_precedence[Token::tok_assign] = 10;
	bin_op_precedence[Token::tok_less] = 10;
	bin_op_precedence[Token::tok_lessEqual] = 10;
	bin_op_precedence[Token::tok_greater] = 10;
	bin_op_precedence[Token::tok_greaterEqual] = 10;
	bin_op_precedence[Token::tok_equal] = 10;
	bin_op_precedence[Token::tok_notEqual] = 10;
	// 3rd degree priority
	bin_op_precedence[Token::tok_plus] = 20;
	bin_op_precedence[Token::tok_minus] = 20;
	bin_op_precedence[Token::tok_kwOr] = 20;

	// 2nd degree prioty
	bin_op_precedence[Token::tok_multiply] = 40;
	bin_op_precedence[Token::tok_kwDiv] = 40;
	bin_op_precedence[Token::tok_kwMod] = 40;
	bin_op_precedence[Token::tok_kwAnd] = 40;
}

/**
 * 'program' name ';'
 * [var_declaration]
 * [const_declaration]
 * [function] [procedure]
 * [main]
 */
std::unique_ptr<ASTProgram> Parser::start()
{
	getNextToken();
	validateToken(tok_kwProgram);	getNextToken();

	validateToken(tok_identifier);
	std::string program_name = lexan.getIdentifierStr();
	getNextToken();

	validateToken(tok_semicolon); getNextToken();

	std::vector<std::unique_ptr<ASTVariableDef>> global;
	std::vector<std::unique_ptr<ASTFunction>> functions;
	std::unique_ptr<ASTBody> main;


	while ( 1 ) {
		if ( current_token == tok_kwVar ) {
			auto global_vars = parseVarDecl();
			for ( auto & v : global_vars )
				global.push_back(std::move(v));
		} else if ( current_token == tok_kwConst ) {
			auto const_vars = parseConstVarDecl();
			for ( auto & v : const_vars )
				global.push_back(std::move(v));
		} else if ( current_token == tok_kwProcedure || current_token == tok_kwFunction ) {
			functions.emplace_back(parseFunction());
		} else {
			// Parse main
			main = parseBody();
			break;
		}
	}

	validateToken(tok_dot); getNextToken();


	return std::make_unique<ASTProgram>(program_name, std::move(global), std::move(functions), std::move(main));
}

/**
 * [number]
 * { '-' } tok_number
 */
std::unique_ptr<ASTNumber> Parser::parseNumberExpr()
{
	int sgn = 1;
	if ( current_token == tok_minus ) {
		getNextToken();
		sgn = -1;
	}

	validateToken(tok_number);
	auto res = std::make_unique<ASTNumber>(sgn * lexan.getNumVal());
	getNextToken();

	return std::move(res);
}

/**
 * [string]
 * tok_string
 */
std::unique_ptr<ASTString> Parser::parseStringExpr()
{
	validateToken(tok_string);
	auto res = std::make_unique<ASTString>(lexan.getStringVal());
	getNextToken();

	return std::move(res);
}

/**
 * '(' expression ')'
 */
std::unique_ptr<ASTExpression> Parser::parseParenthesisExpr()
{
	validateToken(tok_leftParenthesis); getNextToken();

	auto res = parseExpression();

	validateToken(tok_rightParenthesis); getNextToken();

	return res;

}


std::unique_ptr<ASTExpression> Parser::parseIdentifierExpr()
{
	validateToken(tok_identifier);
	std::string identifier = lexan.getIdentifierStr();
	getNextToken(); // Move beyond identifier


/*	if ( current_token == tok_assign )
		return parseAssign(identifier);
	else if ( current_token == tok_leftBracket )
		return parseArrayReference(identifier);*/
	if ( current_token != tok_leftParenthesis ) {
		std::unique_ptr<ASTReference> variable_ref = nullptr;
		if ( current_token == tok_leftBracket )
			variable_ref = std::move(parseArrayReference(identifier));
		else
			variable_ref = std::make_unique<ASTSingleVarReference>(identifier);

		if ( current_token == tok_assign )
			return parseAssign(std::move(variable_ref));
		else
			return variable_ref;
	}


	// Function call
	validateToken(tok_leftParenthesis);	getNextToken(); // Eat "("
	std::vector<std::unique_ptr<ASTExpression>> arguments;

	while ( current_token != tok_rightParenthesis ) {
		auto argument = parseExpression();
		if ( argument )
			arguments.push_back(std::move(argument));
		else
			return nullptr;

		if ( current_token == tok_rightParenthesis )
			break;

		validateToken(tok_comma); getNextToken();
	}

	getNextToken(); // Eat ")"

	return std::make_unique<ASTFunctionCall>(identifier, std::move(arguments));
}
std::unique_ptr<ASTExpression> Parser::parsePrimaryExpr()
{
	switch ( current_token ) {
		case Token::tok_identifier:
			return parseIdentifierExpr();
		case Token::tok_number:
		case tok_minus:
			return parseNumberExpr();
		case tok_string:
			return parseStringExpr();
		case Token::tok_leftParenthesis:
			return parseParenthesisExpr();
		default:
			throw ("unknown token when expecting an primary expression");
	}
}

std::unique_ptr<ASTExpression> Parser::parseExpression()
{
	auto LHS = parsePrimaryExpr();
	if ( !LHS )
		return nullptr;

	return parseBinaryOperatorRHS(1, std::move(LHS));
}

std::unique_ptr<ASTExpression> Parser::parseBinaryOperatorRHS(int precedence, std::unique_ptr<ASTExpression> LHS)
{
	while ( 1 ) {
		int current_precendence = getTokenPrecedence();

		// End of operator grouping
		if ( current_precendence < precedence )
			return LHS;

		Token bin_op = current_token;
		getNextToken();

		auto RHS = parsePrimaryExpr();
		if ( !RHS )
			return nullptr;

		int next_precendence = getTokenPrecedence();

		// The next operator binds current operator as Left side of expression
		if ( current_precendence < next_precendence ) {
			RHS = parseBinaryOperatorRHS(current_precendence + 1, std::move(RHS));
			if ( !RHS )
				return nullptr;
		}

		LHS = std::make_unique<ASTBinaryOperator>(bin_op, std::move(LHS), std::move(RHS));
	}
}

std::unique_ptr<ASTExpression> Parser::parseStatement()
{
	return nullptr;
}

/**
 * [type]
 * 'integer' | 'array' '[' [number] '..' [number] ']' 'of' [type]
 */
std::shared_ptr<ASTVariableType> Parser::parseVarType()
{
	if ( current_token == tok_kwInteger ) {
		getNextToken();
		return std::make_shared<ASTInteger>();
	} else if ( current_token == tok_kwArray ) {
		getNextToken();

		validateToken(tok_leftBracket);	getNextToken(); // Eat '['

		// Lower idx
		std::unique_ptr<ASTNumber> lower_idx = parseNumberExpr();

		// ..
		validateToken(tok_dot);	getNextToken();
		validateToken(tok_dot);	getNextToken();

		std::unique_ptr<ASTNumber> upper_idx = parseNumberExpr();

		validateToken(tok_rightBracket); getNextToken(); // Eat ']'

		validateToken(tok_kwOf); getNextToken();

		std::shared_ptr<ASTVariableType> type = parseVarType();

		return std::make_shared<ASTArray>(std::move(lower_idx), std::move(upper_idx), type);
	}
	return nullptr;
}
/**
 * [var_declaration]
 * 'var' {identifier {',' identifier}* ':' [type] ';'}+
 */
std::vector<std::unique_ptr<ASTVariable>> Parser::parseVarDecl ()
{
	validateToken(tok_kwVar);
	getNextToken();
	std::vector<std::unique_ptr<ASTVariable>> result;
	std::vector<std::string> variable_names;


	if ( current_token == tok_identifier ) {
		variable_names.clear();
		variable_names.push_back(lexan.getIdentifierStr());
		getNextToken();

		while ( current_token == tok_comma ) {
			getNextToken(); // "Eat ','

			validateToken(tok_identifier);
			variable_names.push_back(lexan.getIdentifierStr());
			getNextToken();
		}

		validateToken(tok_colon);
		getNextToken(); // "Eat ":"

		auto type = parseVarType();
		validateToken(tok_semicolon);
		getNextToken();

		for ( const std::string &name : variable_names )
			result.emplace_back(std::make_unique<ASTVariable>(name, type));
	}

	return result;
}
/**
 * [const_declaration]
 * 'const' {identifier '=' value ';'}+
 */
std::vector<std::unique_ptr<ASTConstVariable>> Parser::parseConstVarDecl ()
{
	validateToken(tok_kwConst);
	getNextToken();

	std::vector<std::unique_ptr<ASTConstVariable>> result;

	do {
		validateToken(tok_identifier);
		std::string name = lexan.getIdentifierStr();
		getNextToken();

		validateToken(tok_equal);
		getNextToken();

		validateToken(tok_number);
		result.emplace_back(std::make_unique<ASTConstVariable>(name, lexan.getNumVal()));
		getNextToken();

		validateToken(tok_semicolon);
		getNextToken();
	} while ( current_token == tok_identifier );

	return result;
}




/**
 * [line]
 * ASTExpression ';' (semilocon not parsed here)
 */
std::unique_ptr<ASTExpression> Parser::parseContentLine()
{
	switch ( current_token ) {
		case tok_identifier:
			return parseIdentifierExpr();
		case tok_kwIf:
			return parseIfExpr();
		case tok_kwFor:
			return parseForExpr();
		case tok_kwWhile:
			return parseWhileExpr();
		case tok_kwBreak:
			getNextToken();
			return std::make_unique<ASTBreak>();
		case tok_kwExit:
			getNextToken();
			return std::make_unique<ASTExit>();
		default:
			return nullptr;
	}
}
/**
 * [body]
 * ('begin' {[line]}* 'end') / ([line])
 */
std::unique_ptr<ASTBody> Parser::parseBody ()
{
	std::vector<std::unique_ptr<ASTExpression>> content;

	if ( current_token == tok_kwBegin ) {
		getNextToken();

		while ( current_token != tok_kwEnd && current_token != tok_eof ) {
			content.emplace_back(parseContentLine());
			while ( current_token == tok_semicolon ) {
				getNextToken();
				if ( current_token == tok_kwEnd )
					break;
				content.emplace_back(parseContentLine());
			}
			//validateToken(tok_semicolon); getNextToken();
		}
		validateToken(tok_kwEnd); getNextToken();
	} else {
		content.emplace_back(parseContentLine());
	}

	return std::make_unique<ASTBody>(std::move(content));
}

/**
 * [function_proto]
 * 'function' function_name '(' {var_name ':' [type] ';'}* ')' ':' [type] ';'
 */
std::unique_ptr<ASTFunctionPrototype> Parser::parseFunctionPrototype ()
{
	if ( current_token != tok_kwFunction && current_token != tok_kwProcedure )
		return nullptr;

	bool isFunction = false;
	if ( current_token == tok_kwFunction )
		isFunction = true;

	//validateToken(tok_kwFunction);
	getNextToken();

	validateToken(tok_identifier);
	std::string function_name = lexan.getIdentifierStr();
	getNextToken();

	validateToken(tok_leftParenthesis);
	getNextToken();

	std::vector<std::unique_ptr<ASTVariable>> params;

	// Params
	while ( current_token == tok_identifier ) {
		std::string param_name = lexan.getIdentifierStr();
		getNextToken();

		validateToken(tok_colon);
		getNextToken();

		auto type = parseVarType();

		params.emplace_back(std::make_unique<ASTVariable>(param_name, type));

		if ( current_token != tok_semicolon )
			break;

		// validateToken(tok_semicolon);
		getNextToken();
	}

	validateToken(tok_rightParenthesis);
	getNextToken(); // eat ")"

	std::shared_ptr<ASTVariableType> return_type = nullptr;

	if ( isFunction ) {
		validateToken(tok_colon);
		getNextToken();

		return_type = parseVarType();
	}
	validateToken(tok_semicolon);
	getNextToken();

	return std::make_unique<ASTFunctionPrototype>(function_name, std::move(params), return_type);
}



/**
 * [function_proto] {'forward' ';'}
 * [var_declaration]
 * [body_begin_end]
 */
std::unique_ptr<ASTFunction> Parser::parseFunction()
{
	auto prototype = parseFunctionPrototype();
	std::vector<std::unique_ptr<ASTVariable>> local;

	// Forward declaration
	if ( current_token == tok_kwForward ) {
		getNextToken();

		validateToken(tok_semicolon);
		getNextToken();

		return std::make_unique<ASTFunction>(std::move(prototype), std::move(local), nullptr);
	}

	// Parse local variable declarations
	while ( current_token == tok_kwVar ) {
		auto vars = parseVarDecl();
		for ( auto & v : vars )
			local.push_back(std::move(v));
	}

	validateToken(tok_kwBegin);

	auto body = parseBody();

	validateToken(tok_semicolon);
	getNextToken();

	return std::make_unique<ASTFunction>(std::move(prototype), std::move(local), std::move(body));
}

/**
 * [if]
 * 'if' condition
 * 'then' [body]
 * {'else' [body]}
 */
std::unique_ptr<ASTIf> Parser::parseIfExpr()
{
	validateToken(tok_kwIf);
	getNextToken();

	auto condition = parseExpression();

	validateToken(tok_kwThen);
	getNextToken();

	auto then_body = parseBody();

	if ( current_token == tok_kwElse ) {
		getNextToken();
		auto else_body = parseBody();

		return std::make_unique<ASTIf>(std::move(condition), std::move(then_body), std::move(else_body));
	}
	return std::make_unique<ASTIf>(std::move(condition), std::move(then_body), nullptr);
}

/**
 * [for]
 * 'for' var_name ':=' [number] 'to'/'downto' [number] 'do'
 * [body]
 */
std::unique_ptr<ASTFor> Parser::parseForExpr()
{
	validateToken(tok_kwFor);
	getNextToken();

	validateToken(tok_identifier);
	std::string control_variable = lexan.getIdentifierStr();
	getNextToken();

	validateToken(tok_assign);
	getNextToken();

	auto start = parseExpression();
	if ( !start )
		return nullptr;

	bool downto;
	if ( current_token == tok_kwTo )
		downto = false;
	else if ( current_token == tok_kwDownTo )
		downto = true;
	else
		throw "Expected 'to' or 'downto'. Given " + tokenToStr(current_token) + ".";

	getNextToken();

	auto end = parseExpression();
	if ( !end )
		return nullptr;

	auto step = nullptr;

	validateToken(tok_kwDo);
	getNextToken();

	auto for_body = parseBody();

	return std::make_unique<ASTFor>(control_variable, std::move(start), std::move(end)
		, std::move(step), std::move(for_body), downto);
}

/**
 * [while]
 * 'while' condition 'do'
 * [body]
 */
std::unique_ptr<ASTWhile> Parser::parseWhileExpr()
{
	validateToken(tok_kwWhile); getNextToken();

	auto condition = parseExpression();

	validateToken(tok_kwDo); getNextToken();

	auto while_body = parseBody();

	return std::make_unique<ASTWhile>(std::move(condition), std::move(while_body));
}



/*std::unique_ptr<ASTBreak> parseBreak();
std::unique_ptr<ASTExit> parseExit();*/

/**
 * identifier '[' expression ']'
 */
std::unique_ptr<ASTArrayReference>  Parser::parseArrayReference(const std::string & name)
{
	validateToken(tok_leftBracket); getNextToken();

	auto idx = parseExpression();

	validateToken(tok_rightBracket); getNextToken();

	return std::make_unique<ASTArrayReference>(name, std::move(idx));
}
/**
 * [var_reference] ':=' expression
 */
std::unique_ptr<ASTAssignOp> Parser::parseAssign(const std::string & var_name)
{
	/*	validateToken(tok_identifier);
	std::string variable_name = lexan.getIdentifierStr();
	getNextToken();*/

	std::unique_ptr<ASTReference> variable_ref = nullptr;
	if ( current_token == tok_leftBracket )
		variable_ref = std::move(parseArrayReference(var_name));
	else
		variable_ref = std::make_unique<ASTSingleVarReference>(var_name);

	validateToken(tok_assign); getNextToken();

	auto new_value = parseExpression();

	return std::make_unique<ASTAssignOp>(std::move(variable_ref), std::move(new_value));
}
/**
 * [var_reference] ':=' expression
 */
std::unique_ptr<ASTAssignOp> Parser::parseAssign(std::unique_ptr<ASTReference> var_ref)
{
	validateToken(tok_assign); getNextToken();

	auto new_value = parseExpression();

	return std::make_unique<ASTAssignOp>(std::move(var_ref), std::move(new_value));
}


/**
 * [procedure_proto]
 * 'procedure' procedure_name '(' {var_ident ':' type ';'}* ')' ';'
 */
 /*std::unique_ptr<ASTProcedurePrototype> Parser::parseProcedurePrototype ()
{
	validateToken(tok_kwProcedure);
	getNextToken();

	validateToken(tok_identifier);
	std::string procedure_name = lexan.getIdentifierStr();
	getNextToken();

	validateToken(tok_leftParenthesis);
	getNextToken();

	std::vector<std::unique_ptr<ASTVariable>> params;

	// Params
	while ( current_token == tok_identifier ) {
		std::string param_name = lexan.getIdentifierStr();
		getNextToken();

		validateToken(tok_colon);
		getNextToken();

		auto type = parseVarType();

		params.emplace_back(std::make_unique<ASTVariable>(param_name, type));

		if ( current_token != tok_semicolon )
			break;

		// validateToken(tok_semicolon);
		getNextToken();
	}

	validateToken(tok_rightParenthesis);
	getNextToken(); // eat ")"

	validateToken(tok_semicolon);
	getNextToken();

	return std::make_unique<ASTProcedurePrototype>(procedure_name, std::move(params));
}*/

/** [procedure_proto] {'forward' ';'}
 * [var_declaration]
 * [body_begin_end]
*/

/*std::unique_ptr<ASTProcedure> Parser::parseProcedure()
{
	auto prototype = parseProcedurePrototype();
	std::vector<std::unique_ptr<ASTVariable>> local;

	// Forward declaration
	if ( current_token == tok_kwForward ) {
		getNextToken();
		validateToken(tok_semicolon);
		getNextToken();

		return std::make_unique<ASTProcedure>(std::move(prototype), std::move(local), nullptr);
	}

	// Parse local variable declarations
	while ( current_token == tok_kwVar ) {
		getNextToken();
		auto vars = parseVarDecl();
		local.emplace(local.cend(), vars);
	}

	// Parse body
	validateToken(tok_kwBegin);
	getNextToken();

	auto body = parseExpression();

	validateToken(tok_kwEnd);
	getNextToken();

	validateToken(tok_semicolon);
	getNextToken();

	return std::make_unique<ASTProcedure>(std::move(prototype), std::move(local), std::move(body));
}*/

/* #############################PRIVATE############################# */
/**
 * Move lexan to next token
 * @return next token
 */
Token Parser::getNextToken ()
{
	return current_token = lexan.getToken();
}
/**
 * Return token precedence if its binary operator
 * @return token_precendence or -1
 */
int Parser::getTokenPrecedence()
{
	auto it = bin_op_precedence.find(current_token);
	if ( it == bin_op_precedence.cend() )
		return -1;

	return it -> second;
}

/**
 * Compares current_token with expected token
 * @param correct expected token
 * @return true for match, else false or throw exception
 */
bool Parser::validateToken (Token correct)
{
	if ( current_token == correct )
		return true;

	std::string msg ("Expected: '" + tokenToStr(correct) + "'. Received: '" + tokenToStr(current_token) + "'.");
	throw msg;
	return false;
}
