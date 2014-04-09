set history=30
set path=(/bin /sbin /etc /usr/sbin /usr/ucb /usr/bin /usr/new /usr/local)
if ($?prompt) then
	set prompt="\!% "
endif

set filec
set mail=( 0 /usr/spool/mail/$USER )
set cdpath=( ~ /usr/spool/mqueue)
set prompt="\!% root--> "
alias	mail /usr/ucb/Mail
alias	pwd	'echo $cwd'
alias	h	history
alias	q5	'tail -5 /usr/spool/mqueue/syslog'
