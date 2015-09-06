#!/usr/bin/env python
#  Author:
#  Arpit Gupta (arpitg@cs.princeton.edu)

import json
import os
from random import shuffle, randint
import sys

fname = "rrc03.bview.20150820.0000.temp.txt"

"""
TIME: 08/20/15 00:00:00
TYPE: TABLE_DUMP_V2/IPV4_UNICAST
PREFIX: 1.0.0.0/24
SEQUENCE: 0
FROM: 80.249.211.161 AS8283
ORIGINATED: 08/13/15 13:16:25
ORIGIN: IGP
ASPATH: 8283 5580 15169
NEXT_HOP: 80.249.211.161
MULTI_EXIT_DISC: 0
COMMUNITY: 5580:12533 8283:13
"""

def getMatchHash(part, peer, count):
    if "AS" in part: part = int(part.split("AS")[1])
    if "AS" in peer: peer = int(peer.split("AS")[1])

    return int(1*part+1*peer+count)

def generatePoliciesParticipant(part, asn_2_ip, count, limit_out, limit_in):
    # randomly select fwding participants
    peers = filter(lambda x: x!=part, asn_2_ip.keys())
    shuffle(peers)
    fwding_peers = set(peers[:count])

    # Generate Outbound policies
    cookie_id = 1
    policy = {}
    policy["outbound"] = []
    for peer in fwding_peers:

        peer_count = randint(1, limit_out)
        for ind in range(1, peer_count+1):
            tmp_policy = {}

            # Assign Cookie ID
            tmp_policy["cookie"] = cookie_id
            cookie_id += 1

            # Match
            match_hash = getMatchHash(part, peer, ind)
            tmp_policy["match"] = {}
            tmp_policy["match"]["tcp_dst"] = match_hash
            tmp_policy["match"]["in_port"] = asn_2_ip[part].values()[0]

            # Action: fwd to peer's first port (visible to part)
            tmp_policy["action"] = {"fwd":asn_2_ip[peer].values()[0]}

            # Add this to participants' outbound policies
            policy["outbound"].append(tmp_policy)

    # Dump the policies to appropriate directory
    policy_filename = "participant_"+part+".py"
    policy_path = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), "policies"))
    policy_file = os.path.join(policy_path, policy_filename)
    with open(policy_file,'w') as f:
        json.dump(policy, f)


def getParticipants():
    asn_2_ip = {}

    with open(fname, 'r') as f:
        print "Loaded the RIB file"
        for line in f.readlines():
            if "FROM" in line:
                line
                tmp = line.split("FROM: ")[1].split("\n")[0].split(" ")
                #print tmp
                if tmp[1] not in asn_2_ip:
                    asn_2_ip[tmp[1]] = {}
                asn_2_ip[tmp[1]][tmp[0]] = 0

    print asn_2_ip
    print "Assigning Ports"
    port_id = 10
    for part in asn_2_ip:
        for ip in asn_2_ip[part]:
            asn_2_ip[part][ip] = port_id
            port_id += 1

    out_fname = "asn_2_ip.json"
    with open(out_fname,'w') as f:
        json.dump(asn_2_ip, f)

    return asn_2_ip


def generate_global_config(asn_2_ip):
    # load the base config
    config_filename = "sdx_global.cfg"
    config_path = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), "config"))
    config_file = os.path.join(config_path, config_filename)
    #print "config file: ", config_file
    with open(config_file, 'r') as f:
        config = json.load(f)
        config["Participants"] = {}
        eh_port = 7777

        for part in asn_2_ip:
            config["Participants"][part] = {}
            config["Participants"][part]["Ports"] = []
            for nhip in asn_2_ip[part]:
                tmp = {}
                tmp["Id"] = asn_2_ip[part][nhip]
                tmp["MAC"] = ""
                tmp["IP"] = str(nhip)
            config["Participants"][part]["ASN"] = part
            config["Participants"][part]["Peers"] = filter(lambda x: x!=part, asn_2_ip.keys())
            config["Participants"][part]["Inbound Rules"] = "true"
            config["Participants"][part]["Outbound Rules"] = "true"
            config["Participants"][part]["EH_SOCKET"] = ["localhost", eh_port]
            config["Participants"][part]["Flanc Key"] = "Part"+str(part)+"Key"
            eh_port += 1

        config["RefMon Settings"]["fabric connections"]["main"] = {}
        config["RefMon Settings"]["fabric connections"]["main"]["inbound"] = 1
        config["RefMon Settings"]["fabric connections"]["main"]["outbound"] = 2
        config["RefMon Settings"]["fabric connections"]["main"]["route server"] = 3
        config["RefMon Settings"]["fabric connections"]["main"]["arp proxy"] = 4
        config["RefMon Settings"]["fabric connections"]["main"]["refmon"] = 5


        for part in asn_2_ip:
            config["RefMon Settings"]["fabric connections"]["main"][part] = asn_2_ip[part].values()

        with open(config_file, "w") as f:
            json.dump(config, f)


''' main '''
if __name__ == '__main__':
    # Params
    count = 10
    limit_out = 4
    limit_in = 2

    # Parse ribs to extract asn_2_ip
    #asn_2_ip = getParticipants()

    #asn_2_ip = {"AS1":"1","AS2":"2","AS3":"3"}
    #asn_2_ports = {"AS1":[1], "AS2":[2,3], "AS3":[4,5]}
    asn_2_ip = json.load(open("asn_2_ip.json", 'r'))

    for part in asn_2_ip:
        generatePoliciesParticipant(part, asn_2_ip, count, limit_out, limit_in)

    generate_global_config(asn_2_ip)
