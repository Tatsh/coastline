#include <cstdio>

#include "instantreplay.h"
#include "text.h"
#include "vehicle.h"
#include "vehiclecount.h"
#include "win32/directplay.h"
#include "win32/hud.h"
#include "win32/settings.h"

#undef DrawText

int g_bHudShowNetworkStandings = 1;
int g_bHideNetStandingsHud;

namespace {
s32 RowPosition(s32 y, s32 position) {
    return (s32)((u32)y + ((u32)position << 4) + 20);
}
} // namespace

void DrawNetworkRaceStandingsHud(const ViewportRect *viewport, const Vehicle *vehicles) {
    char text[256];

    if (!g_bHudShowNetworkStandings) {
        return;
    }
    SetTextSize(15);
    SetTextAlignLeft();
    s32 x = (s32)((u32)viewport->x + 16);
    s32 y = (s32)((u32)viewport->y + 29);
    if (!g_dwNetworkPlayMode || !g_dwNumActiveVehicles) {
        return;
    }
    for (s32 index = 0; index < g_dwNumActiveVehicles; ++index) {
        const Vehicle &vehicle = vehicles[index];
        if (vehicle.GetDriver()) {
            int status = GetNormalizedSlotStatusFlags(vehicle.GetGridIndex());
            const char *format = status ? "%%11%d." : index == 0 ? "%%y1%d." : "%%ry%d.";
            std::sprintf(text, format, (s32)((u32)vehicle.GetRacePosition() + 1));
            DrawText(text, x, RowPosition(y, vehicle.GetRacePosition()));
            if (g_bHideNetStandingsHud) {
                int channel = ' ';
                int peer = ' ';
                if (g_dwNetworkPlayMode > 1) {
                    int channelStatus = GetNetworkPlayerSlotStatus(vehicle.GetGridIndex()) & 63;
                    if (channelStatus < 3) {
                        channel = peer = '*';
                    } else if (channelStatus == 3) {
                        peer = (GetNetworkPeerChannelStatus(vehicle.GetGridIndex()) & 63) != 3 ?
                                   '*' :
                                   ' ';
                    }
                }
                if (status) {
                    int ping = GetPlayerPingMs(vehicle.GetGridIndex());
                    std::sprintf(
                        text, "%%11%s %dms %c%c", vehicle.GetPlayerName(), ping, peer, channel);
                } else if (!(vehicle.GetRacer()->GetStatus() & 1) && !gInstantReplayActive) {
                    int ping = GetPlayerPingMs(vehicle.GetGridIndex());
                    const char *gap = FormatGapToLeaderString(vehicles, index);
                    const auto &color = vehicle.GetColor();
                    std::sprintf(text,
                                 "%%x%02x%02x%02x%s %s %dms %c%c",
                                 color[0],
                                 color[1],
                                 color[2],
                                 vehicle.GetPlayerName(),
                                 gap,
                                 ping,
                                 peer,
                                 channel);
                } else {
                    int ping = GetPlayerPingMs(vehicle.GetGridIndex());
                    const auto &color = vehicle.GetColor();
                    std::sprintf(text,
                                 "%%x%02x%02x%02x%s %dms %c%c",
                                 color[0],
                                 color[1],
                                 color[2],
                                 vehicle.GetPlayerName(),
                                 ping,
                                 peer,
                                 channel);
                }
            } else if (status) {
                std::sprintf(text, "%%11%s", vehicle.GetPlayerName());
            } else if (!(vehicle.GetRacer()->GetStatus() & 1) && !gInstantReplayActive) {
                const char *gap = FormatGapToLeaderString(vehicles, index);
                const auto &color = vehicle.GetColor();
                std::sprintf(text,
                             "%%x%02x%02x%02x%s %s",
                             color[0],
                             color[1],
                             color[2],
                             vehicle.GetPlayerName(),
                             gap);
            } else {
                const auto &color = vehicle.GetColor();
                std::sprintf(text,
                             "%%x%02x%02x%02x%s",
                             color[0],
                             color[1],
                             color[2],
                             vehicle.GetPlayerName());
            }
        } else {
            std::sprintf(text, "%%55%d.", (s32)((u32)vehicle.GetRacePosition() + 1));
            DrawText(text, x, RowPosition(y, vehicle.GetRacePosition()));
            std::sprintf(text, "%%55%s", vehicle.GetPlayerName());
        }
        DrawText(text, (s32)((u32)x + 16), RowPosition(y, vehicle.GetRacePosition()));
    }
}
