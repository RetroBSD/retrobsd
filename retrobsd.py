#!/usr/bin/env python3
import os, sys, subprocess

RISCV_MACHINE_MIN = '''
#include "sys/param.h"
#include "sys/user.h"
#include "sys/proc.h"
int	cmask = CMASK;

int securelevel = -1;

struct user u = {};
void _start(){
	struct proc *p;
	p = &proc[0];
	//p->p_addr = *ka6;
	p->p_stat = SRUN;
	p->p_flag |= SLOAD|SSYS;
	p->p_nice = NZERO;

	u.u_procp = p;			/* init user structure */
	//u.u_ap = u.u_arg;
	u.u_cmask = cmask;
	u.u_lastfile = -1;

#ifdef INET
	if (netoff = netinit())
		printf("netinit failed\n");
	else
		{
		NETSETHZ();
		NETSTART();
		}
#endif

}
'''


def c2o(file, out='/tmp/c2o.o', includes=None, defines=None, opt='-O0', bits=64 ):
	if 'fedora' in os.uname().nodename:
		cmd = ['riscv64-linux-gnu-gcc']
	else:
		cmd = ['riscv64-unknown-elf-gcc']

	if bits==32:
		cmd.append('-march=rv32imac')
		cmd.append('-mabi=ilp32')

	cmd += [
		'-c', '-mcmodel=medany', '-fomit-frame-pointer', '-ffunction-sections',
		'-ffreestanding', '-nostdlib', '-nostartfiles', '-nodefaultlibs', 
		opt, '-g', '-o', out, file
	]
	if includes:
		for inc in includes:
			if not inc.startswith('-I'): inc = '-I'+inc
			cmd.append(inc)
	if defines:
		for d in defines:
			if not d.startswith('-D'): d = '-D'+d
			cmd.append(d)
	
	print(cmd)
	subprocess.check_call(cmd)
	return out


def mkkernel():
	defines = [
		'KERNEL', 
		'CONS_MAJOR=UART_MAJOR',
	]
	includes = [
		'./include'
	]
	obs = []

	rtmp = '/tmp/_riscv_.c'
	open(rtmp,'w').write(RISCV_MACHINE_MIN)
	o = c2o(
		rtmp, 
		out = '/tmp/_riscv_.o',
		includes=includes, defines=defines, bits=32)
	obs.append(o)


	for name in os.listdir('./sys/kernel/'):
		assert name.endswith('.c')
		print(name)
		o = c2o(
			os.path.join('./sys/kernel', name), 
			out = '/tmp/%s.o' % name,
			includes=includes, defines=defines, bits=32)
		obs.append(o)

	macho = []
	#for name in 'machdep'.split():
	for name in 'devsw cons swap'.split():
		o = c2o(
			'./sys/pic32/%s.c' % name, 
			out = '/tmp/_riscv_%s.o' % name,
			includes=includes, defines=defines, bits=32)
		macho.append(o)


	if 'fedora' in os.uname().nodename:
		cmd = ['riscv64-linux-gnu-ld']
	else:
		cmd = ['riscv64-unknown-elf-ld']

	cmd += ['-march=rv32', '-m', 'elf32lriscv', '-o', '/tmp/retrobsd.elf'] + obs + macho
	print(cmd)
	subprocess.check_call(cmd)

if __name__=='__main__':
	mkkernel()
