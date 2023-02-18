//
// Created by matous on 6.5.19.
//
#include <iostream>
#include <memory>
#include <vector>
#include <llvm/IR/Value.h>
#include  "llvm/IR/DerivedTypes.h"


#include "Lexan.h"

using namespace llvm;



class ASTExpression
{
public:
	virtual ~ASTExpression () = default;
	virtual Value * codegen() = 0;
};

// Number literals
class ASTNumber : public ASTExpression
{
public:
	ASTNumber ( int val );
	Value *codegen() override;
	int value;
};

// String
class ASTString : public ASTExpression
{
public:
	ASTString(const std::string & str);
	Value * codegen() override;
	std::string str;
};

// Variable Type
class ASTVariableType
{
public:
	virtual Type * codegen() = 0;
};

class ASTInteger : public ASTVariableType
{
public:
	Type * codegen();
};

class ASTArray : public ASTVariableType
{
public:
	ASTArray(std::unique_ptr<ASTNumber> lower, std::unique_ptr<ASTNumber> upper, std::shared_ptr<ASTVariableType> type);
	Type * codegen();
	std::unique_ptr<ASTNumber> lowerIdx, upperIdx;
private:
	std::shared_ptr<ASTVariableType> type;
};

// Function/Cycle body
class ASTBody
{
public:
	ASTBody(std::vector<std::unique_ptr<ASTExpression>> content);
	Value * codegen();

	std::vector<std::unique_ptr<ASTExpression>> content;
};

class ASTVariableDef : public ASTExpression
{
public:
	virtual Value * codegen() = 0;
};

// Variable name and type
class ASTVariable : public ASTVariableDef
{
public:
	ASTVariable (const std::string & name, std::shared_ptr<ASTVariableType> type );
	Value * codegen() override;

	const std::string name;
	std::shared_ptr<ASTVariableType> type;
private:
};

// Const Variable
class ASTConstVariable : public ASTVariableDef
{
public:
	ASTConstVariable (const std::string & name, int value );
	Value * codegen() override;

	const std::string name;
	const int value;
private:
};





// FUNCTION
// Call function
class ASTFunctionCall : public ASTExpression
{
public:
	ASTFunctionCall( const std::string & name, std::vector<std::unique_ptr<ASTExpression>> args );
	Value * codegen() override;
private:
	std::string name;
	std::vector<std::unique_ptr<ASTExpression>> arguments;
};

// Function prototype
class ASTFunctionPrototype
{
public:
	ASTFunctionPrototype(const std::string & name, std::vector<std::unique_ptr<ASTVariable>> params, std::shared_ptr<ASTVariableType> ret);
	Function * codegen ();

	const std::string & getName () const;
	std::shared_ptr<ASTVariableType> returnType;
	std::vector<std::unique_ptr<ASTVariable>> parameters;

private:
	std::string name;
};

// Function definition
class ASTFunction
{
public:
	ASTFunction( std::unique_ptr<ASTFunctionPrototype> proto,
	             std::vector<std::unique_ptr<ASTVariable>> local,
	             std::unique_ptr<ASTBody> body );
	Function * codegen ();

private:
	std::unique_ptr<ASTFunctionPrototype> prototype;
	std::vector<std::unique_ptr<ASTVariable>> local_variables;
	std::unique_ptr<ASTBody> body;

};

// Control-flow
class ASTIf : public ASTExpression
{
public:
	ASTIf(std::unique_ptr<ASTExpression> condition,
	      std::unique_ptr<ASTBody> then_body,
	      std::unique_ptr<ASTBody> else_body);
	Value * codegen() override;
private:
	std::unique_ptr<ASTExpression> condition;
	std::unique_ptr<ASTBody> then_body, else_body;
};

class ASTFor : public ASTExpression
{
public:
	ASTFor(const std::string & control_variable,
		std::unique_ptr<ASTExpression> start,
		std::unique_ptr<ASTExpression> end,
		std::unique_ptr<ASTExpression> step,
		std::unique_ptr<ASTBody> body,
		bool downto);

	Value * codegen() override;
private:
	const std::string variable_name;
	std::unique_ptr<ASTExpression> start, end, step;
	std::unique_ptr<ASTBody> body;
	bool downto;
};

class ASTWhile : public ASTExpression
{
public:
	ASTWhile(std::unique_ptr<ASTExpression> condition, std::unique_ptr<ASTBody> body);

	Value * codegen() override;
private:
	std::unique_ptr<ASTExpression> condition;
	std::unique_ptr<ASTBody> body;
};

class ASTBreak : public ASTExpression
{
public:
	Value * codegen() override;
};

class ASTExit : public ASTExpression
{
public:
	Value * codegen() override;
};

class ASTReference : public ASTExpression
{
public:
	ASTReference(const std::string & name);
	virtual Value * codegen() = 0;
	virtual Value * getAlloca() = 0;
	const std::string name;
};

class ASTSingleVarReference: public ASTReference
{
public:
	ASTSingleVarReference(const std::string & name);
	Value * codegen() override;
	Value * getAlloca() override;
};

class ASTArrayReference: public ASTReference
{
public:
	ASTArrayReference(const std::string & name, std::unique_ptr<ASTExpression> idx);
	Value * codegen() override;
	Value * getAlloca() override;
	const std::unique_ptr<ASTExpression> index;
};

class ASTAssignOp : public ASTExpression
{
public:
	ASTAssignOp(std::unique_ptr<ASTReference> var, std::unique_ptr<ASTExpression> value);
	Value * codegen() override;

	const std::unique_ptr<ASTReference> variable;
	const std::unique_ptr<ASTExpression> value;
};


// Binary operator
class ASTBinaryOperator : public ASTExpression
{
public:
	ASTBinaryOperator ( Token op, std::unique_ptr<ASTExpression> LHS, std::unique_ptr<ASTExpression> RHS );
	Value * codegen() override;
private:
	Token op;
	std::unique_ptr<ASTExpression> LHS, RHS;
};


class ASTProgram : public ASTExpression
{
public:
	ASTProgram(const std::string name,
		std::vector<std::unique_ptr<ASTVariableDef>> global,
		std::vector<std::unique_ptr<ASTFunction>> functions,
		std::unique_ptr<ASTBody> main);

	Value * codegen() override;
	std::unique_ptr<Module> runCodegen(const std::string & output_file);


	const std::string name;
	std::vector<std::unique_ptr<ASTVariableDef>> global;
	std::vector<std::unique_ptr<ASTFunction>> functions;
	std::unique_ptr<ASTBody> main;
};




// PROCEDURE
/*// Call procedure
class ASTProcedureCall : public ASTExpression
{
public:
	ASTProcedureCall ( const std::string & name, std::vector<std::unique_ptr<ASTExpression>> args );
	Value * codegen() override;
private:
	std::string name;
	std::vector<std::unique_ptr<ASTExpression>> arguments;
};*/

/*// Proto procedure
class ASTProcedurePrototype
{
public:
	ASTProcedurePrototype(const std::string & name, std::vector<std::unique_ptr<ASTVariable>> params);
	Function * codegen ();

	const std::string & getName() const;

private:
	std::string name;
	std::vector<std::unique_ptr<ASTVariable>> parameters;
};*/

/*// Procedure definition
class ASTProcedure
{
public:
	ASTProcedure ( std::unique_ptr<ASTProcedurePrototype> proto,
		std::vector<std::unique_ptr<ASTVariable>> local,
		std::unique_ptr<ASTExpression> body );
	Function * codegen ();

private:
	std::unique_ptr<ASTProcedurePrototype> prototype;
	std::vector<std::unique_ptr<ASTVariable>> local_variables;
	std::unique_ptr<ASTExpression> body;

};*/