#!/usr/bin/awk -f

BEGIN {
	start_code = 0x200;	# start of verify_CRC32() from "link/common.lk"
	addr_start = 0x2F4;	# start of passed parameters from "link/common.lk"
	flagError  = 0;		# global error flag
}

END {
	if(flagError == 0) {
		printf "Address check passed\n";
	}
	else {
		printf "ADDRESS CHECK FAILED!!!!!\n";
	}

}

$1 == "Area" && $2 == "Addr" && $3 == "Size" {
	if(getline && getline) {
		switch($1) {
			case "VERIFY_SEG":
				seg_addr = strtonum("0x" $2);
				seg_size = strtonum("0x" $3);

				# check if code starts at correct address
				if(seg_addr != start_code) {
					flagError = 1;
					printf "*** ERROR: Segment %s starting address wrong! ***\n", $1;
					printf "    %s starts @ %#x\n", $1, seg_addr;
					printf "    expected address @ %#x\n", start_code;
				}

				# check if code end overlaps global parameters
				if(seg_addr + seg_size - 1 >= addr_start) {
					flagError = 1;
					printf "*** ERROR: Segment %s overlaps with parameters! ***\n", $1;
					printf "    %s ends @ %#x\n", $1, seg_addr + seg_size - 1;
					printf "    parameters begin @ %#x\n", addr_start;
				}
				break;
		}
	}

	next;
}
