/*
 * Copyright (c) 1982, 1986, 1989, 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "param.h"
#include "user.h"
#include "proc.h"
#include "systm.h"

void
setuid()
{
	struct a {
		uid_t uid;
	} *uap = (struct a*) u.u_arg;
	register uid_t uid;

	uid = uap->uid;
	if (uid != u.u_ruid && ! suser())
		return;
	/*
	 * Everything's okay, do it.
	 */
	u.u_procp->p_uid = uid;
	u.u_uid = uid;
	u.u_ruid = uid;
	u.u_svuid = uid;
	u.u_error = 0;
}

void
seteuid()
{
	struct a {
		uid_t euid;
	} *uap = (struct a *)u.u_arg;
	register uid_t euid;

	euid = uap->euid;
	if (euid != u.u_ruid && euid != u.u_svuid && ! suser())
		return;
	/*
	 * Everything's okay, do it.
	 */
	u.u_uid = euid;
	u.u_error = 0;
}

void
setgid()
{
	struct a {
		gid_t gid;
	} *uap = (struct a *)u.u_arg;
	register gid_t gid;

	gid = uap->gid;
	if (gid != u.u_rgid && ! suser())
		return;

	u.u_groups[0] = gid;		/* effective gid is u_groups[0] */
	u.u_rgid = gid;
	u.u_svgid = gid;
	u.u_error = 0;
}

void
setegid()
{
	struct a {
		gid_t egid;
	} *uap = (struct a *)u.u_arg;
	register gid_t egid;

	egid = uap->egid;
	if (egid != u.u_rgid && egid != u.u_svgid && ! suser())
		return;

	u.u_groups[0] = egid;
	u.u_error = 0;
}
