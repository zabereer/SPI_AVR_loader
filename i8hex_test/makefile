CXXFLAGS=-std=c++11 -g -O0 -I../ -MMD -MP

test: build
	./decoder

build: decoder

I8HEX_decoder.o : ../I8HEX_decoder.cpp
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

decoder: decoder.o I8HEX_decoder.o
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	$(RM) -vf decoder
	$(RM) -vf decoder.o I8HEX_decoder.o
	$(RM) -vf decoder.d I8HEX_decoder.d

.PHONY: all test build clean

-include decoder.d I8HEX_decoder.d
