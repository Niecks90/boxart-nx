/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#include "ns_mitm_service.hpp"
#include "file_utils.hpp"
#include <cstring>
#include <algorithm>

// Check if we should MITM this client process
bool NsAm2MitmService::ShouldMitm(const ams::sm::MitmProcessInfo& client_info) {
    // Avoid mitm'ing our own frontend if it calls ns
    if (client_info.program_id == ams::ncm::ProgramId{0x010000000000B0A7ul}) {
        return false;
    }
    return true;
}

bool NsRoMitmService::ShouldMitm(const ams::sm::MitmProcessInfo& client_info) {
    if (client_info.program_id == ams::ncm::ProgramId{0x010000000000B0A7ul}) {
        return false;
    }
    return true;
}

static bool TryReplaceIcon(u64 tid, void* buffer, size_t buffer_size, u32* out_new_total_size) {
    if (!buffer || buffer_size < 0x4000) {
        return false;
    }

    char path[128];
    ams::util::SNPrintf(path, sizeof(path), "%s/%016llx.jpg", BOXART_COVERS_DIR, (unsigned long long)tid);

    bool file_exists = false;
    if (R_FAILED(ams::fs::HasFile(&file_exists, path)) || !file_exists) {
        // Try uppercase hex as fallback
        ams::util::SNPrintf(path, sizeof(path), "%s/%016llX.jpg", BOXART_COVERS_DIR, (unsigned long long)tid);
        if (R_FAILED(ams::fs::HasFile(&file_exists, path)) || !file_exists) {
            return false;
        }
    }

    ams::fs::FileHandle file;
    if (R_FAILED(ams::fs::OpenFile(&file, path, ams::fs::OpenMode_Read))) {
        return false;
    }

    s64 file_size = 0;
    ams::Result rc = ams::fs::GetFileSize(&file_size, file);
    if (R_FAILED(rc) || file_size <= 0 || file_size > 0x20000) {
        ams::fs::CloseFile(file);
        FileUtils::LogLine("[boxart-nx] Cover rejected for tid=0x%016llx: size=%lld (max 0x20000)",
                           (unsigned long long)tid, (long long)file_size);
        return false;
    }

    size_t max_icon_space = buffer_size - 0x4000;
    if (static_cast<size_t>(file_size) > max_icon_space) {
        ams::fs::CloseFile(file);
        return false;
    }

    u8* icon_dest = reinterpret_cast<u8*>(buffer) + 0x4000;
    size_t bytes_read = 0;
    rc = ams::fs::ReadFile(&bytes_read, file, 0, icon_dest, static_cast<size_t>(file_size));
    ams::fs::CloseFile(file);

    if (R_FAILED(rc) || bytes_read != static_cast<size_t>(file_size)) {
        FileUtils::LogLine("[boxart-nx] Read failed for tid=0x%016llx: bytes_read=%zu, expected=%lld",
                           (unsigned long long)tid, bytes_read, (long long)file_size);
        return false;
    }

    // Zero out any unused tail of the icon buffer up to 0x20000
    if (max_icon_space > bytes_read) {
        size_t pad_size = std::min(max_icon_space - bytes_read, static_cast<size_t>(0x20000 - bytes_read));
        std::memset(icon_dest + bytes_read, 0, pad_size);
    }

    *out_new_total_size = 0x4000 + static_cast<u32>(bytes_read);
    FileUtils::LogLine("[boxart-nx] Successfully substituted icon for tid=0x%016llx (%u bytes)",
                       (unsigned long long)tid, (unsigned int)bytes_read);
    return true;
}

ams::Result NsServiceGetterMitmService::GetROAppControlDataInterface(ams::sf::Out<ams::sf::SharedPointer<NsROAppControlDataInterface>> out) {
    Service s;
    Result rc = serviceDispatch(this->m_forward_service.get(), (u32)NsSrvGetterCmdId::GetROAppControlDataInterface,
        .out_num_objects = 1,
        .out_objects = &s,
    );

    if (R_SUCCEEDED(rc)) {
        const ams::sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&s)};
        out.SetValue(ams::sf::CreateSharedObjectEmplaced<NsROAppControlDataInterface, NsROAppControlDataService>(this->m_client_info, std::make_unique<Service>(s)), target_object_id);
    }

    FileUtils::LogLine("[boxart-nx] GetROAppControlDataInterface: rc=0x%x", rc);
    return rc;
}

// Command 0: Standard GetAppControlData
ams::Result NsROAppControlDataService::GetAppControlData(u8 flag, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<u32> out_size) {
    const struct {
        u8 flag;
        u64 tid;
    } in = {flag, tid};

    Result rc = serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::GetAppControlData, in, *out_size.GetPointer(),
        .buffer_attrs = {SfBufferAttr_HipcMapAlias | SfBufferAttr_Out},
        .buffers = {{buffer.GetPointer(), buffer.GetSize()}},
    );

    if (R_SUCCEEDED(rc)) {
        u32 new_total = 0;
        if (TryReplaceIcon(tid, buffer.GetPointer(), buffer.GetSize(), &new_total)) {
            out_size.SetValue(new_total);
        }
    }

    FileUtils::LogLine("[boxart-nx] GetAppControlData(0): tid=0x%016llx, bufSize=0x%lx, outSize=0x%x, rc=0x%x",
                       (unsigned long long)tid, buffer.GetSize(), out_size.GetValue(), rc);

    return rc;
}

ams::Result NsROAppControlDataService::GetAppDesiredLanguage(u32 bitmask, ams::sf::Out<u8> out_langentry) {
    return serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::GetAppDesiredLanguage, bitmask, *out_langentry.GetPointer());
}

ams::Result NsROAppControlDataService::ConvertAppLanguageToLanguageCode(u8 langentry, ams::sf::Out<u64> langcode) {
    return serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::ConvertAppLanguageToLanguageCode, langentry, *langcode.GetPointer());
}

ams::Result NsROAppControlDataService::ConvertLanguageCodeToAppLanguage(u64 langcode, ams::sf::Out<u8> langentry) {
    return serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::ConvertLanguageCodeToAppLanguage, langcode, *langentry.GetPointer());
}

ams::Result NsROAppControlDataService::SelectApplicationDesiredLanguage(ams::sf::Out<u64> out_bytes, const ams::sf::InMapAliasBuffer &in_buffer) {
    u64 in_dummy = 0;
    return serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::SelectApplicationDesiredLanguage, in_dummy, *out_bytes.GetPointer(),
        .buffer_attrs = {SfBufferAttr_HipcMapAlias | SfBufferAttr_In},
        .buffers = {{in_buffer.GetPointer(), in_buffer.GetSize()}},
    );
}

// Command 5
ams::Result NsROAppControlDataService::GetAppControlData5(u8 source, u8 flag, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<u64> out_size) {
    const struct {
        u8 source;
        u8 flag;
        u64 tid;
    } in = {source, flag, tid};

    Result rc = serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::GetAppControlData5, in, *out_size.GetPointer(),
        .buffer_attrs = {SfBufferAttr_HipcMapAlias | SfBufferAttr_Out},
        .buffers = {{buffer.GetPointer(), buffer.GetSize()}},
    );

    if (R_SUCCEEDED(rc)) {
        u32 new_total = 0;
        if (TryReplaceIcon(tid, buffer.GetPointer(), buffer.GetSize(), &new_total)) {
            out_size.SetValue(static_cast<u64>(new_total));
        }
    }

    FileUtils::LogLine("[boxart-nx] GetAppControlData(5): tid=0x%016llx, outSize=0x%llx, rc=0x%x",
                       (unsigned long long)tid, (unsigned long long)out_size.GetValue(), rc);

    return rc;
}

// Command 6 (19.0.0+)
ams::Result NsROAppControlDataService::GetAppControlData6(u8 source, u8 flag1, u8 flag2, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<u64> out_size) {
    const struct {
        u8 source;
        u8 flag1;
        u8 flag2;
        u8 pad[5];
        u64 tid;
    } in = {source, flag1, flag2, {0}, tid};

    Result rc = serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::GetAppControlData6, in, *out_size.GetPointer(),
        .buffer_attrs = {SfBufferAttr_HipcMapAlias | SfBufferAttr_Out},
        .buffers = {{buffer.GetPointer(), buffer.GetSize()}},
    );

    if (R_SUCCEEDED(rc)) {
        u32 new_total = 0;
        if (TryReplaceIcon(tid, buffer.GetPointer(), buffer.GetSize(), &new_total)) {
            u64 current_val = out_size.GetValue();
            u32 unk = static_cast<u32>(current_val & 0xFFFFFFFF);
            out_size.SetValue((static_cast<u64>(new_total) << 32) | unk);
        }
    }

    FileUtils::LogLine("[boxart-nx] GetAppControlData(6): tid=0x%016llx, outSize=0x%llx, rc=0x%x",
                       (unsigned long long)tid, (unsigned long long)out_size.GetValue(), rc);

    return rc;
}

ams::Result NsROAppControlDataService::Unk7(u64 tid, ams::sf::Out<Struct0x80> out_bytes) {
    return serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::Unk7, tid, *out_bytes.GetPointer());
}

ams::Result NsROAppControlDataService::Unk8(Struct0x88 in_bytes, ams::sf::Out<u32> out_bytes, const ams::sf::OutMapAliasBuffer &out_buffer) {
    return serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::Unk8, in_bytes, *out_bytes.GetPointer(),
        .buffer_attrs = {SfBufferAttr_HipcMapAlias | SfBufferAttr_Out},
        .buffers = {{out_buffer.GetPointer(), out_buffer.GetSize()}},
    );
}

ams::Result NsROAppControlDataService::Unk9(u64 tid, const ams::sf::InMapAliasBuffer &in_buffer) {
    return serviceDispatchIn(this->srv.get(), NsROAppControlDataInterfaceCmdId::Unk9, tid,
        .buffer_attrs = {SfBufferAttr_HipcMapAlias | SfBufferAttr_In},
        .buffers = {{in_buffer.GetPointer(), in_buffer.GetSize()}},
    );
}

// Command 18
ams::Result NsROAppControlDataService::GetAppControlData18(u8 source, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<u64> out_size) {
    const struct {
        u8 source;
        u64 tid;
    } in = {source, tid};

    Result rc = serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::GetAppControlData18, in, *out_size.GetPointer(),
        .buffer_attrs = {SfBufferAttr_HipcMapAlias | SfBufferAttr_Out},
        .buffers = {{buffer.GetPointer(), buffer.GetSize()}},
    );

    struct OutData {
        u32 unk1;
        u32 size;
    };
    OutData* data = reinterpret_cast<OutData*>(out_size.GetPointer());

    if (R_SUCCEEDED(rc)) {
        u32 new_total = 0;
        if (TryReplaceIcon(tid, buffer.GetPointer(), buffer.GetSize(), &new_total)) {
            data->size = new_total;
        }
    }

    FileUtils::LogLine("[boxart-nx] GetAppControlData(18): tid=0x%016llx, outSize=0x%x, rc=0x%x",
                       (unsigned long long)tid, data->size, rc);

    return rc;
}

// Command 19 (Used by qlaunch 21.0.0+)
ams::Result NsROAppControlDataService::GetAppControlData19(u8 source, u8 flag, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<Struct0xC> out_size) {
    const struct {
        u8 source;
        u8 flag;
        u64 tid;
    } in = {source, flag, tid};

    Result rc = serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::GetAppControlData19, in, *out_size.GetPointer(),
        .buffer_attrs = {SfBufferAttr_HipcMapAlias | SfBufferAttr_Out},
        .buffers = {{buffer.GetPointer(), buffer.GetSize()}},
    );

    struct OutData {
        u32 unk1;
        u32 unk2;
        u32 size;
    };
    OutData* data = reinterpret_cast<OutData*>(out_size.GetPointer());

    if (R_SUCCEEDED(rc)) {
        u32 new_total = 0;
        if (TryReplaceIcon(tid, buffer.GetPointer(), buffer.GetSize(), &new_total)) {
            if (data->size >= 0x4000) {
                data->size = new_total;
            } else if (data->unk2 >= 0x4000) {
                data->unk2 = new_total;
            } else {
                data->size = new_total;
            }
        }
    }

    FileUtils::LogLine("[boxart-nx] GetAppControlData(19): tid=0x%016llx, outSize=0x%x, rc=0x%x",
                       (unsigned long long)tid, data->size, rc);

    return rc;
}

// Command 20
ams::Result NsROAppControlDataService::GetAppControlData20(u8 source, u8 flag1, u8 flag2, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<Struct0xC> out_size) {
    const struct {
        u8 source;
        u8 flag1;
        u8 flag2;
        u64 tid;
    } in = {source, flag1, flag2, tid};

    Result rc = serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::GetAppControlData20, in, *out_size.GetPointer(),
        .buffer_attrs = {SfBufferAttr_HipcMapAlias | SfBufferAttr_Out},
        .buffers = {{buffer.GetPointer(), buffer.GetSize()}},
    );

    struct OutData {
        u32 unk1;
        u32 unk2;
        u32 size;
    };
    OutData* data = reinterpret_cast<OutData*>(out_size.GetPointer());

    if (R_SUCCEEDED(rc)) {
        u32 new_total = 0;
        if (TryReplaceIcon(tid, buffer.GetPointer(), buffer.GetSize(), &new_total)) {
            if (data->size >= 0x4000) {
                data->size = new_total;
            } else if (data->unk2 >= 0x4000) {
                data->unk2 = new_total;
            } else {
                data->size = new_total;
            }
        }
    }

    FileUtils::LogLine("[boxart-nx] GetAppControlData(20): tid=0x%016llx, outSize=0x%x, rc=0x%x",
                       (unsigned long long)tid, data->size, rc);

    return rc;
}

// Command 21
ams::Result NsROAppControlDataService::GetAppControlData21(u8 source, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<Struct0xC> out_size) {
    const struct {
        u8 source;
        u64 tid;
    } in = {source, tid};

    Result rc = serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::GetAppControlData21, in, *out_size.GetPointer(),
        .buffer_attrs = {SfBufferAttr_HipcMapAlias | SfBufferAttr_Out},
        .buffers = {{buffer.GetPointer(), buffer.GetSize()}},
    );

    struct OutData {
        u32 unk1;
        u32 unk2;
        u32 size;
    };
    OutData* data = reinterpret_cast<OutData*>(out_size.GetPointer());

    if (R_SUCCEEDED(rc)) {
        u32 new_total = 0;
        if (TryReplaceIcon(tid, buffer.GetPointer(), buffer.GetSize(), &new_total)) {
            if (data->size >= 0x4000) {
                data->size = new_total;
            } else if (data->unk2 >= 0x4000) {
                data->unk2 = new_total;
            } else {
                data->size = new_total;
            }
        }
    }

    FileUtils::LogLine("[boxart-nx] GetAppControlData(21): tid=0x%016llx, outSize=0x%x, rc=0x%x",
                       (unsigned long long)tid, data->size, rc);

    return rc;
}

// Command 22
ams::Result NsROAppControlDataService::GetAppControlData22(u8 source, u8 flag1, u64 tid, const ams::sf::OutBuffer &buffer, ams::sf::Out<Struct0xC> out_size) {
    const struct {
        u8 source;
        u8 flag1;
        u64 tid;
    } in = {source, flag1, tid};

    Result rc = serviceDispatchInOut(this->srv.get(), NsROAppControlDataInterfaceCmdId::GetAppControlData22, in, *out_size.GetPointer(),
        .buffer_attrs = {SfBufferAttr_HipcMapAlias | SfBufferAttr_Out},
        .buffers = {{buffer.GetPointer(), buffer.GetSize()}},
    );

    struct OutData {
        u32 unk1;
        u32 unk2;
        u32 size;
    };
    OutData* data = reinterpret_cast<OutData*>(out_size.GetPointer());

    if (R_SUCCEEDED(rc)) {
        u32 new_total = 0;
        if (TryReplaceIcon(tid, buffer.GetPointer(), buffer.GetSize(), &new_total)) {
            if (data->size >= 0x4000) {
                data->size = new_total;
            } else if (data->unk2 >= 0x4000) {
                data->unk2 = new_total;
            } else {
                data->size = new_total;
            }
        }
    }

    FileUtils::LogLine("[boxart-nx] GetAppControlData(22): tid=0x%016llx, outSize=0x%x, rc=0x%x",
                       (unsigned long long)tid, data->size, rc);

    return rc;
}
