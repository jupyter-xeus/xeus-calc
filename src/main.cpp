/***************************************************************************
* Copyright (c) 2019, Sylvain Corlay, Johan Mabille, Wolf Vollprecht       *
* Copyright (c) 2019, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include <memory>

#include "xeus/xeus_context.hpp"
#include "xeus/xkernel.hpp"
#include "xeus/xkernel_configuration.hpp"
#include "xeus/xserver.hpp"

#include "xeus-zmq/xserver_zmq_split.hpp"
#include "xeus-zmq/xzmq_context.hpp"

#include "xeus-calc/xeus_calc_interpreter.hpp"

int main(int argc, char* argv[])
{
    // Load configuration file
    std::string file_name = (argc == 1) ? "connection.json" : argv[2];
    xeus::xconfiguration config = xeus::load_configuration(file_name);

    // Create context
    std::unique_ptr<xeus::xcontext> context = xeus::make_zmq_context();
    
    // Create interpreter instance
    using interpreter_ptr = std::unique_ptr<xeus_calc::interpreter>;
    interpreter_ptr interpreter = interpreter_ptr(new xeus_calc::interpreter());

    // Create kernel instance and start it
    xeus::xkernel kernel(config,
                         xeus::get_user_name(),
                         std::move(context),
                         std::move(interpreter),
                         xeus::make_xserver_shell_main);
    kernel.start();

    return 0;
}
