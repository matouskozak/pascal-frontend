//
// Created by matous on 6.5.19.
//

#include "AbstractSyntaxTree.h"


ASTNumber::ASTNumber (int val) : value(val) {}

ASTString::ASTString ( const std::string &str ) : str(str) {}

ASTArray::ASTArray(std::unique_ptr<ASTNumber> lower,
	std::unique_ptr<ASTNumber> upper,
	std::shared_ptr<ASTVariableType> type)
: lowerIdx(std::move(lower)), upperIdx(std::move(upper)), type(type) {}

ASTBody::ASTBody ( std::vector<std::unique_ptr<ASTExpression>> content ) : content(std::move(content)) {}

ASTVariable::ASTVariable (const std::string & name,std::shared_ptr<ASTVariableType> type)
	: name(name), type(type) {}

ASTConstVariable::ASTConstVariable ( const std::string & name, int value )
	: name(name), value(value) {}



ASTFunctionCall::ASTFunctionCall(const std::string & name,
	std::vector<std::unique_ptr<ASTExpression>> args)
	: name(name), arguments(std::move(args)) {}

ASTFunctionPrototype::ASTFunctionPrototype(const std::string & name,
	std::vector<std::unique_ptr<ASTVariable>> params,
	std::shared_ptr<ASTVariableType> ret)
	: returnType(ret), parameters(std::move(params)), name(name) {}

const std::string & ASTFunctionPrototype::getName() const { return name; }

ASTFunction::ASTFunction(std::unique_ptr<ASTFunctionPrototype> proto,
	std::vector<std::unique_ptr<ASTVariable>> local,
	std::unique_ptr<ASTBody> body)
	: prototype(std::move(proto)), local_variables(std::move(local)), body(std::move(body)) {}





ASTIf::ASTIf ( std::unique_ptr<ASTExpression> condition,
										std::unique_ptr<ASTBody> then_body,
                    std::unique_ptr<ASTBody> else_body )
                    : condition(std::move(condition)), then_body(std::move(then_body)), else_body(std::move(else_body)) {}

ASTFor::ASTFor ( const std::string & control_variable,
       std::unique_ptr<ASTExpression> start,
       std::unique_ptr<ASTExpression> end,
       std::unique_ptr<ASTExpression> step,
       std::unique_ptr<ASTBody> body,
       bool downto )
       : variable_name(control_variable), start(std::move(start)),
       end(std::move(end)), step(std::move(step)), body(std::move(body)),
       downto(downto) {}

ASTWhile::ASTWhile ( std::unique_ptr<ASTExpression> condition,
	std::unique_ptr<ASTBody> body )
												: condition(std::move(condition)), body(std::move(body)) {}






ASTReference::ASTReference ( const std::string &name ) : name(name) {}

ASTSingleVarReference::ASTSingleVarReference ( const std::string &name ) : ASTReference(name) {}

ASTArrayReference::ASTArrayReference ( const std::string &name, std::unique_ptr<ASTExpression> idx ) :
	ASTReference(name), index(std::move(idx)) {}



ASTAssignOp::ASTAssignOp ( std::unique_ptr<ASTReference> var, std::unique_ptr<ASTExpression> value )
: variable(std::move(var)), value(std::move(value)) {}

ASTBinaryOperator::ASTBinaryOperator(Token op,
                                     std::unique_ptr<ASTExpression> LHS,
                                     std::unique_ptr<ASTExpression> RHS)
	: op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

ASTProgram::ASTProgram ( const std::string name, std::vector<std::unique_ptr<ASTVariableDef>> global,
                         std::vector<std::unique_ptr<ASTFunction>> functions, std::unique_ptr<ASTBody> main ) :
                         name(name), global(std::move(global)), functions(std::move(functions)), main(std::move(main)) {}



// Procedure
/*ASTProcedureCall::ASTProcedureCall(const std::string & name,
	std::vector<std::unique_ptr<ASTExpression>> args)
: name(name), arguments(std::move(args)) {}*/
                         /*ASTProcedurePrototype::ASTProcedurePrototype(const std::string & name,
	std::vector<std::unique_ptr<ASTVariable>> params)
: name(name), parameters(std::move(params)) {}*/

/*const std::string & ASTProcedurePrototype::getName() const { return name; }*/
                         /*ASTProcedure::ASTProcedure(std::unique_ptr<ASTProcedurePrototype> proto,
	std::vector<std::unique_ptr<ASTVariable>> local,
	std::unique_ptr<ASTExpression> body)
: prototype(std::move(proto)), local_variables(std::move(local)), body(std::move(body)) {}*/