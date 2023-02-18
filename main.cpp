#include "Parser.h"

#include <iostream>
#include <fstream>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"



int main(int argc, char * argv[])
{
    std::string input_file;
    std::string output_file = "output.o";
    if ( argc != 2 ) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    input_file = argv[1];

    Parser parser(input_file);

    try {
        std::unique_ptr<ASTProgram> parsed_program(parser.start());

        parsed_program -> runCodegen(output_file);

    } catch (const char * exception) {
        printf("Error while compiling %s\n", input_file.c_str());
        printf("%s\n", exception);
        return 2;
    }

    return 0;
}
