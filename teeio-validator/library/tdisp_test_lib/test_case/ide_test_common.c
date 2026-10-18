/**
 *  Copyright Notice:
 *  Copyright 2024 Intel. All rights reserved.
 *  License: BSD 3-Clause License.
 **/

#include "ide_test.h"
#include "pcie_ide_lib.h"
#include "tdisp_test_internal.h"
#include "teeio_debug.h"

bool setup_ide_stream (void *doe_context, void *spdm_context, uint32_t *session_id,
	uint8_t *kcbar_addr, uint8_t stream_id, uint8_t ks, ide_key_set_t *k_set,
	uint8_t rp_stream_index, uint8_t port_index, IDE_TEST_TOPOLOGY_TYPE top_type,
	ide_common_test_port_context_t *upper_port, ide_common_test_port_context_t *lower_port,
	bool skip_ksetgo);

bool tdisp_test_set_ide_stream (void *test_context, uint8_t ks)
{
	assert_context (test_context);

	ide_common_test_case_context_t *case_context =
		(ide_common_test_case_context_t*) test_context;
	pcie_ide_test_group_context_t *group_context = case_context->group_context;

	// Setup parameters
	void *doe_context = group_context->spdm_doe.doe_context;
	void *spdm_context = group_context->spdm_doe.spdm_context;
	uint32_t *session_id = &group_context->spdm_doe.session_id;
	uint8_t *kcbar_addr = group_context->common.upper_port.mapped_kcbar_addr;
	uint8_t stream_id = group_context->stream_id;
	ide_key_set_t *k_set = &group_context->k_set;
	uint8_t rp_stream_index = group_context->rp_stream_index;
	uint8_t port_index = 0;
	IDE_TEST_TOPOLOGY_TYPE top_type = IDE_TEST_TOPOLOGY_TYPE_SEL_IDE;
	ide_common_test_port_context_t *upper_port = &group_context->common.upper_port;
	ide_common_test_port_context_t *lower_port = &group_context->common.lower_port;

	if (!lower_port->ide_cap.tee_limited_stream_supported) {
		TEEIO_DEBUG ((TEEIO_DEBUG_ERROR,
			"TDISP endpoint does not advertise TEE-Limited Selective IDE support.\n"));
		return false;
	}

	if (!set_tee_limited_in_ecap (lower_port->cfg_space_fd,
			TEST_IDE_TYPE_SEL_IDE, lower_port->ide_id,
			lower_port->ecap_offset, true)) {
		return false;
	}

	bool result = setup_ide_stream (doe_context, spdm_context, session_id,
		kcbar_addr, stream_id, ks, k_set, rp_stream_index, port_index,
		top_type, upper_port, lower_port, false);
	if (!result) {
		set_tee_limited_in_ecap (lower_port->cfg_space_fd,
			TEST_IDE_TYPE_SEL_IDE, lower_port->ide_id,
			lower_port->ecap_offset, false);
		return false;
	}

	PCIE_SEL_IDE_STREAM_CTRL rp_ctrl = {
		.raw = read_ide_stream_ctrl_in_ecap (upper_port->cfg_space_fd,
			TEST_IDE_TYPE_SEL_IDE, upper_port->ide_id,
			upper_port->ecap_offset)
	};
	PCIE_SEL_IDE_STREAM_CTRL dev_ctrl = {
		.raw = read_ide_stream_ctrl_in_ecap (lower_port->cfg_space_fd,
			TEST_IDE_TYPE_SEL_IDE, lower_port->ide_id,
			lower_port->ecap_offset)
	};

	if (!dev_ctrl.tee_limited_stream) {
		TEEIO_DEBUG ((TEEIO_DEBUG_ERROR,
			"TDISP endpoint TEE-Limited mode was not preserved: "
			"rp=0x%08x dev=0x%08x\n",
			rp_ctrl.raw, dev_ctrl.raw));
		return false;
	}

	TEEIO_PRINT (("TDISP endpoint TEE-Limited Selective IDE enabled: "
		"rp_ctrl=0x%08x dev_ctrl=0x%08x\n", rp_ctrl.raw, dev_ctrl.raw));
	return true;
}
