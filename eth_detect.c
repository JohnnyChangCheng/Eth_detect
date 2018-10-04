#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <asm/types.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/if.h>
#include <linux/rtnetlink.h>
#include <pthread.h>
#include <fcntl.h>
static int check_carrier(){
    uint8_t buff=0;
    int fd = open("/sys/class/net/enp2s0/carrier",O_RDONLY);
    printf("read return %d\n",read(fd, &buff, 1));
    return buff;
}
static int  read_msg(int fd)
{
    int len;
    char buf[4096];
    struct iovec iov = { buf, sizeof(buf) };
    struct sockaddr_nl sa;
    struct msghdr msg = { (void *)&sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
    struct nlmsghdr *nh;

    len = recvmsg(fd, &msg, 0);
    if(len == -1) {
        perror("recvmsg");
        return -1;
    }

    for (nh = (struct nlmsghdr *) buf; NLMSG_OK (nh, len);
         nh = NLMSG_NEXT (nh, len)) {
         struct ifinfomsg *ifimsg;
        /* The end of multipart message. */
         printf("netlink message: len = %u, type = %u, flags = 0x%X, seq = %u, pid = %u\n",
            nh->nlmsg_len,
            nh->nlmsg_type,
            nh->nlmsg_flags,
            nh->nlmsg_seq,
            nh->nlmsg_pid);
        ifimsg = NLMSG_DATA(nh);
        if (nh->nlmsg_type == NLMSG_DONE)
            return -1;

        if (nh->nlmsg_type == NLMSG_ERROR) {
            continue;
        }
        if (nh -> nlmsg_type == RTM_NEWLINK){
            if(system("ifup eth0") > 0)
                return 1;    
        }
    }
}
int main(int argc, char *argv[])
{   
    struct sockaddr_nl sa;
    int fd;
    memset(&sa, 0, sizeof(sa));
    uint8_t buff[100];
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR |RTMGRP_IPV6_IFADDR;

    int value = system("ping -w5 -c 5 retroadm-dev.atgames.net");
    if(value == 0){
        value = system("/data/linkup.sh start");
    }else{
        fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
        if(fd == -1) {
            perror("socket");
            return 1;
        }

        if(bind(fd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
            perror("bind");
            return 1;
        }
        for(;;) {
            if(read_msg(fd) > 0)
                break;
        }
        value = system("/data/linkup.sh start");
    }
    return 0;
}
