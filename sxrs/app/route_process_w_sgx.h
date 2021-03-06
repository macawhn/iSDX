#ifndef __ROUTE_PROCESS_W_SGX_H__
#define __ROUTE_PROCESS_W_SGX_H__

#include "shared_types.h"
#include "bgp.h"

void init_w_sgx(as_cfg_t *p_as_cfg, int verbose);

void process_bgp_route_w_sgx(const bgp_route_input_dsrlz_msg_t *p_bgp_dsrlz_msg);

void process_sdn_reach_w_sgx(uint32_t asid, const uint32_t *p_reach, uint32_t reach_size, uint8_t oprt_type);

void get_sdn_reach_by_prefix_w_sgx(uint32_t asid, const char *prefix);

#endif
