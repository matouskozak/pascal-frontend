//
// Created by matous on 16.5.19.
//


#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include <iostream>

#include "AbstractSyntaxTree.h"


static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;

typedef std::pair<Value *, std::shared_ptr<ASTVariableType>> TVarInfo;

static std::map<std::string, std::pair<AllocaInst *, std::shared_ptr<ASTVariableType>>> named_values;
static std::map<std::string, std::pair<GlobalVariable *, std::shared_ptr<ASTVariableType>>> global_vars;
static std::map<std::string, Constant *> const_vars;

static Value * decimal_specifier_character;
static Value * string_specifier_character;
static Value * new_line_specifier;

//std::unique_ptr<legacy::FunctionPassManager> TheFPM;


/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst * CreateEntryBlockAlloca(Function *TheFunction, const std::string &VarName, Type * type)
{
	IRBuilder<> TmpB(&TheFunction -> getEntryBlock(), TheFunction -> getEntryBlock().begin());

	auto init_value = ConstantInt::get(Type::getInt32Ty(TheContext), (type->isArrayTy() ? type->getArrayNumElements() : 0), false);
	return TmpB.CreateAlloca(type, init_value, VarName.c_str());
}



Value * ASTNumber::codegen ()
{
	return ConstantInt::get(TheContext, APInt(32, value, true));
}

Value * ASTString::codegen ()
{
	// return ConstantDataArray::getString(TheContext, str);
	return Builder.CreateGlobalString(StringRef(str));
}

Type * ASTInteger::codegen()
{
	return Type::getInt32Ty(TheContext);
}

Type * ASTArray::codegen ()
{
	Type * elem_type = type -> codegen();
	int size = (upperIdx -> value) - (lowerIdx -> value) + 1;
	return ArrayType::get(elem_type, size);
}

Value * ASTBody::codegen ()
{
	for ( auto & statement : content )
		statement -> codegen();

	return Constant::getNullValue(Type::getInt32Ty(TheContext));
}

// Global Variable declaration
Value * ASTVariable::codegen ()
{
	auto type_value = type -> codegen();
	if ( !type_value )
		return nullptr;

	TheModule -> getOrInsertGlobal(name, type_value);
	GlobalVariable * global_var = TheModule -> getNamedGlobal(name);
	global_var -> setLinkage(GlobalValue::InternalLinkage);

	if ( type_value == Type::getInt32Ty(TheContext) )
		global_var -> setInitializer(ConstantInt::get(TheContext, APInt(32, 0, true)));
	else
		global_var -> setInitializer(ConstantAggregateZero::get(type_value));


	global_vars[name] = std::make_pair(global_var, type);

	return global_vars[name].first;
}
// Const declaration
Value * ASTConstVariable::codegen ()
{
	const_vars[name] = ConstantInt::get(TheContext, APInt(32, value, true));

	return const_vars[name];
}

Value * ASTFunctionCall::codegen ()
{
	if ( name == "writeln" || name == "write" ) {
		if ( arguments.size() == 0 ) {
			if ( name == "writeln" )
				Builder.CreateCall(TheModule -> getFunction("printf"), {new_line_specifier}, "call_printf");
			return nullptr;
		}
		auto value = arguments[0] -> codegen();

		Value * res;
		if ( value -> getType() == Type::getInt32Ty(TheContext))
			res = Builder.CreateCall(TheModule -> getFunction("printf"), {decimal_specifier_character, value}, "call_printf");
		else
			res = Builder.CreateCall(TheModule -> getFunction("printf"), {string_specifier_character, value}, "call_printf");

		if ( name == "writeln" )
			Builder.CreateCall(TheModule -> getFunction("printf"), {new_line_specifier, value}, "call_printf");
		return res;
	} else if ( name == "readln" ) {
		if ( arguments.size() == 0 )
			return nullptr;

		ASTSingleVarReference * var = (ASTSingleVarReference *)arguments[0].get();
		Value * alloca = var -> getAlloca();
		if ( !alloca )
			return nullptr;

		return Builder.CreateCall(TheModule -> getFunction("scanf"), {decimal_specifier_character, alloca}, "call_scanf");
	} else if ( name == "dec" ) {
		if ( arguments.size() == 0 )
			return nullptr;

		ASTSingleVarReference * var = (ASTSingleVarReference *)arguments[0].get();
		Value * alloca = var -> getAlloca();
		if ( !alloca )
			return nullptr;
		Value * current_value = var -> codegen();
		Value * new_value = Builder.CreateSub(current_value, ConstantInt::get(TheContext, APInt(32, 1, true)));

		return Builder.CreateStore(new_value, alloca);
	} else {
			Function *f = TheModule->getFunction(name);
			if ( !f ) {
				printf("Error: Unknown function referenced\n");
				return nullptr;
			}

			if ( f->arg_size() != arguments.size()) {
				printf("Error: Incorrect number of arguments passed\n");
				return NULL;
			}

			// Generate argument expr
			std::vector<Value *> arg_values;
			for ( auto &arg : arguments )
				arg_values.push_back(arg -> codegen());

			return Builder.CreateCall(f, arg_values);
	}
}

Function * ASTFunctionPrototype::codegen ()
{
	std::vector<Type *> param_types;
	for ( auto & param : parameters )
		param_types.push_back(param -> type -> codegen());

	FunctionType * function_type;
	if ( returnType ) // Function
		function_type = FunctionType::get(returnType -> codegen(), param_types, false);
	else  // Procedure
		function_type = FunctionType::get(Type::getVoidTy(TheContext), param_types, false);

	Function * function = Function::Create(function_type, Function::ExternalLinkage, name, TheModule.get());

	// Set names for arguments to match prototype parameters
	unsigned i = 0;
	for ( auto & param : function -> args() )
		param.setName(parameters[i++] -> name);

	return function;
}

Function * ASTFunction::codegen ()
{
	// Lookup function declaration
	Function * function = TheModule -> getFunction(prototype -> getName());
	if ( !function ) // Not yet generated
		function = prototype -> codegen();
	if ( !function || !body )
		return nullptr;

	// Create a new basic block to start insertion into.
	BasicBlock * function_BB = BasicBlock::Create(TheContext, "function_block: " + prototype -> getName(), function);
	BasicBlock * return_BB = BasicBlock::Create(TheContext, "return_block: " + prototype -> getName(), function);

	Builder.SetInsertPoint(function_BB);

	// Remember the old variable binding so that we can restore the binding when
	// we unrecurse.
	std::map<std::string, std::pair<AllocaInst *, std::shared_ptr<ASTVariableType>>> old_named_values (named_values);

	// Save function arguments so they can be used as local variables
	int idx = 0;
	for ( auto & arg : function -> args() ) {
		// TODO maybe use name and type from prototype
		// Create an alloca for arg
		AllocaInst * alloca = CreateEntryBlockAlloca(function, arg.getName(), arg.getType());
		// Store arg into the alloca.
		Builder.CreateStore(&arg, alloca);
		// Add arguments to variable symbol table.
		named_values[arg.getName()] = std::make_pair(alloca, prototype -> parameters[idx++] -> type);
	}

	// Local variables
	for ( auto & var : local_variables ) {
		auto type_value = var -> type -> codegen();
		AllocaInst * alloca = CreateEntryBlockAlloca(function, var -> name, type_value);
		// Builder.CreateStore(ConstantInt::get(TheContext, APInt(32, 0, true)), alloca);  // TODO not for arrays
		named_values[var -> name] = std::make_pair(alloca, var -> type);
	}

	// Return variable for functions
	if ( prototype -> returnType ) {
		AllocaInst * alloca = CreateEntryBlockAlloca(function, prototype -> getName(), prototype -> returnType -> codegen());
		// Builder.CreateStore(ConstantInt::get(TheContext, APInt(32, 0, true)), alloca);  // TODO not for arrays
		named_values[prototype -> getName()] = std::make_pair(alloca, prototype -> returnType);;
	}


	//Builder.SetInsertPoint(function_BB);

	auto body_value = body -> codegen();
	if ( !body_value )
		return nullptr;


	Builder.CreateBr(return_BB);
	Builder.SetInsertPoint(return_BB);

	if ( prototype -> returnType ) {
		auto return_value = Builder.CreateLoad(named_values[prototype -> getName()].first);
		Builder.CreateRet(return_value);
	} else {
		Builder.CreateRetVoid();
	}



	assert(!verifyFunction(*function, &errs()));
	//TheFPM -> run(*function);

	// Unrecurse
	named_values = old_named_values;

	return function;
}



Value * ASTIf::codegen ()
{
	Value * condition_value = condition -> codegen();
	if ( !condition_value )
		return nullptr;

	condition_value = Builder.CreateICmpNE(condition_value , ConstantInt::get(TheContext, APInt(32, 0, true)), "if_condition");

	Function * parent = Builder.GetInsertBlock() -> getParent();

	// Create blocks for the then and else cases.  Insert the 'then' block at the
	// end of the function.
	BasicBlock * then_BB = BasicBlock::Create(TheContext, "then", parent);
	BasicBlock * else_BB = BasicBlock::Create(TheContext, "else", parent);
	BasicBlock * after_BB = BasicBlock::Create(TheContext, "after_block", parent);

	Builder.CreateCondBr(condition_value, then_BB, else_BB);


	// Then body
	Builder.SetInsertPoint(then_BB);
	Value * then_value = then_body -> codegen();
	if ( !then_value )
		return nullptr;
	Builder.CreateBr(after_BB);


	// Else body
	Builder.SetInsertPoint(else_BB);
	Value * else_value = nullptr;
	if ( else_body ) {
		else_value = else_body -> codegen();
		if ( !else_value )
			return nullptr;
	}
	Builder.CreateBr(after_BB);


	Builder.SetInsertPoint(after_BB);

	return Constant::getNullValue(Type::getInt32Ty(TheContext));


/*	PHINode * phi_node = Builder.CreatePHI(Type::getInt32Ty(TheContext), 2, "iftmp");

	phi_node -> addIncoming(then_value, then_BB);
	phi_node -> addIncoming(else_value, else_BB);
	return phi_node; */
}

Value * ASTFor::codegen()
{
	// Search for control_variable
	TVarInfo info;
	if ( named_values.find(variable_name) != named_values.cend() )
		info = named_values[variable_name];
	else if ( global_vars.find(variable_name) != global_vars.cend() )
		info = global_vars[variable_name];
	else
		throw "Using an undeclared variable in for statement";

	// Create an alloca for the variable in the entry block.
	//AllocaInst * Alloca = CreateEntryBlockAlloca(parent, variable_name, Type::getInt32Ty(TheContext));

	// Codegen start value
	Value * start_value = start -> codegen();
	if ( !start_value )
		return nullptr;
	// Codegen end value
	Value * end_value = end -> codegen();
	if ( !end_value )
		return nullptr;

	// Store the value into the alloca.
	Builder.CreateStore(start_value, info.first);

	Function * parent = Builder.GetInsertBlock() -> getParent();
	BasicBlock * condition_BB = BasicBlock::Create(TheContext, "for_condition", parent);
	BasicBlock * body_BB = BasicBlock::Create(TheContext, "for_lopp", parent);
	BasicBlock * after_BB = BasicBlock::Create(TheContext, "after_block", parent);

	// Condition
	Builder.CreateBr(condition_BB);
	Builder.SetInsertPoint(condition_BB);
	Value * for_condition;
	if ( downto )
		for_condition = Builder.CreateICmpSGE(Builder.CreateLoad(info.first, variable_name), end_value);
	else
		for_condition = Builder.CreateICmpSLE(Builder.CreateLoad(info.first, variable_name), end_value);

	Builder.CreateCondBr(for_condition, body_BB, after_BB);

	// Loop branch
	Builder.SetInsertPoint(body_BB);

	if ( !body -> codegen() )
		return nullptr;

	// Calculate Next Value
	Value * current_value = Builder.CreateLoad(info.first, variable_name);
	Value * next_value = nullptr;
	Value * step_value = ConstantInt::get(TheContext, APInt(32, 1, true));
	if ( downto )
		next_value = Builder.CreateSub(current_value, step_value, "next_value");
	else
		next_value = Builder.CreateAdd(current_value, step_value, "next_value");

	// Save to alloca
	Builder.CreateStore(next_value, info.first);

	Builder.CreateBr(condition_BB);



	// After for cycle
	Builder.SetInsertPoint(after_BB);

	// for expr always returns 0
	return Constant::getNullValue(Type::getInt32Ty(TheContext));
}

Value * ASTWhile::codegen()
{
	Function * parent = Builder.GetInsertBlock() -> getParent();
	BasicBlock * condition_BB = BasicBlock::Create(TheContext, "while_condition", parent);
	BasicBlock * body_BB = BasicBlock::Create(TheContext, "while_loop", parent);
	BasicBlock * after_BB = BasicBlock::Create(TheContext, "after_block", parent);

	// Condition
	Builder.CreateBr(condition_BB);
	Builder.SetInsertPoint(condition_BB);

	Value * while_condition = condition -> codegen();
	if ( !while_condition )
		return nullptr;

	auto res = Builder.CreateICmpNE(while_condition, ConstantInt::get(TheContext, APInt(32, 0, true)));

	Builder.CreateCondBr(res, body_BB, after_BB);


	// Body
	Builder.SetInsertPoint(body_BB);
	if ( !(body -> codegen()) )
		return nullptr;
	Builder.CreateBr(condition_BB);



	// After while loop
	Builder.SetInsertPoint(after_BB);

	// while expression always returns 0.
	return Constant::getNullValue(Type::getInt32Ty(TheContext));
}


Value * ASTBreak::codegen ()
{
	auto parent = Builder.GetInsertBlock() -> getParent();
	BasicBlock * after_BB = nullptr;

	for ( auto & b : parent -> getBasicBlockList() ) {
		if ( b.getName().startswith("after_block") ) {
			after_BB = &b;
			break;
		}
	}
	if ( !after_BB )
		return nullptr;

	Builder.CreateBr(after_BB);

	auto break_BB = BasicBlock::Create(TheContext, "after_break", parent);
	Builder.SetInsertPoint(break_BB);

	return Constant::getNullValue(Type::getInt32Ty(TheContext));
}

Value * ASTExit::codegen ()
{
	Function * parent = Builder.GetInsertBlock() -> getParent();

	BasicBlock * function_return_BB = nullptr;

	for ( BasicBlock & b : parent -> getBasicBlockList() ) {
		if ( b.getName().startswith("return_block") ) {
			function_return_BB = &b;
			break;
		}
	}
	if ( !function_return_BB )
		return nullptr;


	Builder.CreateBr(function_return_BB);

	auto exit_BB = BasicBlock::Create(TheContext, "after_exit", parent);
	Builder.SetInsertPoint(exit_BB);

	return Constant::getNullValue(Type::getInt32Ty(TheContext));
}


// Get variable value from stack
Value * ASTSingleVarReference::codegen ()
{
	if ( const_vars.find(name) != const_vars.cend() )
		return const_vars[name];

	// Search for var
	TVarInfo info;

	if ( named_values.find(name) != named_values.cend() )
		info = named_values[name];
	else if ( global_vars.find(name) != global_vars.cend() )
		info = global_vars[name];
	else
		throw "Using an undeclared variable";

	return Builder.CreateLoad(info.first, name);
}
// Find variable address
Value * ASTSingleVarReference::getAlloca()
{
	TVarInfo info;
	if ( named_values.find(name) != named_values.cend() )
		info = named_values[name];
	else if ( global_vars.find(name) != global_vars.cend() )
		info = global_vars[name];

	return info.first;
}

// Get array elem value from stack
Value * ASTArrayReference::codegen ()
{
	return Builder.CreateLoad(getAlloca());
}
// Find array elem address
Value * ASTArrayReference::getAlloca()
{
	// Search for var
	TVarInfo info;
	if ( named_values.find(name) != named_values.cend() )
		info = named_values[name];
	else if ( global_vars.find(name) != global_vars.cend() )
		info = global_vars[name];
	else
		throw "Undeclared array variable";

	// Calculating elem address
	std::vector<Value *> idx_list;
	auto start_idx = std::dynamic_pointer_cast<ASTArray>(info.second) -> lowerIdx -> codegen();
	auto idx = Builder.CreateSub(index -> codegen(), start_idx);

	idx_list.push_back(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
	idx_list.push_back(idx);

	return Builder.CreateGEP(info.first, idx_list);
}

Value * ASTAssignOp::codegen ()
{
	// Find alloca address of left side
	Value * alloca = variable -> getAlloca();
	if ( !alloca )
		return nullptr;

	Value * new_value = value -> codegen();
	if ( !new_value )
		return nullptr;

	Builder.CreateStore(new_value, alloca);

	return new_value;
}


Value * ASTBinaryOperator::codegen ()
{
	Value * left = LHS -> codegen();
	Value * right = RHS -> codegen();

	if ( !left || !right )
		return nullptr;

	Value * bit_result;
	switch ( op ) {
		case tok_plus:
			bit_result = Builder.CreateAdd(left, right, "add");
			break;
		case tok_minus:
			bit_result = Builder.CreateSub(left, right, "sub");
			break;
		case tok_multiply:
			bit_result = Builder.CreateMul(left, right, "mul");
			break;
		case tok_kwDiv:
			bit_result = Builder.CreateSDiv(left, right, "div");
			break;
		case tok_kwMod:
			bit_result = Builder.CreateSRem(left, right, "mod");
			break;
		case tok_equal:
			bit_result = Builder.CreateICmpEQ(left, right, "eq");
			break;
		case tok_notEqual:
			bit_result = Builder.CreateICmpNE(left, right, "neq");
			break;
		case tok_less:
			bit_result = Builder.CreateICmpSLT(left, right, "less");
			break;
		case tok_lessEqual:
			bit_result = Builder.CreateICmpSLE(left, right, "lessEq");
			break;
		case tok_greater:
			bit_result = Builder.CreateICmpSGT(left, right, "greater");
			break;
		case tok_greaterEqual:
			bit_result = Builder.CreateICmpSGE(left, right, "greaterEq");
			break;
		case tok_kwAnd:
			bit_result = Builder.CreateAnd(left, right, "and");
			break;
		case tok_kwOr:
			bit_result = Builder.CreateOr(left, right, "or");
			break;
		default:
			bit_result = nullptr;
			break;
	}
	return Builder.CreateIntCast(bit_result, Type::getInt32Ty(TheContext), true);
}



Value * ASTProgram::codegen ()
{
	// Printf and scanf declarations
	PointerType * ptr = PointerType::get(IntegerType::get(TheContext, 8), 0);
	FunctionType * function_type = FunctionType::get(IntegerType::get(TheContext, 32), ptr, true);
	Function * llvm_printf = Function::Create(function_type, Function::ExternalLinkage, "printf", TheModule.get());
	llvm_printf -> setCallingConv(CallingConv::C);

	Function * llvm_scanf = Function::Create(function_type, Function::ExternalLinkage, "scanf", TheModule.get());
	llvm_scanf -> setCallingConv(CallingConv::C);

	function_type = FunctionType::get(IntegerType::getInt32Ty(TheContext), {}, false);

	Function * program_func = Function::Create(function_type, Function::ExternalLinkage, "main", TheModule.get());
	program_func -> setCallingConv(CallingConv::C);

	BasicBlock * program_BB = BasicBlock::Create(TheContext, "main_BB", program_func);
	//Builder.CreateBr(program_BB);
	Builder.SetInsertPoint(program_BB);


	decimal_specifier_character = Builder.CreateGlobalStringPtr("%d");
	string_specifier_character = Builder.CreateGlobalStringPtr("%s");
	new_line_specifier = Builder.CreateGlobalStringPtr("\n");


	for ( auto & var : global )
		var -> codegen();

	for ( auto & f : functions )
		f -> codegen();

	Builder.SetInsertPoint(program_BB);
	auto v = main -> codegen();
	if ( v )
		return Builder.CreateRet(ConstantInt::get(TheContext, APInt(32, 0, false)));
	else
		return Builder.CreateRet(ConstantInt::get(TheContext, APInt(32, 1, false)));
}


// Does the magic
std::unique_ptr<Module> ASTProgram::runCodegen(const std::string & output_file)
{
	// Create a new pass manager attached to it.
	//	TheFPM = llvm::make_unique<legacy::FunctionPassManager>(TheModule.get());
	/*
	// Promote allocas to registers.
	TheFPM->add(createPromoteMemoryToRegisterPass());
	// Do simple "peephole" optimizations and bit-twiddling optzns.
	TheFPM->add(createInstructionCombiningPass());
	// Reassociate expressions.
	TheFPM->add(createReassociatePass());
	// Eliminate Common SubExpressions.
	TheFPM->add(createGVNPass());
	// Simplify the control flow graph (deleting unreachable blocks, etc).
	TheFPM->add(createCFGSimplificationPass());

	TheFPM->doInitialization();*/

	TheModule = make_unique<Module>("main_module", TheContext);

	auto res = codegen();


	if ( !res )
		return nullptr;

	// llvm::outs() raw_ostream for std::cout
	TheModule -> print(outs(), nullptr);



	// GENERATE OBJECT FILE
	// Initialize the target registry etc.
	InitializeAllTargetInfos();
	InitializeAllTargets();
	InitializeAllTargetMCs();
	InitializeAllAsmParsers();
	InitializeAllAsmPrinters();

	auto TargetTriple = sys::getDefaultTargetTriple();
	TheModule->setTargetTriple(TargetTriple);

	std::string Error;
	auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

	// Print an error and exit if we couldn't find the requested target.
	// This generally occurs if we've forgotten to initialise the
	// TargetRegistry or we have a bogus target triple.
	if (!Target) {
		errs() << Error;
		return nullptr;
	}

	auto CPU = "generic";
	auto Features = "";

	TargetOptions opt;
	auto RM = Optional<Reloc::Model>();
	auto TheTargetMachine =	Target -> createTargetMachine(TargetTriple, CPU, Features, opt, RM);

	TheModule -> setDataLayout(TheTargetMachine->createDataLayout());

	std::error_code EC;
	raw_fd_ostream dest(output_file, EC, sys::fs::F_None);

	if (EC) {
		errs() << "Could not open file: " << EC.message();
		return nullptr;
	}

	legacy::PassManager pass;
	auto file_type = TargetMachine::CGFT_ObjectFile;

	if (TheTargetMachine -> addPassesToEmitFile(pass, dest, file_type)) {
		errs() << "TheTargetMachine can't emit a file of this type";
		return nullptr;
	}

	assert(!verifyModule(*TheModule, &errs()));

	pass.run(*TheModule);

	dest.flush();

	outs() << "Wrote " << output_file << "\n";

	return nullptr;
}



// PROCEDURE
/*Value * ASTProcedureCall::codegen ()
{
	if ( name == "writeln" ) {

	} else if ( name == "readln" ) {

	} else {
		Function * f = TheModule -> getFunction(this -> name);
		if ( !f ) {
			printf("Error: Unknown procedure referenced\n");
			return NULL;
		}

		if ( f -> arg_size() != this -> arguments.size()) {
			printf("Error: Incorrect number of arguments passed\n");
			return NULL;
		}

		std::vector<Value *> arg_values;
		for ( auto & arg : this -> arguments ) {
			arg_values.push_back(arg -> codegen());
		}

		return Builder.CreateCall(f, arg_values, "call_procedure");
	}
}*/
/*Function * ASTProcedurePrototype::codegen ()
{
	std::vector<Type *> param_types;
	for ( auto & param : this -> parameters )
		param_types.push_back(param -> type -> codegen());

	FunctionType * procedure_type = FunctionType::get(Type::getVoidTy(TheContext), param_types, false);
	Function * procedure = Function::Create(procedure_type, Function::ExternalLinkage, this -> name, TheModule.get());

	unsigned i = 0;
	for ( auto & arg : procedure -> args() )
		arg.setName(this -> parameters[i++] -> name);

	return procedure;
}*/
/*Function * ASTProcedure::codegen ()
{

}*/

