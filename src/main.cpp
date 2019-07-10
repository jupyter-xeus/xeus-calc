#include <memory>

#include "xeus/xkernel.hpp"
#include "xeus/xkernel_configuration.hpp"

#include "xeus-calc/xeus_calc_interpreter.hpp"

int main(int argc, char* argv[])
{
    // Load configuration file
    std::string file_name = (argc == 1) ? "connection.json" : argv[2];
    xeus::xconfiguration config = xeus::load_configuration(file_name);

    // Create interpreter instance
    using interpreter_ptr = std::unique_ptr<xeus_calc::interpreter>;
    interpreter_ptr interpreter = interpreter_ptr(new xeus_calc::interpreter());

    // Create kernel instance and start it
    xeus::xkernel kernel(config, "username", std::move(interpreter));
    kernel.start();

    return 0;
}
