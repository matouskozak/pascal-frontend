# Pascal frontend compiler
Frontend analyzer for Pascal source codes. The first part is the Lexer/Lexan responsible for lexical analysis of the given input source code, i.e., breaking the source code into lexical tokens. The second part is the syntax analysis in which the lexical tokens are parsed into the abstract syntax tree (AST). Lastly, the AST is transformed into LLVM intermediate representation (IR) and executable code is generated.

Semester work for BI-PJP at CTU in Prague.

## REQUIREMENTS 
LLVM 6.0.0
make
clang

## HOW_TO_USE
0. run cmake ./ (creates makefile)
1. run make	
2. ./pas_compiler "path_to_source_file"
3. clang output.o
4. ./a.out	
	
