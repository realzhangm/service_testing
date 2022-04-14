
## what is veth-pair?

--------------------------
|  [pair] <-----> [pair] |      
|                        |
|  just like a cable     |
--------------------------

## 实验
### veth-pair 的连通性
```bash
# 创建 veth pair
ip l a veth0 type veth peer name veth1

# set ip address
ip a a 10.1.1.11/24 dev veth1
ip a a 10.1.1.12/24 dev veth2

# link up
ip l set veth0 up
ip l set veth1 up

# ping and tcpdump 
# to see there is no response of ping, and packets are captured by tcpdump via veth0
# why?
# 1)
ping -I veth1 10.1.1.10

# 2)
tcpdump -i veth0

#  but there is packets captured by tcpdump via lo
tcpdump -i lo
# to see route table : `ip route show table 0`

```
if can't get ping response, try this:
```bash
echo 1 > /proc/sys/net/ipv4/conf/veth1/accept_local
echo 1 > /proc/sys/net/ipv4/conf/veth0/accept_local
echo 0 > /proc/sys/net/ipv4/conf/all/rp_filter
echo 0 > /proc/sys/net/ipv4/conf/veth0/rp_filter
echo 0 > /proc/sys/net/ipv4/conf/veth1/rp_filter
```

### 两个 namespace 之间的连通性

```bash
# set namespace
ip netns a ns1
ip netns a ns2

# 将 veth0 veth1 分别加入两个 ns
ip l s veth0 netns ns1
ip l s veth1 netns ns2
# 给两个 veth0 veth1 配上 IP 并启用
ip netns exec ns1 ip a a 10.1.1.2/24 dev veth0
ip netns exec ns2 ip a a 10.1.1.3/24 dev veth1
ip netns exec ns1 ip l s veth0 up
ip netns exec ns2 ip l s veth1 up


# 从 veth0 ping veth1
root@vm1:~# ip netns exec ns1 ping 10.1.1.3
PING 10.1.1.3 (10.1.1.3) 56(84) bytes of data.
64 bytes from 10.1.1.3: icmp_seq=1 ttl=64 time=0.130 ms
64 bytes from 10.1.1.3: icmp_seq=2 ttl=64 time=0.076 ms

```

### 通过 Bridge 相连
Linux Bridge 相当于一台交换机，可以中转两个 namespace 的流量

```bash
# 首先创建 bridge br0
ip l a br0 type bridge
ip l s br0 up 

# 然后创建两对 veth-pair
ip l a veth0 type veth peer name br-veth0
ip l a veth1 type veth peer name br-veth1

# 分别将两对 veth-pair 加入两个 ns 和 br0
ip l s veth0 netns ns1
ip l s br-veth0 master br0
ip l s br-veth0 up

ip l s veth1 netns ns2
ip l s br-veth1 master br0
ip l s br-veth1 up

# 给两个 ns 中的 veth 配置 IP 并启用
ip netns exec ns1 ip a a 10.1.1.2/24 dev veth0
ip netns exec ns1 ip l s veth0 up

ip netns exec ns2 ip a a 10.1.1.3/24 dev veth1
ip netns exec ns2 ip l s veth1 up

# 从 ns1 的 veth0 ping ns2 的 veth1
root@vm1:~# ip netns exec ns1 ping 10.1.1.3
PING 10.1.1.3 (10.1.1.3) 56(84) bytes of data.
64 bytes from 10.1.1.3: icmp_seq=1 ttl=64 time=0.047 ms
64 bytes from 10.1.1.3: icmp_seq=2 ttl=64 time=0.094 ms

```

### 通过 OVS 相连
安装 open vSwith
```bash
apt install openvswitch-switch
```
步骤
```bash
# 用 ovs 提供的命令创建一个 ovs bridge
ovs-vsctl add-br ovs-br

# 创建两对 veth-pair
ip l a veth0 type veth peer name ovs-veth0
ip l a veth1 type veth peer name ovs-veth1

# 将 veth-pair 两端分别加入到 ns 和 ovs bridge 中
ip l s veth0 netns ns1
ovs-vsctl add-port ovs-br ovs-veth0
ip l s ovs-veth0 up

ip l s veth1 netns ns2
ovs-vsctl add-port ovs-br ovs-veth1
ip l s ovs-veth1 up

# 给 ns 中的 veth 配置 IP 并启用
ip netns exec ns1 ip a a 10.1.1.2/24 dev veth0
ip netns exec ns1 ip l s veth0 up

ip netns exec ns2 ip a a 10.1.1.3/24 dev veth1
ip netns exec ns2 ip l s veth1 up

# veth0 ping veth1
[root@localhost ~]# ip netns exec ns1 ping 10.1.1.3
PING 10.1.1.3 (10.1.1.3) 56(84) bytes of data.
64 bytes from 10.1.1.3: icmp_seq=1 ttl=64 time=0.311 ms
64 bytes from 10.1.1.3: icmp_seq=2 ttl=64 time=0.087 ms
^C
--- 10.1.1.3 ping statistics ---
2 packets transmitted, 2 received, 0% packet loss, time 999ms
rtt min/avg/max/mdev = 0.087/0.199/0.311/0.112 ms
```

## 总结
veth-pair 在虚拟网络中充当着桥梁的角色，连接多种网络设备构成复杂的网络。

veth-pair 的三个经典实验，直接相连、通过 Bridge 相连和通过 OVS 相连。

## Reference
https://www.cnblogs.com/bakari/p/10613710.html

https://man7.org/linux/man-pages/man4/veth.4.html


