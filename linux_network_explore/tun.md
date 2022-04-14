
##

内核版本 5.4.0-90-generic tun 驱动已经是内置的了。
```bash
root@vm1:~# modinfo tun
name:           tun
filename:       (builtin)
alias:          devname:net/tun
alias:          char-major-10-200
license:        GPL
author:         (C) 1999-2004 Max Krasnyansky <maxk@qualcomm.com>
description:    Universal TUN/TAP device driver
root@vm1:~# 
root@vm1:~# uname -a
Linux vm1 5.4.0-90-generic #101-Ubuntu SMP Fri Oct 15 20:00:55 UTC 2021 x86_64 x86_64 x86_64 GNU/Linux
```

##