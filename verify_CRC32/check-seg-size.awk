#!/usr/bin/awk -f

BEGIN {
	addr_start = 0x2F4;		# start of passed parameters from "link/common.lk"
}

END {

}

$1 == "Area" && $2 == "Addr" && $3 == "Size" {
	if(getline && getline) {
		switch($1) {
			case "VERIFY_SEG":
				seg_addr = strtonum("0x" $2);
				seg_size = strtonum("0x" $3);

				if(seg_addr + seg_size - 1 >= addr_start) {
					printf "*** ERROR: Segment %s overlaps with parameters! ***\n", $1;
					printf "    %s ends @ %#x\n", $1, seg_addr + seg_size - 1;
					printf "    parameters begin @ %#x\n", addr_start;
				}
				break;
		}
	}

	next;
}
