/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#pragma once

#include <stratosphere.hpp>

#define AMS_CONCAT_NAMESPACE(A, B) A::B

#define AMS_SF_METHOD_INFO_F(CLASSNAME, HANDLER, ENUM_TYPE, NAME, ARGS, ARGNAMES) \
	AMS_SF_METHOD_INFO_9(CLASSNAME, HANDLER, (u32)AMS_CONCAT_NAMESPACE(ENUM_TYPE, NAME), ams::Result, NAME, ARGS, ARGNAMES, ams::hos::Version_Min, ams::hos::Version_Max)

#define AMS_SF_DECLARE_INTERFACE_METHODS(CLASSNAME, CMD_ID, RETURN, NAME, ARGS, ARGNAMES, VERSION_MIN, VERSION_MAX) \
	RETURN NAME ARGS;

#define AMS_SF_DEFINE_INTERFACE_F(NAME, MACRO, INTF_ID) \
	AMS_SF_DEFINE_INTERFACE(ams::sf_interface, NAME, MACRO, INTF_ID); \
	using AMS_CONCAT_NAMESPACE(::ams::sf_interface, NAME); \
	using AMS_CONCAT_NAMESPACE(::ams::sf_interface, Is##NAME)

#define AMS_SF_DEFINE_MITM_INTERFACE_F(NAME, MACRO, INTF_ID) \
	AMS_SF_DEFINE_MITM_INTERFACE(ams::sf_mitm_interface, NAME, MACRO, INTF_ID); \
	using AMS_CONCAT_NAMESPACE(::ams::sf_mitm_interface, NAME); \
	using AMS_CONCAT_NAMESPACE(::ams::sf_mitm_interface, Is##NAME)
