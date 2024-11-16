#include "../include/ArgParser.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

using namespace ArgParser;

#ifdef __linux__ 
#include <fstream> 
ParseArgument::ParseArgument(std::string const& arg_prec) : arg_prec(arg_prec) {
    AddArgument("h", "help", "Prints the help text.", [&]() {
        help();
    });
    std::ifstream cmdlineFile("/proc/self/cmdline");
    std::getline(cmdlineFile, program_name, '\0'); 
    for(std::string line; std::getline(cmdlineFile, line, '\0');) {
        arg_list.emplace_back(line);
    }
}
#elif _WIN32
#include <windows.h>
#include <shellapi.h>
std::string wstring_to_string(const std::wstring& str) {
    return {str.begin(), str.end()};
}

ParseArgument::ParseArgument(std::string const& arg_prec) : arg_prec(arg_prec) {
    AddArgument("h", "help", "Prints the help text.", [&]() {
        help();
    });

    int argc = 0; 
    LPWSTR * argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    program_name = wstring_to_string(std::wstring(argv[0]));
    
    for(int i = 1; i < argc; i++) {
        arg_list.emplace_back(wstring_to_string(std::wstring(argv[i]))); 
    }
}
#else 
    static_assert(false, "Unsupported OS!"); 
#endif

void ParseArgument::SetHelp(std::string const& help) {
    help_text.clear(); 
    help_text << help;
    default_help_text = false;
}

void ParseArgument::SetErrorText(ErrorType type, std::string const& error_message) {
    switch(type) {
        case ErrorType::MissingValue:
            Errors.missing_value = error_message; 
        break;
        case ErrorType::KeyExists:
            Errors.key_exists = error_message; 
        break;
    }
}

void ParseArgument::HandleArguments() {
    if(arg_list.empty()) {
        return; 
    }
    
    for(auto it = arg_list.begin() + 1; it != arg_list.end(); it++) {
        if(*it == arg_prec + "h" or *it == (arg_prec + arg_prec) + "help") {
            help(); 
        }
    }

    for(auto it = arg_list.begin(); it != arg_list.end(); it++) {
        for(auto &[key, handler] : user_defined_arg_list) {
            auto& [names, data] = key; 

            if(not names.does_match(*it, arg_prec)) {
                continue; 
            }

            if(data.has_value) {
                if(it == arg_list.end()) {        
                    std::cerr << Errors.missing_value << std::endl; 
                    exit(1);
                }

                data.value = *(++it);
                auto type = handler->GetType(); 

                if (type == "int") {
                    std::for_each(data.value.begin(), data.value.end(), [names](char ch) {
                        if (not std::isdigit(ch)) {
                            std::cerr << "For given argument \"" << names.get_matched() << "\" expected value type was integer." << std::endl;  
                            exit(1); 
                        }
                    }); 
                    static_cast<ParamHandler<int>*>(handler)->Invoke(std::stoi(data.value));
                    break;
                }

                if (type == "float") {
                    bool dot_found = false; 
                    std::for_each(data.value.begin(), data.value.end(), [&dot_found, names](char ch) {
                        if (std::isdigit(ch)) {
                            return; 
                        }
                        if (ch == '.') {
                            if (dot_found) {
                                std::cerr << "For given argument \"" << names.get_matched() << "\" expected value type was float. But found multiple points." << std::endl;  
                                exit(1); 
                            }
                            dot_found = true; 
                            return; 
                        }
                        std::cerr << "For given argument \"" << names.get_matched() << "\" expected value type was float." << std::endl;  
                        exit(1); 
                    });
                    static_cast<ParamHandler<float>*>(handler)->Invoke(std::stof(data.value));
                    break;
                }

                if (type == "bool") {
                    std::array<std::string, 6> valid_responses{"true", "false", "on", "off", "1", "0"};
                    bool valid = false;
                    for(auto valid_response : valid_responses) {
                        if (data.value == valid_response) {
                            valid = true;
                            if (valid_response == "true" or valid_response == "on" or valid_response == "1") {
                                static_cast<ParamHandler<bool>*>(handler)->Invoke(true);
                            } else {
                                static_cast<ParamHandler<bool>*>(handler)->Invoke(false);
                            }
                            break;
                        }
                    }
                    
                    if (not valid) {
                        std::cerr << "For given argument \"" << names.get_matched() << "\" expected value type was boolean (accepted: true/false, on/off, 1/0)" << std::endl;
                        exit(1);
                    }

                    break;
                }
                if (type == "string") {
                    static_cast<ParamHandler<std::string>*>(handler)->Invoke(data.value);
                    break;
                }
                
            }
            static_cast<NoParamHandler*>(handler)->Invoke();
        }
    }
}

std::pair<std::string, std::string> ParseArgument::split(std::string const& str, char delim) {
    auto pos = str.find(delim); 
    if(pos == std::string::npos) {
        throw MissingValue(Errors.missing_value); 
    }
    return {str.substr(0, pos), str.substr(pos + 1)}; 
}


std::pair<bool, bool> ParseArgument::IsKeyExists(std::pair<std::string, std::string> const& keys) {
    for(auto &[arg_keys, arg_handler] : user_defined_arg_list) {
        if(arg_keys.name._short == keys.first or arg_keys.name._long == keys.second) {
            return {arg_keys.name._short == keys.first, arg_keys.name._long == keys.second};
        }
    }
    return {false, false}; 
}

void ParseArgument::help() {
    std::cout << "Usage: " << program_name << " " << usage.str() << std::endl; 
    std::cout << "Options:" << std::endl; 
    for(std::string line; std::getline(help_text, line);) {
        std::cout << "  " << line << std::endl; 
    }
    exit(0);
}

void ParseArgument::prepare_help_text_from_long_key(std::string const& argument_short, std::string const& argument_long, bool has_value) {
    help_text << arg_prec << argument_short << (has_value ? " <value>" : "") << ", " << arg_prec << arg_prec << argument_long << (has_value ? " <value>" : "") << " : ";

    for(auto ch : argument_long) {
        if(ch == '-' or ch == '_') { help_text << " "; continue; }
        help_text << (char)std::toupper(ch);
    }

    help_text << "\n"; 
}