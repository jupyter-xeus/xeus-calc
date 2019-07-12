#include <iostream>
#include <vector>
#include <sstream>
#include <stack>
#include <cctype>

#include "xeus/xinterpreter.hpp"

#include "xeus-calc/xeus_calc_interpreter.hpp"

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

    std::string parse_rpn(std::string expression)
    {
        std::istringstream infix(expression);
        std::string token;
        std::string output;
        std::string operators = "+-*/^";
        std::stack<double> operator_stack;
        while (infix >> token)
        {
            char first_term = token[0];
            if (isdigit(first_term))
            {
                std::cout << "number : " << token.str() << std::endl;
                output += token.str();
            }
            else if (operators.find(first_term) != std::string::npos)
            {
                while(!operators.empty)
                {

                }
            }
            else if (first_term == '(')
            {

            }
            else if (first_term == ')')
            {

            }
            else
            {

            }
        }
    }

    using operators_map_type = std::map<std::string, std::function<double(double first_argument, double second_argument)>>;

    operators_map_type build_operators_map()
    {
        operators_map_type operators_map;
        operators_map["+"] = std::plus<double>();
        operators_map["-"] = std::minus<double>();
        operators_map["*"] = std::multiplies<double>();
        operators_map["/"] = std::divides<double>();
        operators_map["^"] = [](double first_argument, double second_argument) {
            return std::pow(first_argument, second_argument);
        };

        return operators_map;
    }

    double compute_rpn(const std::string &expr, publish_type publish)
    {
        publish("stdout", "\nInput\tOperation\tStack after\n");
        std::istringstream input(expr);
        std::vector<double> evaluation;
        std::string token;
        // the map is initialized only once
        static operators_map_type operators_map = build_operators_map();
        while (input >> token)
        {
            publish("stdout", token + "\t");
            double token_num;
            if (std::istringstream(token) >> token_num)
            {
                publish("stdout","Push\t\t");
                evaluation.push_back(token_num);
            }
            else
            {
                // if less than 2 entries in the stack -> missing operand
                if (evaluation.size() >= 2)
                {
                    publish("stdout", "Operate\t\t");
                    auto it = operators_map.find(token);
                    if (it != operators_map.end())
                    {
                        double second_argument = evaluation.back();
                        evaluation.pop_back();
                        double first_argument = evaluation.back();
                        evaluation.pop_back();
                        evaluation.push_back((it->second)(first_argument, second_argument));
                    }
                    else
                    {
                        throw std::runtime_error("\nSyntax error:\noperator or function not recognized");
                    }
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
        result["implementation"] = "xcalc";
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
