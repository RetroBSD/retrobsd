###########
#       pgms/mwips.awk
#
#	Measures MWIPS for Whetstone (self-timing, ignore "time" output)
#
#	Created 1997.08.25	DCN
#          
BEGIN { rsum = 0.000; r2sum = 0.000; r_product = 0.0000;
	  iter = 0; Test=""; SubTest=""; secs = 10.00; secs_sum = 0.00;
	}
/TEST\|/ { split($0, junk,"|");
	Test=junk[2]; 
	}
/FLAVOR\|/ { split($0, junk,"|");
	flavor=junk[2] ; 
	}
/^MWIPS/ {
	loopspersec=$2;
	secs=$3;
	secs_sum += secs;
	loops=secs*loopstmp;
	iter ++;
	rsum += loopspersec;
	r2sum += loopspersec*loopspersec;
	r_product += log(loopspersec);
        }
END {
	if (iter > 0) {  
# TestName|Sample(seconds)|units|ArithMean|GeoMean|DataPoints
	    printf("%s|%.1f|MWIPS|%.1f|%.1f|%d\n",Test,secs_sum/iter,rsum/iter,exp(r_product/iter),iter)
	    }
	else { 
	    printf("%s|  no measured results|\n",Test); 
	    }
    }
