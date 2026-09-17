#pragma once
#include <sys/types.h>

int setup_network_namespaces(const char* veth_host, const char* veth_cont, pid_t container_pid);

int netlink_link_set_up(const char* ifname);
int netlink_link_set_master(const char* ifname, const char* master_name);
int netlink_add_ip(const char* ifname, const char* ip_str, unsigned char prefix);
int netlink_add_default_route(const char* ifname, const char* gw_str);
