#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <libmnl/libmnl.h>
#include <linux/rtnetlink.h>
#include <linux/veth.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "logger.h"
#include "network_configuration.h"

int setup_network_namespaces(const char* veth_host, const char* veth_cont, pid_t container_pid) {
    struct mnl_socket *nl = NULL;
    char buf[MNL_SOCKET_BUFFER_SIZE];
    struct nlmsghdr *nlh;
    struct ifinfomsg *ifm;
    int ret = -1;

    LOG_SYSERR_AND_CLEANUP(nl = mnl_socket_open(NETLINK_ROUTE), NULL);
    LOG_SYSERR_AND_CLEANUP(mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID), -1);

    nlh = mnl_nlmsg_put_header(buf);
    nlh->nlmsg_type = RTM_NEWLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
    nlh->nlmsg_seq = time(NULL);

    ifm = mnl_nlmsg_put_extra_header(nlh, sizeof(struct ifinfomsg));
    ifm->ifi_family = AF_UNSPEC;

    mnl_attr_put_str(nlh, IFLA_IFNAME, veth_host);

    struct nlattr *nest_linkinfo = mnl_attr_nest_start(nlh, IFLA_LINKINFO);
    mnl_attr_put_str(nlh, IFLA_INFO_KIND, "veth");

    struct nlattr *nest_infodata = mnl_attr_nest_start(nlh, IFLA_INFO_DATA);
    struct nlattr *nest_peer = mnl_attr_nest_start(nlh, VETH_INFO_PEER);
    
    struct ifinfomsg *ifm_peer = mnl_nlmsg_put_extra_header(nlh, sizeof(struct ifinfomsg));
    ifm_peer->ifi_family = AF_UNSPEC;
    
    mnl_attr_put_str(nlh, IFLA_IFNAME, veth_cont);

    mnl_attr_put_u32(nlh, IFLA_NET_NS_PID, container_pid);

    mnl_attr_nest_end(nlh, nest_peer);
    mnl_attr_nest_end(nlh, nest_infodata);
    mnl_attr_nest_end(nlh, nest_linkinfo);

    LOG_SYSERR_AND_CLEANUP(mnl_socket_sendto(nl, nlh, nlh->nlmsg_len), -1);

    ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
    LOG_SYSERR_AND_CLEANUP(ret, -1);
    
    ret = mnl_cb_run(buf, ret, nlh->nlmsg_seq, mnl_socket_get_portid(nl), NULL, NULL);
    LOG_SYSERR_AND_CLEANUP(ret, -1);

    mnl_socket_close(nl);
    return 0;

cleanup:
    if (nl != NULL) mnl_socket_close(nl);
    return -1;
}

static int netlink_talk(struct nlmsghdr *nlh) {
    struct mnl_socket *nl = NULL;
    char rcv_buf[MNL_SOCKET_BUFFER_SIZE];
    int ret = -1;

    LOG_SYSERR_AND_CLEANUP(nl = mnl_socket_open(NETLINK_ROUTE), NULL);
    LOG_SYSERR_AND_CLEANUP(mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID), -1);

    LOG_SYSERR_AND_CLEANUP(mnl_socket_sendto(nl, nlh, nlh->nlmsg_len), -1);

    ret = mnl_socket_recvfrom(nl, rcv_buf, sizeof(rcv_buf));
    LOG_SYSERR_AND_CLEANUP(ret, -1);

    ret = mnl_cb_run(rcv_buf, ret, nlh->nlmsg_seq, mnl_socket_get_portid(nl), NULL, NULL);
    LOG_SYSERR_AND_CLEANUP(ret, -1);

    mnl_socket_close(nl);
    return 0;

cleanup:
    if (nl != NULL) mnl_socket_close(nl);
    return -1;
}

int netlink_link_set_up(const char* ifname) {
    char buf[MNL_SOCKET_BUFFER_SIZE];
    struct nlmsghdr *nlh = mnl_nlmsg_put_header(buf);
    nlh->nlmsg_type = RTM_NEWLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    nlh->nlmsg_seq = time(NULL);

    struct ifinfomsg *ifm = mnl_nlmsg_put_extra_header(nlh, sizeof(struct ifinfomsg));
    ifm->ifi_family = AF_UNSPEC;
    
    unsigned int ifindex = if_nametoindex(ifname);
    if (ifindex == 0) return -1;
    ifm->ifi_index = ifindex;
    ifm->ifi_change = IFF_UP;
    ifm->ifi_flags = IFF_UP;

    return netlink_talk(nlh);
}

int netlink_link_set_master(const char* ifname, const char* master_name) {
    char buf[MNL_SOCKET_BUFFER_SIZE];
    struct nlmsghdr *nlh = mnl_nlmsg_put_header(buf);
    nlh->nlmsg_type = RTM_NEWLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    nlh->nlmsg_seq = time(NULL);

    struct ifinfomsg *ifm = mnl_nlmsg_put_extra_header(nlh, sizeof(struct ifinfomsg));
    ifm->ifi_family = AF_UNSPEC;
    
    unsigned int ifindex = if_nametoindex(ifname);
    unsigned int master_idx = if_nametoindex(master_name);
    if (ifindex == 0 || master_idx == 0) return -1;
    
    ifm->ifi_index = ifindex;
    mnl_attr_put_u32(nlh, IFLA_MASTER, master_idx);

    return netlink_talk(nlh);
}

int netlink_add_ip(const char* ifname, const char* ip_str, unsigned char prefix) {
    char buf[MNL_SOCKET_BUFFER_SIZE];
    struct nlmsghdr *nlh = mnl_nlmsg_put_header(buf);
    nlh->nlmsg_type = RTM_NEWADDR;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
    nlh->nlmsg_seq = time(NULL);

    struct ifaddrmsg *ifa = mnl_nlmsg_put_extra_header(nlh, sizeof(struct ifaddrmsg));
    ifa->ifa_family = AF_INET;
    ifa->ifa_prefixlen = prefix;
    ifa->ifa_flags = IFA_F_PERMANENT;
    ifa->ifa_scope = RT_SCOPE_UNIVERSE;
    
    unsigned int ifindex = if_nametoindex(ifname);
    if (ifindex == 0) return -1;
    ifa->ifa_index = ifindex;

    uint32_t ip;
    if (inet_pton(AF_INET, ip_str, &ip) != 1) return -1;
    
    mnl_attr_put_u32(nlh, IFA_LOCAL, ip);
    mnl_attr_put_u32(nlh, IFA_ADDRESS, ip);

    return netlink_talk(nlh);
}

int netlink_add_default_route(const char* ifname, const char* gw_str) {
    char buf[MNL_SOCKET_BUFFER_SIZE];
    struct nlmsghdr *nlh = mnl_nlmsg_put_header(buf);
    nlh->nlmsg_type = RTM_NEWROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
    nlh->nlmsg_seq = time(NULL);

    struct rtmsg *rtm = mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtmsg));
    rtm->rtm_family = AF_INET;
    rtm->rtm_dst_len = 0;
    rtm->rtm_src_len = 0;
    rtm->rtm_tos = 0;
    rtm->rtm_table = RT_TABLE_MAIN;
    rtm->rtm_protocol = RTPROT_BOOT;
    rtm->rtm_scope = RT_SCOPE_UNIVERSE;
    rtm->rtm_type = RTN_UNICAST;

    uint32_t gw;
    if (inet_pton(AF_INET, gw_str, &gw) != 1) return -1;
    
    unsigned int ifindex = if_nametoindex(ifname);
    if (ifindex == 0) return -1;

    mnl_attr_put_u32(nlh, RTA_GATEWAY, gw);
    mnl_attr_put_u32(nlh, RTA_OIF, ifindex);

    return netlink_talk(nlh);
}
