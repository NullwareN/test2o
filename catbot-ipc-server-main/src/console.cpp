#include <SimpleIPC/ipcb.hpp>
#include "cathookipc.hpp"
#include "json.hpp"

#include <algorithm>
#include <csignal>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

using json   = nlohmann::json;
using peer_t = cat_ipc::Peer<server_data_s, user_data_s>;

namespace
{

std::unique_ptr<peer_t> ipc_peer;
std::unordered_map<std::string, std::function<json(const json &)>> commands{};

bool has_key(const json &object, const std::string &key)
{
    return object.find(key) != object.end();
}

void require_connected()
{
    if (!ipc_peer || !ipc_peer->memory)
        throw std::runtime_error("not connected to ipc server");
}

bool peer_dead(unsigned id)
{
    if (id >= cat_ipc::max_peers)
        return true;
    std::unique_lock<boost::interprocess::interprocess_mutex> lock(ipc_peer->memory->mutex);
    const auto &slot = ipc_peer->memory->peer_data[id];
    if (slot.free)
        return true;
    return std::time(nullptr) - slot.heartbeat >= 10;
}

std::string bounded_string(const char *value, std::size_t size)
{
    return value == nullptr ? std::string{} : std::string{ value, strnlen(value, size) };
}

void send_game_command(int target_id, std::string command)
{
    require_connected();
    while (true)
    {
        auto found = command.find(" && ");
        if (found == std::string::npos)
            break;
        command.replace(found, 4, " ; ");
    }
    if (command.size() >= 63)
        ipc_peer->SendMessage(nullptr, target_id, ipc_commands::execute_client_cmd_long, command.c_str(),
                              command.size() + 1);
    else
    {
        char small[cat_ipc::command_data]{};
        std::memcpy(small, command.c_str(), std::min(command.size(), sizeof(small) - 1));
        ipc_peer->SendMessage(small, target_id, ipc_commands::execute_client_cmd, nullptr, 0);
    }
}

bool kill_os_peer(const cat_ipc::PeerData &peer)
{
#if defined(SYS_pidfd_open) && defined(SYS_pidfd_send_signal)
    const auto pidfd = static_cast<int>(::syscall(SYS_pidfd_open, peer.pid, 0));
    if (pidfd >= 0)
    {
        ProcStat stat{};
        if (!ReadStat(peer.pid, &stat) || stat.starttime != peer.starttime)
        {
            ::close(pidfd);
            return false;
        }
        const auto result = ::syscall(SYS_pidfd_send_signal, pidfd, SIGKILL, nullptr, 0);
        ::close(pidfd);
        return result == 0;
    }
#endif
    ProcStat stat{};
    if (!ReadStat(peer.pid, &stat) || stat.starttime != peer.starttime)
        return false;
    return ::kill(peer.pid, SIGKILL) == 0;
}

json query_peer(unsigned id)
{
    require_connected();
    if (id >= cat_ipc::max_peers)
        throw std::out_of_range("peer out of range");

    std::unique_lock<boost::interprocess::interprocess_mutex> lock(ipc_peer->memory->mutex);
    json result{};
    const auto &slot = ipc_peer->memory->peer_data[id];
    if (slot.free || std::time(nullptr) - slot.heartbeat >= 10)
    {
        result["dead"] = true;
        return result;
    }

    const auto &data = ipc_peer->memory->peer_user_data[id];
    result["name"]            = bounded_string(data.name, sizeof(data.name));
    result["friendid"]        = data.friendid;
    result["connected"]       = data.connected;
    result["heartbeat"]       = data.heartbeat;
    result["ts_injected"]     = data.ts_injected;
    result["ts_connected"]    = data.ts_connected;
    result["ts_disconnected"] = data.ts_disconnected;
    result["ts_queue_started"] = 0;
    result["accumulated"]     = { { "kills", data.accumulated.kills },
                              { "deaths", data.accumulated.deaths },
                              { "score", data.accumulated.score },
                              { "shots", data.accumulated.shots },
                              { "hits", data.accumulated.hits },
                              { "headshots", data.accumulated.headshots } };
    result["ingame"]          = { { "good", data.ingame.good },
                         { "kills", data.ingame.kills },
                         { "deaths", data.ingame.deaths },
                         { "score", data.ingame.score },
                         { "shots", data.ingame.shots },
                         { "hits", data.ingame.hits },
                         { "headshots", data.ingame.headshots },
                         { "team", data.ingame.team },
                         { "role", data.ingame.role },
                         { "life_state", data.ingame.life_state },
                         { "health", data.ingame.health },
                         { "health_max", data.ingame.health_max },
                         { "x", data.ingame.x },
                         { "y", data.ingame.y },
                         { "z", data.ingame.z },
                         { "player_count", data.ingame.player_count },
                         { "bot_count", data.ingame.bot_count },
                         { "server", bounded_string(data.ingame.server, sizeof(data.ingame.server)) },
                         { "mapname", bounded_string(data.ingame.mapname, sizeof(data.ingame.mapname)) } };
    result["pid"]       = slot.pid;
    result["starttime"] = slot.starttime;
    return result;
}

namespace cmd
{

json exec(const json &args)
{
    require_connected();
    if (!has_key(args, "target"))
        throw std::runtime_error("undefined pid");
    if (!has_key(args, "cmd"))
        throw std::runtime_error("undefined command");
    const auto target_id = args["target"].get<int>();
    if (target_id < 0 || target_id >= static_cast<int>(cat_ipc::max_peers))
        throw std::out_of_range("peer out of range");
    if (peer_dead(static_cast<unsigned>(target_id)))
        throw std::runtime_error("peer is not connected");
    send_game_command(target_id, args["cmd"].get<std::string>());
    return json{};
}

json exec_all(const json &args)
{
    require_connected();
    if (!has_key(args, "cmd"))
        throw std::runtime_error("undefined command");
    send_game_command(-1, args["cmd"].get<std::string>());
    return json{};
}

json query(const json &args)
{
    require_connected();
    json result = json::object();
    const auto skip_empty = has_key(args, "skipEmpty") && args["skipEmpty"].get<bool>();
    if (has_key(args, "ids"))
    {
        for (const auto &id_json : args["ids"])
        {
            const auto id = id_json.get<unsigned>();
            if (skip_empty && peer_dead(id))
                continue;
            result[std::to_string(id)] = query_peer(id);
        }
        return result;
    }
    for (unsigned id = 0; id < cat_ipc::max_peers; ++id)
    {
        if (skip_empty && peer_dead(id))
            continue;
        result[std::to_string(id)] = query_peer(id);
    }
    return result;
}

json squery(const json &)
{
    require_connected();
    std::unique_lock<boost::interprocess::interprocess_mutex> lock(ipc_peer->memory->mutex);
    return json{ { "count", ipc_peer->memory->peer_count }, { "command_count", ipc_peer->memory->command_count } };
}

json kill(const json &args)
{
    require_connected();
    if (getuid() != 0)
        throw std::runtime_error("kill can only be used as root");
    if (!has_key(args, "pid"))
        throw std::runtime_error("undefined pid");
    const auto id = args["pid"].get<int>();
    if (id < 0 || id >= static_cast<int>(cat_ipc::max_peers))
        throw std::out_of_range("peer out of range");
    std::unique_lock<boost::interprocess::interprocess_mutex> lock(ipc_peer->memory->mutex);
    const auto &peer = ipc_peer->memory->peer_data[id];
    if (peer.free)
        throw std::runtime_error("already dead");
    if (!kill_os_peer(peer))
        throw std::runtime_error("peer is no longer the registered process");
    return json{};
}

json echo(const json &args)
{
    return json{ { "args", args } };
}

json connect(const json &args)
{
    // A failed Connect() used to leave ipc_peer set with memory == nullptr.
    // Every later connect then returned "already connected" while every query
    // returned "not connected", and the panel stayed blind to live peers.
    if (ipc_peer && ipc_peer->memory)
        throw std::runtime_error("already connected");
    ipc_peer.reset();
    if (has_key(args, "server") && args["server"].get<std::string>() != cathook_ipc_name)
        throw std::runtime_error("custom ipc server names are not supported by this build");
    try
    {
        ipc_peer = std::make_unique<peer_t>(cathook_ipc_name, false, false, true);
        ipc_peer->Connect();
    }
    catch (...)
    {
        ipc_peer.reset();
        throw;
    }
    return json{};
}

json disconnect(const json &)
{
    ipc_peer.reset();
    return json{};
}

} // namespace cmd

} // namespace

int main()
{
    commands["exec"]       = &cmd::exec;
    commands["exec_all"]   = &cmd::exec_all;
    commands["query"]      = &cmd::query;
    commands["kill"]       = &cmd::kill;
    commands["echo"]       = &cmd::echo;
    commands["connect"]    = &cmd::connect;
    commands["disconnect"] = &cmd::disconnect;
    commands["squery"]     = &cmd::squery;

    std::cout << json{ { "init", std::time(nullptr) } } << std::endl;

    std::string input{};
    while (std::getline(std::cin, input))
    {
        auto cmdid = std::string{ "undefined" };
        try
        {
            const auto args = json::parse(input);
            if (!has_key(args, "command"))
                throw std::runtime_error("empty command");
            if (has_key(args, "cmdid"))
                cmdid = args["cmdid"];
            const auto command = args["command"].get<std::string>();
            if (command == "exit" || command == "quit")
            {
                std::cout << json{ { "exit", std::time(nullptr) } } << std::endl;
                break;
            }
            if (const auto it = commands.find(command); it != commands.end())
                std::cout << json{ { "status", "success" }, { "cmdid", cmdid }, { "result", it->second(args) } }
                          << std::endl;
            else
                throw std::runtime_error("command not found");
        }
        catch (const std::exception &error)
        {
            std::cout << json{ { "status", "error" }, { "cmdid", cmdid }, { "error", std::string{ error.what() } } }
                      << std::endl;
        }
    }
}
