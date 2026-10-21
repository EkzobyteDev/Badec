cc = g++
v  = 2

badec: badec_v$(v).cpp header_v$(v).h path_extractor.*
	rm -f badec && $(cc) badec_v$(v).cpp header_v$(v).h path_extractor.* -o badec

undec: undec_v$(v).cpp header_v$(v).h path_extractor.*
	rm -f undec && $(cc) undec_v$(v).cpp header_v$(v).h path_extractor.* -o undec

badec3:
	gcc path_extractor.* huffman/badec.c huffman/header.h huffman/huffman_codes.c huffman/huffman_codes.h  -o badec

undec3:
	gcc path_extractor.* huffman/undec.c huffman/header.h huffman/huffman_codes.c huffman/huffman_codes.h  -o undec