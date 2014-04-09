	echo "ProYAM/ZCOMM script to upload rz/sz files to VMS"
	if !fvmodem.h echo "Can't find vmodem.h !!";  abort
	pat
	pat 1 "Server"
	pat 2 "unrecognized command verb"
	put "kermit server\r"
	wait -f15
	if 1 goto frog
	if !2 echo "Unexpected response from VMS!"; abort
	expand vuplfile.t vmodem.h rz.c sz.c vrzsz.c vvmodem.c 
	expand vuplfile.t zm.c zmr.c zmodem.h crctab.c init.com
	goto endmsg
frog:	send vmodem.h rz.c sz.c vrzsz.c vvmodem.c zm.c
	send zmodem.h crctab.c init.com
	finish
endmsg:	while "L<11" echo "Please read the VMS C Compile and Link instructions in sz.c and rz.c"
	echo "vupl.t finished."
	return
