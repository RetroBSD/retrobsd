##############################################################################
#  The BYTE UNIX Benchmarks - Release 1
#          Module: report.awk   SID: 1.4 5/15/91 19:30:25
#          
##############################################################################
# Bug reports, patches, comments, suggestions should be sent to:
#
#	Ben Smith or Tom Yager at BYTE Magazine
#	ben@bytepb.byte.com   tyager@byptepb.byte.com
#
##############################################################################
# generate an report from test log
# 
#############################################################################
#  Modification Log:
#        created 4/1/91 - Ben Smith
#
##############################################################################
BEGIN { FS="|" ; 
	printf("\n");
      }
NF==2		{ print $2; }
NF==3		{ printf("%-40.40s %s\n",$1,$2); }
NF==6		{ printf("%-40.40s %8.1f %-5s (%.1f secs, %d samples)\n",$1,$5,$3,$2,$6); }
END   { printf("\n"); }
