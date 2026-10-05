#pragma once
// Copyright (c) 2023, kiwec, All rights reserved.

#include "config.h"

#include <string_view>

struct Packet;

// neomod's own Bancho server (a third-party server for mikosu). Only used to enable features that server provides;
// never as a default and never as mikosu's identity.
#define NEOMOD_DOMAIN "neomod.net"

// NOTE: Full version can be something like "b20200201.2cuttingedge"
// OSU_VERSION_DATEONLY also versions neomod's own database formats and the score-submission protocol (BanchoSubmitter,
// BanchoAes); it is NOT sent as mikosu's identity. The login identifies mikosu honestly (BANCHO_CLIENT_VERSION).
#define OSU_VERSION_DATEONLY 20260711
#define OSU_VERSION "b20260711.1"

// what mikosu tells a Bancho-protocol server it is, in the login request's version field: mikosu and its version,
// never an osu!stable/lazer/neomod version string. Servers that only accept official-looking versions will refuse
// the login; that is intended (ground rule: identify honestly, never pretend to get past a server's checks).
#define BANCHO_CLIENT_VERSION PACKAGE_NAME "-" PACKAGE_VERSION

namespace BANCHO::Net {

// Queue a packet for the next request to Bancho (copies it, so the packet can be dropped or reused right after)
void send_packet(const Packet& packet);

// Process networking logic. Should be called regularly from main thread.
void update_networking();

// Clean up networking. Should be called once when exiting neomod.
void cleanup_networking();

}  // namespace BANCHO::Net
