###############################################################################
#  The BYTE UNIX Benchmarks - Release 3
#          Module: fs.awk   SID: 3.4 5/15/91 19:30:24
#          
###############################################################################
# Bug reports, patches, comments, suggestions should be sent to:
#
#	Ben Smith or Tom Yager at BYTE Magazine
#	ben@bytepb.byte.com   tyager@bytepb.byte.com
#
###############################################################################
#  Modification Log:
#       added geometric mean 8/6/89 -ben
#       modified for new version of fstime 11/15/89 -ben
#       removed variance 10/23/90 -ben
#
###############################################################################
BEGIN   { w_product = 0.0000;
	r_product = 0.0000;
	c_product = 0.0000;
	iter=0;
	w_too_quick=0;
	r_too_quick=0;
	c_too_quick=0;
	}
/FLAVOR\|/ { split($0, junk,"|");
	flavor = junk[2];
		}
/TEST\|/ { split($0, junk," ");
	bufsize = junk[3];
	maxblocks = junk[5];
		}
/real/	{ iter++; ok++; next; }
/user/	{ if (flavor == "SysV") {next;} }
/sys/	{ if (flavor == "SysV") {next;} }
/^$/	{ next; } 
/^#/	{ next; } 
/sample/ { sample = $1; next; }
/fstime/ {
         print "** Iteration ",iter," Failed: ",$0;
	 ok--;
         fail=1;
         } 
/write/ { if (!fail) {
	       w+=$1;
	       w2+=$1*$1;
	       w_product += log($1); 
	       }
        }
/read/  { if (!fail) {
	       r+=$1;
	       r2+=$1*$1;
	       r_product += log($1); 
	       }
        }
/copy/  { if (!fail) {
	       c+=$1;
	       c2+=$1*$1;
	       c_product += log($1);
	       }
	}
END {
	if (ok > 0) {
# TestName|Sample(seconds)|Unit(KiloBytes/sec)|ArithMean|GeoMean|DataPoints
	printf "File Read %d bufsize %d maxblocks|%d|KBps|%.0f|%.0f|%d\n", bufsize, maxblocks, sample, r/ok, exp(r_product/ok), ok;
	printf "File Write %d bufsize %d maxblocks|%d|KBps|%.0f|%.0f|%d\n", bufsize, maxblocks, sample, w/ok, exp(w_product/ok), ok;
	printf "File Copy %d bufsize %d maxblocks|%d|KBps|%.0f|%.0f|%d\n", bufsize, maxblocks, sample, c/ok, exp(c_product/ok), ok;
	} else {
	    print "File I/O|  no measured results|"
	}
    }
