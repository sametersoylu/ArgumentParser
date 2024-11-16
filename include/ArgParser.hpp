#pragma once

#include <exception>
#include <string>
#include <functional>
#include <sstream>

namespace ArgParser {
    namespace ArgumentFunction {
        template<typename Fn>
        concept NoParam = std::is_invocable_r_v<void, Fn>; 
        
        template<typename Fn, typename ParamType>
        concept WithParam = std::is_invocable_r_v<void, Fn, ParamType const&>; 

        template<typename Fn>
        concept WithString = WithParam<Fn, std::string>; 

        template<typename Fn>
        concept WithInteger = WithParam<Fn, std::string>; 
        
        template<typename Fn>
        concept WithFloat = WithParam<Fn, std::string>; 
    };

    

    enum class ErrorType {
        MissingValue,
        KeyExists,
    }; 

    class ParseArgument {
        public: 
        ParseArgument(std::string const& arg_prec = "-");

        void SetHelp(std::string const& help);

        void SetErrorText(ErrorType type, std::string const& error_message);

        template<ArgumentFunction::NoParam Fn>
        void AddArgument(std::string const& argument_short, std::string const& argument_long, Fn const& function) {
            FailIfExists({argument_short, argument_long});
            user_defined_arg_list.emplace_back(argument_short, argument_long, new NoParamHandler(function), false);

            prepare_usage_text(argument_short, argument_long); 

            if(not default_help_text) return; 

            prepare_help_text_from_long_key(argument_short, argument_long); 
        }

        template<typename ParamType = std::string, ArgumentFunction::WithParam<ParamType> Fn>
        void AddArgument(std::string const& argument_short, std::string const& argument_long, Fn const& function) {
            FailIfExists({argument_short, argument_long});
            user_defined_arg_list.emplace_back(argument_short, argument_long, new ParamHandler<ParamType>(function), true);

            prepare_usage_text_with_value(argument_short, argument_long); 

            if(not default_help_text) return; 

            prepare_help_text_from_long_key(argument_short, argument_long, true); 
        }

        template<ArgumentFunction::NoParam Fn>
        void AddArgument(std::string const& argument_short, std::string const& argument_long, std::string const& help, Fn const& function) {
            FailIfExists({argument_short, argument_long});
            user_defined_arg_list.emplace_back(argument_short, argument_long, new NoParamHandler(function), false);

            prepare_usage_text(argument_short, argument_long); 

            if(not default_help_text) return; 

            help_text << arg_prec << argument_short << ", " << arg_prec << arg_prec << argument_long << " : " << help << "\n";
        }

        template<typename ParamType = std::string, ArgumentFunction::WithParam<ParamType> Fn>
        void AddArgument(std::string const& argument_short, std::string const& argument_long, std::string const& help, Fn const& function) {
            FailIfExists({argument_short, argument_long});
            user_defined_arg_list.emplace_back(argument_short, argument_long, new ParamHandler<ParamType>(function), true);

            prepare_usage_text_with_value(argument_short, argument_long); 

            if(not default_help_text) return; 

            help_text << arg_prec << argument_short << " <value>" << ", " << arg_prec << arg_prec << argument_long << " <value>" << " : " << help << "\n";
        }

        void HandleArguments();

        private:
        std::pair<std::string, std::string> split(std::string const& str, char delim = '=');
        std::pair<bool, bool> IsKeyExists(std::pair<std::string, std::string> const& keys);
        void FailIfExists(std::pair<std::string, std::string> const& keys) {
            auto [se, le] = IsKeyExists(keys);
            if(se) {
                throw KeyExists(keys.first + Errors.key_exists); 
            }
            if(le) {
                throw KeyExists(keys.second + Errors.key_exists); 
            }
        }

        class BaseHandler {
            protected:
            std::string __type;
            public: 
            std::string GetType() {
                return __type; 
            }
        };
        class NoParamHandler : public BaseHandler {
            private:
            std::function<void()> handler;
            public: 
            NoParamHandler(std::function<void()> function) : handler(function) {}
            void operator()() {
                handler(); 
            }
            void Invoke() {
                handler(); 
            }
        }; 

        template<typename T_>
        class ParamHandler : public BaseHandler {
            private:
            std::function<void(T_ const&)> handler;
            public: 
            ParamHandler(std::function<void(T_ const&)> function) : handler(function) {
                if constexpr (std::is_same_v<T_, int>) {
                    BaseHandler::__type = "int"; 
                }
                else if constexpr (std::is_same_v<T_, float>) {
                    BaseHandler::__type = "float"; 
                }
                else if constexpr (std::is_same_v<T_, std::string>) {
                    BaseHandler::__type = "string"; 
                } 
                else if constexpr (std::is_same_v<T_, bool>) {
                    BaseHandler::__type = "bool";
                }
                else {
                    static_assert(false, "Unsupported type.");
                }
            }
            void operator()(T_ const& val) {
                handler(val); 
            }
            void Invoke(T_ const& val) {
                handler(val); 
            }
            
        }; 
    
        struct Name {
            public:
            std::string _short; 
            std::string _long;
            bool does_match(std::string const& val, std::string const& arg_prec) {
                if (val == (arg_prec) + _short) {
                    matched = _short; 
                    return true; 
                }
                if (val == (arg_prec + arg_prec) + _long) {
                    matched = _long; 
                    return true; 
                }
                matched.clear(); 
                return false; 
            }
            std::string get_matched() const {
                return matched; 
            }
            private:
            std::string matched; 
        };
        struct Data {
            std::string value; 
            bool has_value = false; 
        };
        struct Keys {
            Name name; 
            Data data;   
        };
        struct ArgumentType {
            Keys keys; 
            BaseHandler* handler;
            ArgumentType() = default; 
            ArgumentType(std::string const& _short, std::string const& _long, BaseHandler * _handler, bool has_value = false) {
                keys.name._short = _short; 
                keys.name._long = _long;
                handler = _handler; 
                keys.data.has_value = has_value; 
            }
        };
        class BaseError : public std::exception {
            private:
            std::string msg; 
            public: 
            BaseError(const std::string& message = "Base error.") : msg(message) {}
            const char * what()  const noexcept override {
                return msg.c_str(); 
            }
        };

        class MissingValue : public BaseError {
            public:
            MissingValue(std::string const& msg) : BaseError(msg) {}
        }; 
        class KeyExists : public BaseError {
            public:
            KeyExists(std::string const& msg) : BaseError(msg) {}
        }; 
        struct {
            std::string missing_value = "A value must be supplied."; 
            std::string key_exists = " already exists.";
        } Errors; 

        void help();
        void prepare_help_text_from_long_key(std::string const& argument_short, std::string const& argument_long, bool has_value = false);
        void prepare_usage_text(std::string const& argument_short, std::string const& argument_long) {
            usage << "[" << arg_prec << argument_short << " | " << (arg_prec + arg_prec) << argument_long << "]";
        }
        void prepare_usage_text_with_value(std::string const& argument_short, std::string const& argument_long) {
            usage << "[" << arg_prec << argument_short << " <value> " << " | " << (arg_prec + arg_prec) << argument_long << " <value>" << "]";
        }

        std::vector<ArgumentType> user_defined_arg_list; 
        std::vector<std::string> arg_list;
        std::stringstream help_text;
        std::stringstream usage; 
        bool default_help_text = true;
        std::string arg_prec;
        std::string program_name; 
    }; 

}; 