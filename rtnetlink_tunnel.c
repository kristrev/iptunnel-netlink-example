#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libmnl/libmnl.h>
#include <linux/rtnetlink.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/ip.h>
#include <linux/if_link.h>
#include <linux/if_tunnel.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]){
    struct mnl_socket *nl_sock = NULL;
    struct nlmsghdr *nlh = NULL;
    struct nlmsgerr *nlerr = NULL;
    struct nlattr *linkinfo = NULL, *tunnelinfo = NULL;
    struct ifinfomsg *ifinfo = NULL;
    uint8_t buf[MNL_SOCKET_BUFFER_SIZE];
    size_t numbytes = 0;
    struct in_addr addr;

    memset(buf, 0, sizeof(buf));

    if((nl_sock = mnl_socket_open(NETLINK_ROUTE)) == NULL){
        perror("mnl_socket_open: ");
        exit(EXIT_FAILURE);
    }

    //Add and configure header
    nlh = mnl_nlmsg_put_header(buf);
    //Create only if interface does not exists from before (EXCL)
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
    nlh->nlmsg_type = RTM_NEWLINK; 

    //see rtnl_newlink in net/core/rtnetlink.c for observing how this function
    //works
    ifinfo = mnl_nlmsg_put_extra_header(nlh, sizeof(struct ifinfomsg));
    ifinfo->ifi_family = AF_UNSPEC;

    //Add a name and mtu, for testing
    mnl_attr_put_str(nlh, IFLA_IFNAME, "kre2");
    mnl_attr_put_u32(nlh, IFLA_MTU, 1234);
  
    //Type is required, set to IPIP
    linkinfo = mnl_attr_nest_start(nlh, IFLA_LINKINFO);
    mnl_attr_put_str(nlh, IFLA_INFO_KIND, "ipip");

    //Add information about the tunnel
    tunnelinfo = mnl_attr_nest_start(nlh, IFLA_INFO_DATA);
    inet_pton(AF_INET, "192.168.203.19", &addr);
    mnl_attr_put_u32(nlh, IFLA_IPTUN_LOCAL, addr.s_addr);
    inet_pton(AF_INET, "10.0.0.2", &addr);
    mnl_attr_put_u32(nlh, IFLA_IPTUN_REMOTE, addr.s_addr);

    mnl_attr_nest_end(nlh, tunnelinfo);
    mnl_attr_nest_end(nlh, linkinfo);

    numbytes = mnl_socket_sendto(nl_sock, nlh, nlh->nlmsg_len);
    numbytes = mnl_socket_recvfrom(nl_sock, buf, sizeof(buf));

    nlh = (struct nlmsghdr*) buf;

    if(nlh->nlmsg_type == NLMSG_ERROR){
        nlerr = mnl_nlmsg_get_payload(nlh);
        //error==0 for ack
        if(nlerr->error)
            printf("Error: %s\n", strerror(-nlerr->error));
    } else {
        printf("Link added\n");
    }

    return EXIT_FAILURE;
}
