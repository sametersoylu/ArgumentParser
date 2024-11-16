#include "../include/ArgParser.hpp"

#include <iostream>

enum Op {
    none,
    add,
    subtract,
    multiply,
    divide
};

void error(std::string const& text, int exit_code = 1) {
    std::cerr << text << std::endl; 
    exit(exit_code);
}

void error_if(bool condition, std::string const& text, int exit_code = 1) {
    if (condition) error(text, exit_code);
}

int main() {
    ArgParser::ParseArgument parser;
    
    parser.AddArgument("v", "version", "prints version", []() {
        std::cout << "0.0.1" << std::endl; 
    });

    parser.AddArgument<std::string>("e", "echo", "echoes the value back", [](std::string const& val) {
        std::cout << val << std::endl;
    });
    
    Op op = none; 
    
    parser.AddArgument("a", "add", "set flag to add two numbers", [&]() { op = add; });
    parser.AddArgument("s", "subtract", "set flag to subtract two numbers", [&]() { op = subtract; });
    parser.AddArgument("m", "multiply", "set flag to multiply two numbers", [&]() { op = multiply; });
    parser.AddArgument("d", "divide", "set flag to divide two numbers", [&]() { op = divide; });
    
    std::array<std::pair<int, bool>, 2> numbers{};

    parser.AddArgument<int>("f_n", "first_number", "set the first number for operation", [&numbers](int num) {
        numbers[0] = {num, true}; 
    });
    
    parser.AddArgument<int>("s_n", "second_number", "set the second number for operation", [&numbers](int num) {
        numbers[1] = {num, true}; 
    });

    parser.AddArgument("ex", "execute", "execute the operation", [&]() {
        error_if(op == none, "You didn't set operation type. Please use one of the operation flags.");
        auto [x, y] = numbers; 
        error_if(not x.second, "You didn't set first number. Please use -f_n or --first_number to set first number.");
        error_if(not y.second, "You didn't set second number. Please use -s_n or --second_number to set second number.");

        switch(op) {
            case add: 
                std::cout << x.first + y.first << std::endl; 
                break; 
            case subtract: 
                std::cout << x.first - y.first << std::endl; 
                break;
            case multiply: 
                std::cout << x.first * y.first << std::endl; 
                break;
            case divide:
                error_if(y.first == 0, "Divider cannot be 0.");
                std::cout << x.first / y.first << std::endl; 
                break;
        }
    });

    parser.HandleArguments();
}