/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#pragma once

#include "libams.hpp"

// Sizing structures for untouched commands
struct Struct0x90 { u8 data[0x90]; };
struct Struct0x88 { u8 data[0x88]; };
struct Struct0x80 { u8 data[0x80]; };
struct Struct0x10 { u8 data[0x10]; };
struct Struct0xC  { u8 data[0xC];  };

enum NsROAppControlDataInterfaceCmdId : u32 {
	GetAppControlData                = 0,
	GetAppDesiredLanguage            = 1,
	ConvertAppLanguageToLanguageCode = 2,
	ConvertLanguageCodeToAppLanguage = 3,
	SelectApplicationDesiredLanguage = 4,
	GetAppControlData5               = 5,
	GetAppControlData6               = 6,
	Unk7                             = 7,
	Unk8                             = 8,
	Unk9                             = 9,
	GetAppTitleAsync                 = 10,
	Unk11                            = 11,
	Unk12                            = 12,
	GetAppTitle2Async                = 13,
	Unk14                            = 14,
	Unk15                            = 15,
	Unk16                            = 16,
	Unk17                            = 17,
	GetAppControlData18              = 18,
	GetAppControlData19              = 19,
	GetAppControlData20              = 20,
	GetAppControlData21              = 21,
	GetAppControlData22              = 22
};

#define NS_RO_APP_CONTROL_DATA_INTERFACE_INFO(C, H) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, GetAppControlData, (u8 flag, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<u32> out_size), (flag, tid, buffer, out_size)) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, GetAppDesiredLanguage, (u32 bitmask, ams::sf::Out<u8> out_langentry), (bitmask, out_langentry)) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, ConvertAppLanguageToLanguageCode, (u8 langentry, ams::sf::Out<u64> langcode), (langentry, langcode)) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, ConvertLanguageCodeToAppLanguage, (u64 langcode, ams::sf::Out<u8> langentry), (langcode, langentry)) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, SelectApplicationDesiredLanguage, (ams::sf::Out<u64> out_bytes, const ams::sf::InMapAliasBuffer &in_buffer), (out_bytes, in_buffer)) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, GetAppControlData5, (u8 source, u8 flag, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<u64> out_size), (source, flag, tid, buffer, out_size)) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, GetAppControlData6, (u8 source, u8 flag1, u8 flag2, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<u64> out_size), (source, flag1, flag2, tid, buffer, out_size)) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, Unk7, (u64 tid, ams::sf::Out<Struct0x80> out_bytes), (tid, out_bytes)) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, Unk8, (Struct0x88 in_bytes, ams::sf::Out<u32> out_bytes, const ams::sf::OutMapAliasBuffer &out_buffer), (in_bytes, out_bytes, out_buffer)) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, Unk9, (u64 tid, const ams::sf::InMapAliasBuffer &in_buffer), (tid, in_buffer)) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, GetAppControlData18, (u8 source, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<u64> out_size), (source, tid, buffer, out_size)) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, GetAppControlData19, (u8 source, u8 flag, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<Struct0xC> out_size), (source, flag, tid, buffer, out_size)) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, GetAppControlData20, (u8 source, u8 flag1, u8 flag2, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<Struct0xC> out_size), (source, flag1, flag2, tid, buffer, out_size)) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, GetAppControlData21, (u8 source, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<Struct0xC> out_size), (source, tid, buffer, out_size)) \
	AMS_SF_METHOD_INFO_F(C, H, NsROAppControlDataInterfaceCmdId, GetAppControlData22, (u8 source, u8 flag1, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<Struct0xC> out_size), (source, flag1, tid, buffer, out_size))

AMS_SF_DEFINE_INTERFACE_F(NsROAppControlDataInterface, NS_RO_APP_CONTROL_DATA_INTERFACE_INFO, 0x5dc5db1b);

class NsROAppControlDataService {
private:
    ams::sm::MitmProcessInfo m_client_info;
    std::unique_ptr<Service> srv;
public:
    NsROAppControlDataService(const ams::sm::MitmProcessInfo &client_info, std::unique_ptr<Service> &&srv)
        : m_client_info(client_info), srv(std::move(srv)) {}
    ~NsROAppControlDataService() {
        if (this->srv) {
            serviceClose(this->srv.get());
        }
    }

    NS_RO_APP_CONTROL_DATA_INTERFACE_INFO(_, AMS_SF_DECLARE_INTERFACE_METHODS);
};
static_assert(IsNsROAppControlDataInterface<NsROAppControlDataService>);

enum class NsSrvGetterCmdId : u32 {
    GetROAppControlDataInterface = 7989,
};

#define NS_SRV_GETTER_MITM_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO_F(C, H, NsSrvGetterCmdId, GetROAppControlDataInterface, (ams::sf::Out<ams::sf::SharedPointer<NsROAppControlDataInterface>> out), (out))

AMS_SF_DEFINE_MITM_INTERFACE_F(NsServiceGetterMitmInterface, NS_SRV_GETTER_MITM_INTERFACE_INFO, 0xf4ec2d1a);

class NsServiceGetterMitmService : public ams::sf::MitmServiceImplBase {
public:
    using ::ams::sf::MitmServiceImplBase::MitmServiceImplBase;
    NS_SRV_GETTER_MITM_INTERFACE_INFO(_, AMS_SF_DECLARE_INTERFACE_METHODS);
};

class NsAm2MitmService : public NsServiceGetterMitmService {
public:
    using ::NsServiceGetterMitmService::NsServiceGetterMitmService;
    static bool ShouldMitm(const ams::sm::MitmProcessInfo& client_info);
    static constexpr ams::sm::ServiceName GetServiceName() {
        return ams::sm::ServiceName::Encode("ns:am2");
    }
    static constexpr size_t GetMaxSessions() { return 4; }
};
static_assert(IsNsServiceGetterMitmInterface<NsAm2MitmService>);

class NsRoMitmService : public NsServiceGetterMitmService {
public:
    using ::NsServiceGetterMitmService::NsServiceGetterMitmService;
    static bool ShouldMitm(const ams::sm::MitmProcessInfo& client_info);
    static constexpr ams::sm::ServiceName GetServiceName() {
        return ams::sm::ServiceName::Encode("ns:ro");
    }
    static constexpr size_t GetMaxSessions() { return 4; }
};
static_assert(IsNsServiceGetterMitmInterface<NsRoMitmService>);
