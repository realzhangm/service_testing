#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <linux/if_tun.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>

#include "icmp_reply.h"

#define TUN_DEV "/dev/net/tun"

void print_err(int ret, const char *description) {
  fprintf(stderr, "%s: %d, %s \n", description, ret, strerror(errno));
}

int make_taptun(int flags, char **dev_name) {
  struct ifreq ifr;

  int fd = open(TUN_DEV, O_RDWR);
  if (fd < 0) {
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = flags;

  int ret = ioctl(fd, TUNSETIFF, &ifr);
  if (ret < 0) {
    close(fd);
    return ret;
  }
  *dev_name = strdup(ifr.ifr_name);
  return fd;
}

int make_tap(char **dev_name) {
  return make_taptun(IFF_TAP | IFF_NO_PI, dev_name);
}

int make_tun(char **dev_name) {
  return make_taptun(IFF_TUN | IFF_NO_PI, dev_name);
}

void print_hex(uint8_t *data, size_t data_len) {
  for (size_t i = 0; i < data_len; i++) {
    printf("%02hhx ", data[i]);
  }
  printf("\n");
}

void handle_tun(int fd) {
  unsigned char buffer[4096];
  unsigned char buffer2[4096];

  ssize_t nr = read(fd, buffer, sizeof(buffer));
  if (nr < 0) {
    print_err((int)nr, "read error");
    return;
  }

  int protocl_type = -1;
  unsigned char *ip_payload = NULL;
  struct iphdr *ip_pack = (struct iphdr *)buffer;
  ip_payload = buffer + ((int)(ip_pack->ihl) << 2);
  if (ip_pack->protocol == IPPROTO_ICMP) {
    struct icmphdr *icmp_pack = (struct icmphdr *)ip_payload;
    if (icmp_pack->type == ICMP_ECHO) {
      protocl_type = ICMP_ECHO;
    }
  } else if (ip_pack->protocol == IPPROTO_TCP) {
    struct tcphdr *tcp_pack = (struct tcphdr *)ip_payload;
    short target_port = ntohs(tcp_pack->dest);
    printf("TCP, port=%d\n", target_port);
  } else if (ip_pack->protocol == IPPROTO_UDP) {
    struct udphdr *udp_pack = (struct udphdr *)ip_payload;
    short target_port = ntohs(udp_pack->dest);
    printf("UDP, port=%d\n", target_port);
  } else {
    protocl_type = -1;
    printf("unknown! [%d] \n", ip_pack->protocol);
  }

  print_hex(buffer, nr);

  // just icmp response
  if (protocl_type == ICMP_ECHO) {
    int rspLen = pack_respond_icmp(buffer, buffer2);
    ssize_t nw = write(fd, buffer2, rspLen);
    if (nw < 0) {
      print_err((int)nw, "write error");
      return;
    }
  }
}

int main() {
  char *dev_name;
  int tun_fd = make_tun(&dev_name);
  // int tun_fd = make_tap(&dev_name);
  if (tun_fd < 0) {
    print_err(tun_fd, "fail to make tun");
    exit(-1);
  }

  printf("open %s \n", dev_name);

  int max_fd = tun_fd;

  for (;;) {
    fd_set rd_set;
    FD_ZERO(&rd_set);
    FD_SET(tun_fd, &rd_set);
    FD_SET(STDIN_FILENO, &rd_set);
    int ret = select(max_fd + 1, &rd_set, NULL, NULL, NULL);
    if (ret < 0) {
      print_err(ret, "select error");
      goto exit_;
    }

    if (FD_ISSET(STDIN_FILENO, &rd_set)) {
      char buffer[1024];
      int ret = read(STDIN_FILENO, buffer, sizeof(buffer));
      if (ret < 0 || memcmp(buffer, "quit", 4) == 0) {
        goto exit_;
      }
      write(STDOUT_FILENO, buffer, ret);
    } else if (FD_ISSET(tun_fd, &rd_set)) {
      handle_tun(tun_fd);
    }
  }

exit_:
  free(dev_name);
  close(tun_fd);
  return 0;
}
