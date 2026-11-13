/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#pragma once

#include "libams.hpp"
#include "ns_mitm_service.hpp"

enum {
    MitmManagerPort_NsAm2,
    MitmManagerPort_NsRo,
    MitmManagerPort_Count,
};

static constexpr size_t MitmManagerMaxSessions = 1
    + NsAm2MitmService::GetMaxSessions()
    + NsRoMitmService::GetMaxSessions();

struct MitmManagerOptions : public ams::sf::hipc::DefaultServerManagerOptions {
    static constexpr bool CanManageMitmServers = true;
};

class MitmManager : public ams::sf::hipc::ServerManager<MitmManagerPort_Count, MitmManagerOptions, MitmManagerMaxSessions> {
private:
    virtual ams::Result OnNeedsToAccept(int port_index, Server* server) override;
public:
    ams::Result RegisterServers();
};
