###############################################################################
#  The BYTE UNIX Benchmarks - Release 1
#          Module: loopm.awk   SID: 1.4 5/15/91 19:30:25
#      LOOPS per minute
#          
###############################################################################
# Bug reports, patches, comments, suggestions should be sent to:
#
#	Ben Smith or Tom Yager at BYTE Magazine
#	ben@bytepb.byte.com   tyager@bytepb.byte.com
#
###############################################################################
#  Modification Log:
#        created 2/12/91 -ben
#
###############################################################################
BEGIN { rsum = 0.000; r2sum = 0.000; r_product = 0.0000;
	  iter = 0; Test=""; SubTest=""; secs = 0.00; secs_sum = 0.00;
	  mins = 0.000; time_str=""; flavor="SysV";
	}
/TEST\|/ { split($0, junk,"|");
	Test=junk[2]; 
	}
/FLAVOR\|/ { split($0, junk,"|");
	flavor=junk[2] ; 
	}
/loops/ { loops=$1; 
	iter ++;
        }
/real/	{ if (flavor == "SysV") {time_str = $2; }
  	else { time_str = $1; }
	# determine seconds from time_str
	if ( time_str ~/\:/)
	   {
	   split(time_str,junk,":");
	   secs = 60 * junk[1] + junk[2];
	   }
	else { secs = time_str; }
	mins=secs/60.000; 
	print mins;
	if(loops) { rsum += loops/mins;
		r2sum += (loops*loops)/(mins*mins);
		r_product += (log(loops)-log(mins));
		secs_sum += secs;
		}
	}  
/user/	{ if (flavor == "SysV") { next;} } # don't use these times
/sys/	{ if (flavor == "SysV") { next;} } # don't use these times
#/^$/	{ next } 
END {
	if (iter > 0) {  
# TestName|Sample(seconds)|units|ArithMean|GeoMean|DataPoints
	    printf("%s|%.1f|lpm|%.1f|%.1f|%d\n",Test,secs_sum/iter,rsum/iter,exp(r_product/iter),iter);
	    }
	else { 
	    printf("%s|  no measured results|\n",Test); 
	    }
    }
