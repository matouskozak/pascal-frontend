//
// Created by matous on 5.5.19.
//

#include <iostream>
#include <map>
#include <fstream>

enum Token {
	tok_identifier,
	tok_number,
	tok_eof,
	tok_error,
	tok_string,

	tok_plus,
	tok_minus,
	tok_multiply,
	tok_equal,
	tok_notEqual,
	tok_less,
	tok_lessEqual,
	tok_greater,
	tok_greaterEqual,
	tok_leftParenthesis,
	tok_rightParenthesis,
	tok_leftBracket,
	tok_rightBracket,
	tok_assign,
	tok_comma,
	tok_semicolon,
	tok_dot,
	tok_colon,

	tok_kwProgram,
	tok_kwConst,
	tok_kwVar,
	tok_kwBegin,
	tok_kwEnd,
	tok_kwDiv,
	tok_kwMod,
	tok_kwInteger,
	tok_kwArray,
	tok_kwOf,
	tok_kwIf,
	tok_kwThen,
	tok_kwElse,
	tok_kwWhile,
	tok_kwDo,
	tok_kwFor,
	tok_kwTo,
	tok_kwDownTo,
	tok_kwOr,
	tok_kwAnd,
	tok_kwProcedure,
	tok_kwFunction,
	tok_kwForward,
	tok_kwBreak,
	tok_kwExit,

	tok_kwWrite,
	tok_kwRead,
};


class Lexan {
public:
	Lexan(const std::string & input_file);
	std::string getIdentifierStr();
	int getNumVal();
	std::string getStringVal();
	Token getToken();
private:
	std::ifstream is;
	std::string identifier_str; // variable name - tok_identifier
	int num_val;                // tok_number
	std::string string_val;
	std::map<std::string, Token> key_words {
		{"program", tok_kwProgram},
		{"const", tok_kwConst},
		{"var", tok_kwVar},
		{"begin", tok_kwBegin},
		{"end", tok_kwEnd},
		{"div", tok_kwDiv},
		{"mod", tok_kwMod},
		{"integer", tok_kwInteger},
		{"array", tok_kwArray},
		{"of", tok_kwOf},
		{"if", tok_kwIf},
		{"then", tok_kwThen},
		{"else", tok_kwElse},
		{"while", tok_kwWhile},
		{"do", tok_kwDo},
		{"for", tok_kwFor},
		{"to", tok_kwTo},
		{"downto", tok_kwDownTo},
		{"or", tok_kwOr},
		{"and", tok_kwAnd},

		{"procedure", tok_kwProcedure},
		{"function", tok_kwFunction},
		{"forward", tok_kwForward},
		{"break", tok_kwBreak},
		{"exit", tok_kwExit},

/*		{"writeln", tok_kwWrite},
		{"readln", tok_kwRead}*/
	};

};
