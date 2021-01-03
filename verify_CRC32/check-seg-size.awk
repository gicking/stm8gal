#!/usr/bin/awk -f

BEGIN {
	start_code = 0x200;	# start of verify_CRC32() from "link/common.lk". Required by stm8gal!
	addr_start = 0x2F4;	# start of passed parameters from "link/common.lk". Required by stm8gal!
	flagError  = 0;			# global error flag
	total_size = 0;			# size of code + 12B parameters
}

END {
	if(flagError == 0) {
		printf "Address check passed, total size %dB\n", total_size;
	}
	else {
		printf "\nADDRESS CHECK FAILED!!!!!\n\n";
	}

}


# check routine "verify_CRC32" start address. Required by stm8gal!
$2 == "_verify_CRC32" {
	#print $0;

	func_addr = strtonum("0x" $1);

	# check if function is located at correct address
	if(func_addr != start_code) {
		flagError = 1;
		printf "*** ERROR: Function 'verify_CRC32()' address wrong! ***\n";
		printf "    'verify_CRC32()' starts @ %#x\n", func_addr;
		printf "    expected address @ %#x\n", start_code;
	}
}


# check segment start and end addresses
$1 == "VERIFY_SEG" {

	switch($1) {
		case "VERIFY_SEG":
			seg_addr = strtonum("0x" $2);
			seg_size = strtonum("0x" $3);
			total_size = seg_size + 12			# account for 12B parameters and return value

			# check if code segment starts at correct address
			if(seg_addr != start_code) {
				flagError = 1;
				printf "*** ERROR: Segment %s starting address wrong! ***\n", $1;
				printf "    %s starts @ %#x\n", $1, seg_addr;
				printf "    expected address @ %#x\n", start_code;
			}

			# check if code segment overlaps global parameters
			if(seg_addr + seg_size - 1 >= addr_start) {
				flagError = 1;
				printf "*** ERROR: Segment %s overlaps with parameters! ***\n", $1;
				printf "    %s ends @ %#x\n", $1, seg_addr + seg_size - 1;
				printf "    parameters begin @ %#x\n", addr_start;
			}
			break;
	}
}
