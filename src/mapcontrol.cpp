#include "engine.h"

#include <cmath>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <algorithm>
#include <vector>

#include <enet/enet.h>
#include <zlib.h>

#include "tools.h"
#include "geom.h"
#include "command.h"

#include "iengine.h"
#include "igame.h"

#include "game.h"
#include "cserver.h"

//location for the spawns
vec spawn1 = vec(0,0,0),
    spawn2 = vec(0,0,0);

constexpr int maxgamescore = 10; //score margin at which to end the game

bool mapcontrolintermission()
{
    if(std::max(server::teaminfos[0].score, server::teaminfos[1].score) < maxgamescore)
    {
        return false; //keep playing
    }
    else
    {
        return true; //one team has enough points, end the game
    }
}

void calcscores()
{
    uint team1size = 0;
    uint team2size = 0;
    uint team1dead = 0;
    uint team2dead = 0;
    for(int i = 0; i < server::clients.length(); ++i)
    {
        if(server::clients[i] != nullptr)
        {
            server::clientinfo *ci = server::clients[i];
            //get the sizes of each team
            if(ci->team == 1)
            {
                team1size++;
                if(ci->state.state != ClientState_Alive)
                {
                    team1dead++;
                }
            }
            else if(ci->team == 2)
            {
                team2size++;
                if(ci->state.state != ClientState_Alive)
                {
                    team2dead++;
                }
            }
        }
    }
    //do not attempt to calculate scores if one team is empty
    if(team1size == 0 || team2size == 0)
    {
        return;
    }
    server::clientinfo *ci = server::clients[0];
    //we now know how big each team is and how many are not alive
    //so now we check if either team is all dead
    if(team1size == team1dead) //team 1 is all dead
    {
        printf("Tean 1 has died\n");
        server::teaminfos[1].score += 1; //add score to team 2

        //now award all alive players on other team 1 point for living
        for(int j = 0; j < server::clients.length(); ++j)
        {
            if(server::clients[j] != nullptr)
            {
                if(server::clients[j]->state.state == ClientState_Alive && server::clients[j]->team == 2)
                {
                    server::clients[j]->state.score += 1;
                }
            }
        }
    }
    //we now know how big each team is and how many are not alive
    //so now we check if either team is all dead
    if(team2size == team2dead) //team 1 is all dead
    {
        printf("Team 2 has died\n");
        server::teaminfos[0].score += 1; //add score to team 1

        //now award all alive players on other team 1 point for living
        for(int j = 0; j < server::clients.length(); ++j)
        {
            if(server::clients[j] != nullptr)
            {
                if(server::clients[j]->state.state == ClientState_Alive && server::clients[j]->team == 1)
                {
                    server::clients[j]->state.score += 1;
                }
            }
        }
    }
    //now respawn all clients
    if(team2size == team2dead || team1size == team1dead)
    {
        for(int i = 0; i < server::clients.length(); ++i)
        {
            server::clients[i]->state.respawn();
            server::sendspawn(server::clients[i]);
            printf("player health: %d\n", server::clients[i]->state.health);
        }
    }
}

void sendscore()
{
    for(int i = 0; i < server::clients.length(); i++)
    {
        if(server::clients[i] != nullptr)
        {
            server::clientinfo *ci = server::clients[i];
            sendf(-1, 1, "iiiii", NetMsg_GetScore, ci->clientnum , ci->state.score, server::teaminfos[0].score, server::teaminfos[1].score);
        }
    }
}
void updatescores()
{
    calcscores();
}
