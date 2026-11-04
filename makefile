cc = g++
v  = 2
config = shannon

badec: v$(v)/badec_v$(v).cpp v$(v)/header_v$(v).h v$(v)/path_extractor.c v$(v)/path_extractor.h
	rm -f legacy/badec_v$(v) && $(cc) v$(v)/badec_v$(v).cpp v$(v)/header_v$(v).h v$(v)/path_extractor.* -o legacy/badec_v$(v)

undec: v$(v)/undec_v$(v).cpp v$(v)/header_v$(v).h v$(v)/path_extractor.c v$(v)/path_extractor.h
	rm -f legacy/undec_v$(v) && $(cc) v$(v)/undec_v$(v).cpp v$(v)/header_v$(v).h v$(v)/path_extractor.* -o legacy/undec_v$(v)

badec3:
	gcc path_extractor.* $(config)/badec.c $(config)/header.h $(config)/*_codes.c $(config)/*_codes.h  -o badec

undec3:
	gcc path_extractor.* $(config)/undec.c $(config)/header.h $(config)/*_codes.c $(config)/*_codes.h  -o undec

rlebadec:
	gcc path_extractor.* rle/badec.c rle/header.h -o badec

rleundec:
	gcc path_extractor.* rle/undec.c rle/header.h -o undec