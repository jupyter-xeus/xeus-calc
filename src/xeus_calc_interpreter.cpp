#include <iostream>
#include <vector>
#include <sstream>
#include <stack>
#include <cctype>

#include "xeus/xinterpreter.hpp"

#include "xeus_calc_interpreter.hpp"

namespace xeus_calc
{
    void interpreter::configure_impl()
    {
    }

    std::string formating_expr(const std::string& expr)
    {
        std::string operators = "-+/*^()";
        std::string spaced_expr;
        for (char itr : expr)
        {
            std::istringstream num(std::to_string(itr));
            size_t op = operators.find(itr);
            if(op != std::string::npos)
            {
                spaced_expr += ' ';
                spaced_expr += itr;
                spaced_expr += ' ';
            }
            else if (isdigit(itr) || isdigit(spaced_expr.back()) && itr == '.')
            {
                spaced_expr += itr;
            }
            else if (itr == ' ')
            {
                continue;
            }
            else
            {
                std::string s = "Syntax error :\none of the characters presents an issue : ";
                s.push_back(itr);
                throw std::runtime_error(s);
            }
        }
        return spaced_expr;
    }

    std::string parse_rpn(const std::string& infix, publish_type publish)
    {
        const std::string ops = "-+/*^";
        std::stringstream parsed_infix;
        std::size_t parenthesis_counter = 0; // used for error management purpose

        std::stack<int> operators;

        std::stringstream input(infix);
        std::string token;
        while (std::getline(input, token, ' '))
        {
            if (token.empty())
            {
                continue;
            }

            char c = token[0]; // checks the first character of the token at each loop
            size_t idx = ops.find(c); // Position in the string

            // check for operator
            if (idx != std::string::npos)
            {
                while (!operators.empty())
                {
                    int prec2 = operators.top() / 2; // Integer division to have the left to right priority when similar operators (* / or + -)
                    int prec1 = idx / 2;
                    if (prec2 > prec1 || (prec2 == prec1 && c != '^')) // higher or equivalent position value in the string -> higher priority, exception for ^
                    {
                        parsed_infix << ops[operators.top()] << ' ';
                        operators.pop();
                    }
                    else break;
                }
                operators.push(idx); // after checking previous operator we can push in the stack
            }
            else if (c == '(')
            {
                ++ parenthesis_counter;
                operators.push(-2); // -2 stands for '('
            }
            else if (c == ')')
            {
                if (parenthesis_counter > 0)
                {
                    -- parenthesis_counter;
                    // until '(' on stack, pop operators.
                    while (operators.top() != -2)
                    {
                        parsed_infix << ops[operators.top()] << ' ';
                        operators.pop();
                    }
                    operators.pop();
                    continue;
                }
                else
                {
                    throw std::runtime_error("Syntax error :\nmissing or misplaced parenthesis");
                }
            }
            else
            {
                // if the first value of the token is the number, we can assume that the whole token is composed of numbers and thus push it
                parsed_infix << token << ' ';
            }
        }

        while (!operators.empty()) // push the remaining operators in the sting stream
        {
            parsed_infix << ops[operators.top()] << ' ';
            operators.pop();
        }
        if (parenthesis_counter == 0)
        {
            publish("stdout", "RPN = " + parsed_infix.str());
            return parsed_infix.str();
        }
        else
        {
            throw std::runtime_error("Syntax error :\nmissing or misplaced parenthesis");
        }
    }

    double compute_rpn(const std::string &expr, publish_type publish)
    {
        publish("stdout", "\nInput\tOperation\tStack after\n");
        std::istringstream input(expr);
        std::vector<double> evaluation;
        std::string token;
        while (input >> token)
        {
            publish("stdout", token + "\t");
            double tokenNum;
            if (std::istringstream(token) >> tokenNum)
            {
                publish("stdout","Push\t\t");
                evaluation.push_back(tokenNum);
            }
            else
            {
                if (evaluation.size() >= 2) // if less than 2 entries in the stack -> missing operand
                {
                    publish("stdout", "Operate\t\t");
                    double secondOperand = evaluation.back();
                    evaluation.pop_back();
                    double firstOperand = evaluation.back();
                    evaluation.pop_back();
                    if (token == "*")
                        evaluation.push_back(firstOperand * secondOperand);
                    else if (token == "/")
                        evaluation.push_back(firstOperand / secondOperand);
                    else if (token == "-")
                        evaluation.push_back(firstOperand - secondOperand);
                    else if (token == "+")
                        evaluation.push_back(firstOperand + secondOperand);
                    else if (token == "^")
                        evaluation.push_back(std::pow(firstOperand, secondOperand));
                }
                else
                {
                    throw std::runtime_error("\nSyntax error:\nmissing operand");
                }
            }
            std::stringstream result;
            std::copy(evaluation.begin(), evaluation.end(), std::ostream_iterator<double>(result, " "));
            publish("stdout",  result.str()+ "\n");
        }
        return evaluation.back();
    }

    nl::json interpreter::execute_request_impl(int execution_counter,
                                               const std::string& code,
                                               bool /*silent*/,
                                               bool /*store_history*/,
                                               nl::json /*user_expressions*/,
                                               bool /*allow_stdin*/)
    {
        // You can use the C-API of your target language for executing the code,
        // e.g. `PyRun_String` for the Python C-API
        //      `luaL_dostring` for the Lua C-API

        // Use this method for publishing the execution result to the client,
        // this method takes the ``execution_counter`` as first argument,
        // the data to publish (mime type data) as second argument and metadata
        // as third argument.
        // Replace "Hello World !!" by what you want to be displayed under the execution cell

        nl::json pub_data;
        std::string result = "Result = ";
        auto publish = [this](const std::string& name, const std::string& text) {
            this->publish_stream(name,text);
        };
        try
        {
            std::string spaced_code = formating_expr(code);
            result += std::to_string(compute_rpn(parse_rpn(spaced_code,publish), publish));
            pub_data["text/plain"] = result;
            publish_execution_result(execution_counter, std::move(pub_data), nl::json::object());
            nl::json jresult;
            jresult["status"] = "ok";
            jresult["payload"] = nl::json::array();
            jresult["user_expressions"] = nl::json::object();
            return jresult;
        }
        catch (const std::runtime_error& err)
        {
            nl::json jresult;
            publish_stream("stderr", err.what());
            jresult["status"] = "error";
            return jresult;
        }
        // You can also use this method for publishing errors to the client, if the code
        // failed to execute
        // publish_execution_error(error_name, error_value, error_traceback);
        // publish_execution_error("TypeError", "123", {"!@#$", "*(*"});

   }

    nl::json interpreter::complete_request_impl(const std::string& /*code*/,int /*cursor_pos*/)
    {
        nl::json jresult;
        jresult["status"] = "ok";
        return jresult;
    };

    nl::json interpreter::inspect_request_impl(const std::string& /*code*/, int /*cursor_pos*/,int /*detail_level*/)
    {
        nl::json jresult;
        jresult["status"] = "ok";
        return jresult;
    };



    nl::json interpreter::is_complete_request_impl(const std::string& /*code*/)
    {
        nl::json jresult;
        jresult["status"] = "complete";
        return jresult;
    };

    nl::json interpreter::kernel_info_request_impl()
    {
        nl::json result;
        result["implementation"] = "xeus-calc";
        result["implementation_version"] = "0.1.0";
        std::string banner = ""
        " **     ** ******** **     **  ********         ******      **     **         ****** \n"
        "//**   ** /**///// /**    /** **//////         **////**    ****   /**        **////** \n"
        " //** **  /**      /**    /**/**              **    //    **//**  /**       **    // \n"
        "  //***   /******* /**    /**/********* *****/**         **  //** /**      /**       \n"
        "   **/**  /**////  /**    /**////////**///// /**        **********/**      /**       \n"
        "  ** //** /**      /**    /**       /**      //**    **/**//////**/**      //**    ** \n"
        " **   //**/********//*******  ********        //****** /**     /**/******** //****** \n"
        "//     // ////////  ///////  ////////          //////  //      // ////////   ////// \n"
        "\n"
        " Implementation of a calculator based on RPN through Xeus";
        result["banner"] = banner;
        result["language_info"]["name"] = "calc";
        result["language_info"]["version"] = "";
        result["language_info"]["mimetype"] = "";
        result["language_info"]["file_extension"] = "";
        return result;
    }

    void interpreter::shutdown_request_impl()
    {
    }

}
