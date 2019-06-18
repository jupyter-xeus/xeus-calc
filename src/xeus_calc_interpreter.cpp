#include <iostream>
#include "xeus_calc_interpreter.hpp"
#include <vector>
#include <sstream>
#include <stack>
#include "xeus/xinterpreter.hpp"

namespace xeus_calc
{
    void interpreter::configure_impl()
    {
    }

    std::string interpreter::parse_rpn(const std::string& infix)
    {
        std::string operators = "-+/*^()";
        std::string spaced_infix;
        for (const char& itr : infix)
        {
            size_t op = operators.find(itr);
            if(op != std::string::npos)
            {
                spaced_infix += ' ';
                spaced_infix += itr;
                spaced_infix += ' ';
            }
            else
            {
                spaced_infix += itr;
            }
        }

        const std::string ops = "-+/*^";
        std::stringstream ss;

        std::stack<int> s;

        std::stringstream input(spaced_infix);
        std::string token;
        while (std::getline(input, token, ' '))
        {
            if (token.empty())
            {
                continue;
            }

            char c = token[0];
            size_t idx = ops.find(c);

            // check for operator
            if (idx != std::string::npos)
            {
                while (!s.empty())
                {
                    int prec2 = s.top() / 2;
                    int prec1 = idx / 2;
                    if (prec2 > prec1 || (prec2 == prec1 && c != '^'))
                    {
                        ss << ops[s.top()] << ' ';
                        s.pop();
                    }
                    else break;
                }
                s.push(idx);
            }
            else if (c == '(')
            {
                s.push(-2); // -2 stands for '('
            }
            else if (c == ')')
            {
                // until '(' on stack, pop operators.
                while (s.top() != -2)
                {
                    ss << ops[s.top()] << ' ';
                    s.pop();
                }
                s.pop();
            }
            else
            {
                ss << token << ' ';
            }
        }

        while (!s.empty())
        {
            ss << ops[s.top()] << ' ';
            s.pop();
        }
        publish_stream("stdout", "RPN = " + ss.str());
        return ss.str();
    }

    double interpreter::compute_rpn(const std::string &expr)
    {
        publish_stream("stdout", "\nInput\tOperation\tStack after\n");
        std::istringstream iss(expr);
        std::vector<double> stack;
        std::string token;
        while (iss >> token)
        {
            publish_stream("stdout", token + "\t");
            double tokenNum;
            if (std::istringstream(token) >> tokenNum)
            {
                publish_stream("stdout","Push\t\t");
                stack.push_back(tokenNum);
            }
            else
            {
                publish_stream("stdout", "Operate\t\t");
                double secondOperand = stack.back();
                stack.pop_back();
                double firstOperand = stack.back();
                stack.pop_back();
                if (token == "*")
                    stack.push_back(firstOperand * secondOperand);
                else if (token == "/")
                    stack.push_back(firstOperand / secondOperand);
                else if (token == "-")
                    stack.push_back(firstOperand - secondOperand);
                else if (token == "+")
                    stack.push_back(firstOperand + secondOperand);
                else if (token == "^")
                    stack.push_back(std::pow(firstOperand, secondOperand));
                else
                { //just in case
                    publish_stream("stderr", "Error");
                    std::exit(1);
                }
            }
            std::stringstream ss;
            std::copy(stack.begin(), stack.end(), std::ostream_iterator<double>(ss, " "));
            publish_stream("stdout",  ss.str()+ "\n");
        }
        return stack.back();
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

        result += std::to_string(compute_rpn(parse_rpn(code)));
		pub_data["text/plain"] = result;
        publish_execution_result(execution_counter, std::move(pub_data), nl::json::object());

        // You can also use this method for publishing errors to the client, if the code
        // failed to execute
        // publish_execution_error(error_name, error_value, error_traceback);
        // publish_execution_error("TypeError", "123", {"!@#$", "*(*"});

        nl::json jresult;
        jresult["status"] = "ok";
        jresult["payload"] = nl::json::array();
        jresult["user_expressions"] = nl::json::object();
        return jresult;
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
