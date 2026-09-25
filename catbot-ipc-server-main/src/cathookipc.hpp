#pragma once

#include <ctime>

// Layout must match ogcathook-rewrite include/ipc.hpp.
struct server_data_s
{
    unsigned magic_number;
};

struct user_data_s
{
    char name[32];
    unsigned friendid;
    bool textmode;

    bool connected;

    time_t heartbeat;

    time_t ts_injected;
    time_t ts_connected;
    time_t ts_disconnected;

    struct accumulated_t
    {
        int kills;
        int deaths;
        int score;

        int shots;
        int hits;
        int headshots;
    } accumulated;

    struct
    {
        bool good;

        int kills;
        int deaths;
        int score;

        int shots;
        int hits;
        int headshots;

        int team;
        int role;
        char life_state;
        int health;
        int health_max;

        float x;
        float y;
        float z;

        int player_count;
        int bot_count;

        char server[24];
        char mapname[32];
    } ingame;
};

namespace ipc_commands
{
constexpr unsigned execute_client_cmd      = 1;
constexpr unsigned set_follow_steamid      = 2;
constexpr unsigned execute_client_cmd_long = 3;
constexpr unsigned move_to_vector          = 4;
constexpr unsigned stop_moving             = 5;
constexpr unsigned start_moving            = 6;
} // namespace ipc_commands

constexpr unsigned cathook_magic_number = 0x0DEADCA7;
constexpr const char *cathook_ipc_name  = "cathook_followbot_server";
