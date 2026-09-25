#include <SimpleIPC/ipcb.hpp>
#include "cathookipc.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

using peer_t = cat_ipc::Peer<server_data_s, user_data_s>;

int main(int argc, const char **argv)
{
    if (argc < 2)
    {
        std::cerr << "usage: exec_all <command...>\n";
        return EXIT_FAILURE;
    }

    std::string command{};
    for (int index = 1; index < argc; ++index)
    {
        if (!command.empty())
            command.push_back(' ');
        command += argv[index];
    }

    try
    {
        peer_t peer(cathook_ipc_name, false, false, true);
        peer.Connect();
        if (command.size() >= 63)
            peer.SendMessage(nullptr, -1, ipc_commands::execute_client_cmd_long, command.c_str(), command.size() + 1);
        else
        {
            char small[cat_ipc::command_data]{};
            std::memcpy(small, command.c_str(), std::min(command.size(), sizeof(small) - 1));
            peer.SendMessage(small, -1, ipc_commands::execute_client_cmd, nullptr, 0);
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
