#include <SimpleIPC/ipcb.hpp>
#include "cathookipc.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>

namespace
{

using peer_t = cat_ipc::Peer<server_data_s, user_data_s>;
std::atomic_bool running = true;

constexpr const char *classes[] = {
    "Unknown", "Scout", "Sniper", "Soldier", "Demoman", "Medic", "Heavy", "Pyro", "Spy", "Engineer"
};
constexpr const char *teams[] = { "UNK", "SPEC", "RED", "BLU" };

void signal_handler(int)
{
    running.store(false);
}

bool good_class(int class_id)
{
    return class_id > 0 && class_id < 10;
}

bool good_team(int team_id)
{
    return team_id >= 0 && team_id < 4;
}

void print_status(peer_t &peer)
{
    std::unique_lock<boost::interprocess::interprocess_mutex> lock(peer.memory->mutex);
    std::printf("\033[1;1H\033[2J");
    std::printf("\033[2;2H\033[1mnullhook IPC server (SimpleIPC / ogcathook-rewrite)\033[0m");
    std::printf("\033[3;4H\033[1mconnected: \033[0m%u / %u", peer.memory->peer_count, cat_ipc::max_peers);
    std::printf("\033[3;5H\033[1mcommand count: \033[0m%lu", peer.memory->command_count);
    std::printf("\033[2;8H%-2s %-5s %-12s %-21s %s\n", "ID", "PID", "SteamID", "Server IP", "Name");
    auto row = 11;
    const auto now = std::time(nullptr);
    for (unsigned index = 0; index < cat_ipc::max_peers; ++index)
    {
        if (peer.memory->peer_data[index].free)
            continue;
        const auto &data = peer.memory->peer_user_data[index];
        std::printf("\033[%d;1H%-2u %-5d %-12u %-21.*s %.*s\n", row, index, peer.memory->peer_data[index].pid,
                    data.friendid, static_cast<int>(sizeof(data.ingame.server)), data.ingame.server,
                    static_cast<int>(sizeof(data.name)), data.name);
        if (data.connected && data.ingame.good)
        {
            std::printf("    %-5s %-9s %-4s   %-5d   %-5d   %-4d/%-4d %ld\n", data.ingame.life_state ? "Dead" : "Alive",
                        good_class(data.ingame.role) ? classes[data.ingame.role] : classes[0],
                        good_team(data.ingame.team) ? teams[data.ingame.team] : teams[0], data.ingame.score,
                        data.accumulated.score, data.ingame.health, data.ingame.health_max, now - data.heartbeat);
        }
        row += 2;
    }
    std::fflush(stdout);
}

} // namespace

int main(int argc, char **argv)
{
    bool silent         = false;
    bool reset_existing = false;
    for (int index = 1; index < argc; ++index)
    {
        const std::string_view arg{ argv[index] };
        if (arg == "-s")
            silent = true;
        else if (arg == "--reset")
            reset_existing = true;
    }
    (void)reset_existing;

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try
    {
        peer_t peer(cathook_ipc_name, false, true, false);
        peer.Connect();
        peer.memory->global_data.magic_number = cathook_magic_number;
        while (running.load())
        {
            peer.SweepDead();
            if (!silent)
                print_status(peer);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << "ipc server failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
