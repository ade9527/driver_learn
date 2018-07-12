
# 内核应用错误 引用前没有检查 netdev_ops是否为空
错误码
```
/root/modules # insmod vnet.ko
Unable to handle kernel NULL pointer dereference at virtual address 00000000
pgd = c3068000
[00000000] *pgd=33bb9831, *pte=00000000, *ppte=00000000
Internal error: Oops: 17 [#1] ARM
Modules linked in: vnet(O+) key_input_drv(O) lcd_2440fb(O)
CPU: 0 PID: 494 Comm: insmod Tainted: G           O    4.6.0 #71
Hardware name: SMDK2440
task: c3998100 ti: c392e000 task.ti: c392e000
PC is at register_netdevice+0x5c/0x4dc
LR is at 0x74656e76
pc : [<c0476ff4>]    lr : [<74656e76>]    psr: 60000013
sp : c392fd80  ip : 0000001c  fp : 00000018
r10: bf00806c  r9 : c392fefc  r8 : c3070600
r7 : c06fed78  r6 : c072d6b0  r5 : 00000000  r4 : c385a000
r3 : 00000000  r2 : 80000000  r1 : c392fd5d  r0 : 00000000
Flags: nZCv  IRQs on  FIQs on  Mode SVC_32  ISA ARM  Segment none
Control: c000717f  Table: 33068000  DAC: 00000051
Process insmod (pid: 494, stack limit = 0xc392e190)
Stack: (0xc392fd80 to 0xc3930000)
fd80: c385a260 c072e614 00000000 c385a000 00000000 c06fed78 c06fed78 c3070600
fda0: c392fefc bf00806c 00000018 c0477488 bf00a000 bf00a034 00000001 00000001
fdc0: bf00806c c0109618 c07195e8 c3fd4ed4 ffffffff c02e5a5c 00000001 c07339c8
fde0: ffffffff ffffff80 c3fe6954 c3f6b00c c3fd4ed4 00000000 00000003 c3f6b000
fe00: 00000000 20000013 c3f74000 c392fefc bf00806c 00000018 00000000 bf008060
fe20: 00000001 c384f980 bf008060 00000000 c392fefc c0163134 bf00806c c392ff44
fe40: 00000001 bf00806c c392ff44 00000001 bf00806c c0164ae4 ffff8000 00007fff
fe60: bf008060 c0162da4 00000001 c49aa100 bf008060 00000000 000eabb5 c49c7b58
fe80: c392fedc c05697e4 bf00806c 00000000 c3805000 6e72656b 00006c65 00000000
fea0: 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
fec0: 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
fee0: 00000000 00000000 74230200 78040900 00000051 c49aa000 00000000 2a0000ca
ff00: c392ff1c bf008064 c02dfab0 00000001 bf0081a8 bf0081b0 00000000 c392ff44
ff20: 000eabb5 00005e19 00000000 b6d7a008 00000051 c49ca000 00000000 c0164e9c
ff40: c3406c20 c49aa000 00025e19 c49c7540 c49c739b c49cfd28 000001c4 00000214
ff60: 00000000 00000000 00000000 00000490 00000027 00000028 0000000f 00000000
ff80: 0000000c 00000000 b6d84a54 00000069 bed42e94 00000080 c010a5a4 c392e000
ffa0: 00000000 c010a400 b6d84a54 00000069 b6d5a008 00025e19 000eabb5 00000001
ffc0: b6d84a54 00000069 bed42e94 00000080 bed42e98 000eabb5 bed42e98 00000000
ffe0: 00000001 bed42b34 00023820 b6e45984 60000010 b6d5a008 00000000 00000000
[<c0476ff4>] (register_netdevice) from [<c0477488>] (register_netdev+0x14/0x24)
[<c0477488>] (register_netdev) from [<bf00a034>] (vnet_init+0x34/0x4c [vnet])
[<bf00a034>] (vnet_init [vnet]) from [<c0109618>] (do_one_initcall+0x84/0x204)
[<c0109618>] (do_one_initcall) from [<c0163134>] (do_init_module+0x5c/0x1b0)
[<c0163134>] (do_init_module) from [<c0164ae4>] (load_module+0x1594/0x175c)
[<c0164ae4>] (load_module) from [<c0164e9c>] (SyS_init_module+0x13c/0x14c)
[<c0164e9c>] (SyS_init_module) from [<c010a400>] (ret_fast_syscall+0x0/0x38)
Code: ebfffea0 e2505000 ba000009 e5943128 (e5933000)
---[ end trace 77c81bafce9aca09 ]---
Segmentation fault<Paste>
```
```
	if (dev->netdev_ops->ndo_init) {
		ret = dev->netdev_ops->ndo_init(dev);
		if (ret) {
			if (ret > 0)
				ret = -EIO;
			goto out;
		}
	}
```


