/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#include "mitm_manager.hpp"
#include "file_utils.hpp"

ams::Result MitmManager::OnNeedsToAccept(int port_index, Server* server) {
    std::shared_ptr<Service> fsrv;
    ams::sm::MitmProcessInfo client_info;
    server->AcknowledgeMitmSession(std::addressof(fsrv), std::addressof(client_info));

    switch (port_index) {
        case MitmManagerPort_NsAm2:
            return this->AcceptMitmImpl(server, ams::sf::CreateSharedObjectEmplaced<NsServiceGetterMitmInterface, NsAm2MitmService>(decltype(fsrv)(fsrv), client_info), fsrv);
        case MitmManagerPort_NsRo:
            return this->AcceptMitmImpl(server, ams::sf::CreateSharedObjectEmplaced<NsServiceGetterMitmInterface, NsRoMitmService>(decltype(fsrv)(fsrv), client_info), fsrv);
        AMS_UNREACHABLE_DEFAULT_CASE();
    }
}

ams::Result MitmManager::RegisterServers() {
    FileUtils::LogLine("[boxart-nx] Registering MitM server for ns:am2...");
    R_TRY((this->RegisterMitmServer<NsAm2MitmService>(MitmManagerPort_NsAm2, NsAm2MitmService::GetServiceName())));

    FileUtils::LogLine("[boxart-nx] Registering MitM server for ns:ro...");
    R_TRY((this->RegisterMitmServer<NsRoMitmService>(MitmManagerPort_NsRo, NsRoMitmService::GetServiceName())));

    return ams::ResultSuccess();
}
