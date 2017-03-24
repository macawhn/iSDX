#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "bgp.h"

#ifndef SAFE_FREE
#define SAFE_FREE(ptr)     {if (NULL != (ptr)) {free(ptr); (ptr)=NULL;}}
#endif

char *my_strdup(const char *s)
{
    int l = strlen(s);
    char *d = malloc(l + 1);
    if (!d) return NULL;
    memcpy(d, s, l);
    d[l] = '\0';
    return d;
}

void free_route_ptr(route_t **pp_route)
{
    if (!pp_route || !*pp_route) {
        return;
    }
    SAFE_FREE((*pp_route)->prefix);
    SAFE_FREE((*pp_route)->neighbor);
    SAFE_FREE((*pp_route)->next_hop);
    SAFE_FREE((*pp_route)->origin);
    SAFE_FREE((*pp_route)->as_path.asns);
    SAFE_FREE((*pp_route)->communities);
    SAFE_FREE(*pp_route);
}

void free_route(route_t *p_route)
{
    if (!p_route) {
        return;
    }
    SAFE_FREE(p_route->prefix);
    SAFE_FREE(p_route->neighbor);
    SAFE_FREE(p_route->next_hop);
    SAFE_FREE(p_route->origin);
    SAFE_FREE(p_route->as_path.asns);
    SAFE_FREE(p_route->communities);
}

void free_resp_dec_msg(resp_dec_msg_t *p_resp_dec_msg)
{
    SAFE_FREE(p_resp_dec_msg->prefix);
    SAFE_FREE(p_resp_dec_msg->next_hop);
    SAFE_FREE(p_resp_dec_msg->as_path.asns);
}

void print_route(route_t *p_route)
{
    int i;
    if (!p_route) {
        return;
    }
    printf("%s,%s,%s,%s,", p_route->prefix, p_route->neighbor, p_route->next_hop, p_route->origin);
    if (!p_route->as_path.length) {
        printf(" ,");
    } else {
        for (i = 0; i < p_route->as_path.length - 1; i++) {
            printf("%d ", p_route->as_path.asns[i]);
        }
        printf("%d,", p_route->as_path.asns[i]);
    }
    printf("%s,%d,%d\n", p_route->communities, p_route->med, p_route->atomic_aggregate);
}

void parse_as_path_from_file(as_path_t *p_as_path, char *p_s_as_path)
{
    int delimiter_count = 0, i;
    char *token, *p_save, *p_s_as_path_tmp;
    char *delimiter = AS_PATH_DELIMITER_STR;

    if (!p_as_path || !p_s_as_path) {
        return;
    }

    if (strlen(p_s_as_path) == 1 && p_s_as_path[0] == AS_PATH_DELIMITER_CHAR) {
        p_as_path->length = 0;
        p_as_path->asns = NULL;
        return;
    }

    p_s_as_path_tmp = p_s_as_path;
    // ensure that we have correct delimiter number in the input
    while (*p_s_as_path_tmp) {
        delimiter_count += (*p_s_as_path_tmp++ == AS_PATH_DELIMITER_CHAR);
    }

    p_as_path->length = delimiter_count + 1;
    p_as_path->asns = malloc(sizeof(*p_as_path->asns) * (p_as_path->length));

    token = strtok_r(p_s_as_path, delimiter, &p_save);
    p_as_path->asns[0] = atoi(token);
    for (i = 1; i < p_as_path->length; i++) {
        token = strtok_r(0, delimiter, &p_save);
        p_as_path->asns[i] = atoi(token);
    }
}

void parse_route_from_file(route_t **pp_route, char *p_s_route)
{
    int delimiter_count = 0;
    char *token, *p_save, *p_s_route_tmp;
    char *delimiter = ROUTE_DELIMITER_STR;

    if (!pp_route || *pp_route || !p_s_route) {
        return;
    }

    // ensure that we have correct delimiter number in the input
    p_s_route_tmp = p_s_route;
    while (*p_s_route_tmp) {
        delimiter_count += (*p_s_route_tmp++ == ROUTE_DELIMITER_CHAR);
    }
    assert(delimiter_count == ROUTE_FIELD - 1);

    *pp_route = malloc(sizeof(route_t));
    if (!*pp_route) {
        return;
    }

    token = strtok_r(p_s_route, delimiter, &p_save);
    (*pp_route)->prefix = my_strdup(token);
    token = strtok_r(0, delimiter, &p_save);
    (*pp_route)->neighbor = my_strdup(token);
    token = strtok_r(0, delimiter, &p_save);
    (*pp_route)->next_hop = my_strdup(token);
    token = strtok_r(0, delimiter, &p_save);
    (*pp_route)->origin = my_strdup(token);
    token = strtok_r(0, delimiter, &p_save);
    parse_as_path_from_file(&(*pp_route)->as_path, token);
    token = strtok_r(0, delimiter, &p_save);
    (*pp_route)->communities = my_strdup(token);
    token = strtok_r(0, delimiter, &p_save);
    (*pp_route)->med = atoi(token);
    token = strtok_r(0, delimiter, &p_save);
    (*pp_route)->atomic_aggregate = atoi(token);
}

int parse_route_from_stream(route_t **pp_route, uint8_t *p_s_route)
{
    int offset = 0;
    uint32_t size = 0;

    if (!pp_route || *pp_route || !p_s_route) {
        return 0;
    }

    *pp_route = malloc(sizeof(route_t));
    if (!*pp_route) {
        return 0;
    }

    size = *((uint8_t *) p_s_route);
    offset++;
    (*pp_route)->prefix = malloc(size + 1);
    memcpy((*pp_route)->prefix, p_s_route + offset, size);
    (*pp_route)->prefix[size] = '\0';
    offset += size;

    size = *((uint8_t *) p_s_route + offset);
    offset++;
    (*pp_route)->neighbor = malloc(size + 1);
    memcpy((*pp_route)->neighbor, p_s_route + offset, size);
    (*pp_route)->neighbor[size] = '\0';
    offset += size;

    size = *((uint8_t *) p_s_route + offset);
    offset++;
    (*pp_route)->next_hop = malloc(size + 1);
    memcpy((*pp_route)->next_hop, p_s_route + offset, size);
    (*pp_route)->next_hop[size] = '\0';
    offset += size;

    size = *((uint8_t *) p_s_route + offset);
    offset++;
    (*pp_route)->origin = malloc(size + 1);
    memcpy((*pp_route)->origin, p_s_route + offset, size);
    (*pp_route)->origin[size] = '\0';
    offset += size;

    size = *((uint32_t *) (p_s_route + offset));
    offset += 4;
    (*pp_route)->as_path.length = size / sizeof(int);
    (*pp_route)->as_path.asns = malloc(size);
    memcpy((*pp_route)->as_path.asns, p_s_route + offset, size);
    offset += size;

    size = *((uint8_t *) p_s_route + offset);
    offset++;
    (*pp_route)->communities = malloc(size + 1);
    memcpy((*pp_route)->communities, p_s_route + offset, size);
    (*pp_route)->communities[size] = '\0';
    offset += size;

    (*pp_route)->med = *((int *) (p_s_route + offset));
    offset += sizeof(int);
    (*pp_route)->atomic_aggregate = *((int *) (p_s_route + offset));
    offset += sizeof(int);
    return offset;
}

int get_route_size(route_t *r)
{
    int route_size = 0;
    if (!r) return route_size;
    route_size += strlen(r->prefix);
    route_size += strlen(r->neighbor);
    route_size += strlen(r->next_hop);
    route_size += strlen(r->origin);
    route_size += sizeof(int) * r->as_path.length;
    route_size += strlen(r->communities);
    route_size += sizeof(int);          // med
    route_size += sizeof(int);          // atomic_aggregate
    route_size += 9;                    // header count

    return route_size;
}

int write_route_to_existed_stream(uint8_t *route, route_t *input)
{
    if (!route || !input) return 0;
    int offset = 0;

    *((uint8_t *) route) = (uint8_t) strlen(input->prefix);
    offset++;
    memcpy(route + offset, input->prefix, strlen(input->prefix));
    offset += strlen(input->prefix);

    *((uint8_t *) route + offset) = (uint8_t) strlen(input->neighbor);
    offset++;
    memcpy(route + offset, input->neighbor, strlen(input->neighbor));
    offset += strlen(input->neighbor);

    *((uint8_t *) route + offset) = (uint8_t) strlen(input->next_hop);
    offset++;
    memcpy(route + offset, input->next_hop, strlen(input->next_hop));
    offset += strlen(input->next_hop);

    *((uint8_t *) route + offset) = (uint8_t) strlen(input->origin);
    offset++;
    memcpy(route + offset, input->origin, strlen(input->origin));
    offset += strlen(input->origin);

    *((uint32_t *) (route + offset)) = sizeof(*input->as_path.asns) * input->as_path.length;
    offset += 4;
    if (input->as_path.length) {
        memcpy(route + offset, input->as_path.asns, sizeof(*input->as_path.asns) * input->as_path.length);
        offset += sizeof(*input->as_path.asns) * input->as_path.length;
    }

    *((uint8_t *) route + offset) = (uint8_t) strlen(input->communities);
    offset++;
    memcpy(route + offset, input->communities, strlen(input->communities));
    offset += strlen(input->communities);

    memcpy(route + offset, &input->med, sizeof(input->med));
    offset += sizeof(input->med);

    memcpy(route + offset, &input->atomic_aggregate, sizeof(input->atomic_aggregate));
    offset += sizeof(input->atomic_aggregate);
    return offset;
}

int write_route_to_stream(uint8_t **pp_msg, route_t *input)
{
    if (!pp_msg || *pp_msg || !input) return 0;

    int route_size = get_route_size(input);
    *pp_msg = malloc(route_size);
    return write_route_to_existed_stream(*pp_msg, input);
}

int parse_resp_from_stream(resp_dec_msg_t **pp_resp_dec_msg, size_t *p_resp_msg_num, uint8_t *p_msg)
{
    if (!pp_resp_dec_msg || *pp_resp_dec_msg || !p_resp_msg_num) {
        return 0;
    }

    uint32_t i = 0, offset = 0, tmp_size;
    *p_resp_msg_num = *((uint32_t *) (p_msg + offset));
    offset += 4;
    *pp_resp_dec_msg = malloc(*p_resp_msg_num * sizeof **pp_resp_dec_msg);
    for (i = 0; i < *p_resp_msg_num; i++) {
        // asn
        (*pp_resp_dec_msg)[i].asn = *((uint32_t *) (p_msg + offset));
        offset += 4;
        // oprt_type
        (*pp_resp_dec_msg)[i].oprt_type = *(p_msg + offset);
        offset++;
        // prefix
        tmp_size = *(p_msg + offset);
        offset++;
        (*pp_resp_dec_msg)[i].prefix = malloc(tmp_size + 1);
        memcpy((*pp_resp_dec_msg)[i].prefix, p_msg + offset, tmp_size);
        offset += tmp_size;
        (*pp_resp_dec_msg)[i].prefix[tmp_size] = '\0';
        // next_hop
        tmp_size = *(p_msg + offset);
        offset++;
        (*pp_resp_dec_msg)[i].next_hop = malloc(tmp_size + 1);
        memcpy((*pp_resp_dec_msg)[i].next_hop, p_msg + offset, tmp_size);
        offset += tmp_size;
        (*pp_resp_dec_msg)[i].next_hop[tmp_size] = '\0';
        if ((*pp_resp_dec_msg)[i].oprt_type == WITHDRAW) continue;
        // as_path
        (*pp_resp_dec_msg)[i].as_path.length = *((uint32_t *) (p_msg + offset));
        offset += 4;
        tmp_size = (*pp_resp_dec_msg)[i].as_path.length * sizeof *(*pp_resp_dec_msg)[i].as_path.asns;
        (*pp_resp_dec_msg)[i].as_path.asns = malloc(tmp_size);
        memcpy((*pp_resp_dec_msg)[i].as_path.asns, p_msg + offset, tmp_size);
        offset += tmp_size;
    }
    return offset;
}

int write_resp_to_stream(uint8_t **pp_msg, resp_dec_msg_t *p_resp_dec_msgs, size_t resp_msg_num)
{
    if (!pp_msg || !p_resp_dec_msgs) {
        return 0;
    }

    uint32_t i = 0, offset = 0;
    size_t resp_msg_size = 0;

    // serialize the response message
    resp_msg_size += 4; // resp_msg_num (4)
    for (i = 0; i < resp_msg_num; i++) {
        resp_msg_size += 5; // asn (4) + oprt_type (1)
        resp_msg_size += 2; // prefix_size (1) + next_hop_size (1)
        resp_msg_size += strlen(p_resp_dec_msgs[i].prefix);
        resp_msg_size += strlen(p_resp_dec_msgs[i].next_hop);
        if (p_resp_dec_msgs[i].oprt_type == ANNOUNCE) {
            resp_msg_size += 4;  // as_path_length (4)
            resp_msg_size += p_resp_dec_msgs[i].as_path.length * sizeof *p_resp_dec_msgs[i].as_path.asns;
        }
    }
    *pp_msg = malloc(resp_msg_size);
    *((uint32_t *) (*pp_msg + offset)) = resp_msg_num;
    offset += 4;
    for (i = 0; i < resp_msg_num; i++) {
        *((uint32_t *) (*pp_msg + offset)) = p_resp_dec_msgs[i].asn;
        offset += 4;
        *(*pp_msg + offset) = p_resp_dec_msgs[i].oprt_type;
        offset++;
        *(*pp_msg + offset) = (uint8_t) strlen(p_resp_dec_msgs[i].prefix);
        offset++;
        memcpy(*pp_msg + offset, p_resp_dec_msgs[i].prefix, strlen(p_resp_dec_msgs[i].prefix));
        offset += strlen(p_resp_dec_msgs[i].prefix);
        *(*pp_msg + offset) = (uint8_t) strlen(p_resp_dec_msgs[i].next_hop);
        offset++;
        memcpy(*pp_msg + offset, p_resp_dec_msgs[i].next_hop, strlen(p_resp_dec_msgs[i].next_hop));
        offset += strlen(p_resp_dec_msgs[i].next_hop);
        if (p_resp_dec_msgs[i].oprt_type == ANNOUNCE) {
            *((uint32_t *) (*pp_msg + offset)) = p_resp_dec_msgs[i].as_path.length;
            offset += 4;
            memcpy(*pp_msg + offset, p_resp_dec_msgs[i].as_path.asns, p_resp_dec_msgs[i].as_path.length * sizeof *p_resp_dec_msgs[i].as_path.asns);
            offset += p_resp_dec_msgs[i].as_path.length * sizeof *p_resp_dec_msgs[i].as_path.asns;
        }
    }
    assert(offset == resp_msg_size);
    return offset;
}

// ret < 0: r2 is better, ret > 0: r1 is better
// if r1 is the same as r2, then we return r1 > r2
// r1 should be the old best route to prefer recent route
int _route_cmp(route_t *r1, route_t *r2)
{
    /*------- lowest path length -------*/
    if (r1->as_path.length > r2->as_path.length) {
        return -1;
    } else if (r1->as_path.length < r2->as_path.length) {
        return 1;
    } else {
        /*------- lowest med -------*/
        if (r1->med > r2->med) {
            return -1;
        } else if (r1->med < r2->med) {
            return 1;
        } else {
            /*------- lowest next_hop -------*/
            if (strcmp(r1->next_hop, r2->next_hop) > 0) {
                return -1;
            } else {
                return 1;
            }
        }
    }
}

void route_cpy(route_t **dst_route, uint32_t *src_asn, route_t *src_route)
{
    if (!dst_route) return;
    *dst_route = malloc(sizeof **dst_route);
    (*dst_route)->prefix = my_strdup(src_route->prefix);
    (*dst_route)->neighbor = my_strdup(src_route->neighbor);
    (*dst_route)->next_hop = my_strdup(src_route->next_hop);
    (*dst_route)->origin = my_strdup(src_route->origin);
    (*dst_route)->as_path.length = src_asn ? src_route->as_path.length + 1 : src_route->as_path.length;
    (*dst_route)->as_path.asns = malloc((*dst_route)->as_path.length * sizeof(int));
    if (src_asn) {
        (*dst_route)->as_path.asns[0] = *src_asn;
        memcpy((*dst_route)->as_path.asns + 1, src_route->as_path.asns, src_route->as_path.length * sizeof(int));
    } else {
        memcpy((*dst_route)->as_path.asns, src_route->as_path.asns, src_route->as_path.length * sizeof(int));
    }
    (*dst_route)->communities = my_strdup(src_route->communities);
    (*dst_route)->med = src_route->med;
    (*dst_route)->atomic_aggregate = src_route->atomic_aggregate;
}

route_node_t* get_selected_route_node(route_node_t *p_rns)
{
    if (!p_rns) {
        return NULL;
    }
    route_node_t *p_tmp = p_rns;
    while (p_tmp) {
        if (p_tmp->is_selected == 1) {
            return p_tmp;
        } else {
            p_tmp = p_tmp->next;
        }
    }
    return NULL;
}

void add_route(route_node_t **pp_rns, uint32_t src_asn, route_t *src_route, uint8_t *import_policy)
{
    int ret;
    if (!pp_rns) return;

    // create new route node
    route_node_t *p_rn = malloc(sizeof *p_rn);
    p_rn->is_selected = 0;
    p_rn->next_hop = src_asn;
    p_rn->prev = NULL;
    p_rn->next = NULL;
    route_cpy(&p_rn->route, NULL, src_route);

    // add new route node to the list
    if (!*pp_rns) {
        *pp_rns = p_rn;
        p_rn->is_selected = 1;
        return;
    }
    p_rn->next = *pp_rns;
    (*pp_rns)->prev = p_rn;
    *pp_rns = p_rn;

    route_node_t *tmp_rn = get_selected_route_node(*pp_rns);
    assert(tmp_rn);
    ret = import_policy[p_rn->next_hop] - import_policy[tmp_rn->next_hop];
    if (ret < 0) {
        tmp_rn->is_selected = 0;
        p_rn->is_selected = 1;
    } else if (ret > 0) {
        return;
    } else {
        if (_route_cmp(tmp_rn->route, p_rn->route) < 0) {
            tmp_rn->is_selected = 0;
            p_rn->is_selected = 1;
        } else {
            return;
        }
    }
}

void del_route(route_node_t **pp_rns, uint32_t src_asn, route_t *src_route, uint8_t *import_policy, route_node_t *p_old_best_rn)
{
    if (!pp_rns) return;
    if (!*pp_rns) return;
    int del_best_rn_flag = 0, ret = 0;

    // traverse and delete
    route_node_t *tmp_rn = *pp_rns;
    while (tmp_rn) {
        if (tmp_rn->next_hop == src_asn) {
            if (tmp_rn->prev && tmp_rn->next) {
                tmp_rn->prev->next = tmp_rn->next;
                tmp_rn->next->prev = tmp_rn->prev;
            } else if (tmp_rn->next) {
                tmp_rn->next->prev = tmp_rn->prev;
                *pp_rns = tmp_rn->next;
            } else if (tmp_rn->prev) {
                tmp_rn->prev->next = tmp_rn->next;
            } else {
                *pp_rns = NULL;
            }
            if (tmp_rn->is_selected == 1) del_best_rn_flag = 1;
            if (tmp_rn != p_old_best_rn) {
                free_route_ptr(&tmp_rn->route);
                SAFE_FREE(tmp_rn);
            } else {
                // p_old_best_rn will be freed after a whole iteration
                // p_old_best_rn is needed to construct the inner msg
                tmp_rn->is_selected = TO_BE_DEL;
            }
            break;
        } else {
            tmp_rn = tmp_rn->next;
        }
    }
    if (!del_best_rn_flag) return;

    // the best route node has been deleted, select a new one
    route_node_t *cur_best_rn = *pp_rns;
    if (!cur_best_rn) return;
    tmp_rn = cur_best_rn->next;
    while (tmp_rn) {
        ret = import_policy[cur_best_rn->next_hop] - import_policy[tmp_rn->next_hop];
        if (ret > 0 || (ret = 0 && _route_cmp(cur_best_rn->route, tmp_rn->route) < 0)) {
            cur_best_rn = tmp_rn;
        }
        tmp_rn = tmp_rn->next;
    }
    cur_best_rn->is_selected = 1;
}

void execute_export_policy(rs_inner_msg_t **pp_inner_msgs, uint32_t num, uint8_t *export_policy, uint32_t src_asn, uint32_t src_next_hop, uint8_t oprt_type, route_t *src_route)
{
    uint32_t dst_asn;
    rs_inner_msg_t *p_inner_msg = NULL;
    for (dst_asn = 0; dst_asn < num; dst_asn++) {
        if (export_policy[src_next_hop * num + dst_asn]) {
            p_inner_msg = malloc(sizeof *p_inner_msg);
            p_inner_msg->src_asn = src_asn;
            p_inner_msg->oprt_type = oprt_type;
            p_inner_msg->src_route = NULL;
            p_inner_msg->prev = NULL;
            p_inner_msg->next = NULL;
            if (src_route) {
                if (src_next_hop == src_asn) {
                    route_cpy(&p_inner_msg->src_route, NULL, src_route);
                } else {
                    route_cpy(&p_inner_msg->src_route, &src_asn, src_route);
                }
            }
            if (pp_inner_msgs[dst_asn]) {
                pp_inner_msgs[dst_asn]->prev->next = p_inner_msg;
                p_inner_msg->prev = pp_inner_msgs[dst_asn]->prev;
                p_inner_msg->next = pp_inner_msgs[dst_asn];
                pp_inner_msgs[dst_asn]->prev = p_inner_msg;
                pp_inner_msgs[dst_asn] = p_inner_msg;
            } else {
                pp_inner_msgs[dst_asn] = p_inner_msg;
                pp_inner_msgs[dst_asn]->prev = p_inner_msg;
                pp_inner_msgs[dst_asn]->next = p_inner_msg;
            }
        }
    }
}
