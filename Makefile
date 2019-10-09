.PHONY: clean all realclean examples benches
.SECONDARY:

top:=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))

examples:=
benches:=bench_sma

all:: unit $(examples) $(benches)

test-src:=unit.cc test_stub.cc

all-src:=$(test-src) $(patsubst %, %.cc, $(examples)) $(patsubst %, %.cc, $(benches))
all-obj:=$(patsubst %.cc, %.o, $(all-src))

OPTFLAGS?=-O3 -march=native
#OPTFLAGS?=-O0 -march=native
CXXFLAGS+=$(OPTFLAGS) -MMD -MP -std=c++14 -g -pthread
CPPFLAGS+=-I $(top)include
RMDIR:=rmdir

# $(call build-cc, src-prefix, obj-prefix, source)
define build-cc
$(2)$(patsubst %.cc,%.o,$(3)): $(1)$(3) | $(dir $(2))
	$$(COMPILE.cc) $$< $$(OUTPUT_OPTION)

-include $(2)$(patsubst %.cc,%.d,$(3))
clean:: ; $$(RM) $(2)$(patsubst %.cc,%.o,$(3))
realclean:: ; $$(RM) $(2)$(patsubst %.cc,%.d,$(3))
endef

# $(call dump-cc-asm, src-prefix, obj-prefix, source)
define dump-cc-asm
$(2)$(patsubst %.cc,%.s,$(3)): $(1)$(3) | $(dir $(2))
	$$(filter-out -c -g,$$(COMPILE.cc)) -S $$< $$(OUTPUT_OPTION)

-include $(2)$(patsubst %.cc,%.d,$(3))
clean:: ; $$(RM) $(2)$(patsubst %.cc,%.s,$(3))
endef

# $(call build-cclib, libname, src-prefix, obj-prefix, sources)
define build-cclib
$(foreach src,$(4),$(eval $(call build-cc,$(2),$(3),$(src))))
$(1): $(foreach src,$(4),$(3)$(patsubst %.cc,%.o,$(src)))
	$$(AR) $$(ARFLAGS) $$@ $$^

realclean:: ; $(RM) $(1)
endef

# $(call build-ccexe, libname, src-prefix, obj-prefix, sources)
define build-ccexe
$(foreach src,$(4),$(eval $(call build-cc,$(2),$(3),$(src))))
$(1): $(foreach src,$(4),$(3)$(patsubst %.cc,%.o,$(src)))
	$$(LINK.cc) $$^ $$(OUTPUT_OPTION) $$(LOADLIBES) $$(LDLIBS)

realclean:: ; $(RM) $(1)
endef

### Google Test

gtest-top:=$(top)import/gtest/googletest

libgtest.a: CPPFLAGS+=-I $(gtest-top) -I $(gtest-top)/include
$(eval $(call build-cclib,libgtest.a,$(gtest-top)/src/,,gtest-all.cc))

### Google Benchmark

gbench-top:=$(top)import/gbench
gbench-srcdir:=$(gbench-top)/src
gbench-src:=$(notdir $(wildcard $(gbench-srcdir)/*.cc))

libbench.a: CPPFLAGS+=-I $(gbench-top)/include
gbench: ; mkdir gbench
$(eval $(call build-cclib,libbench.a,$(gbench-srcdir)/,gbench/,$(gbench-src)))

### Unit tests

test-srcdir:=$(top)test
test-src:=$(notdir $(wildcard $(test-srcdir)/*.cc))

unit: CPPFLAGS+=-I $(gtest-top)/include
unit: LDLIBS+=libgtest.a
unit: libgtest.a
$(eval $(call build-ccexe,unit,$(test-srcdir)/,,$(test-src)))

### Benchmarks

bench-srcdir:=$(top)bench
bench-src:=$(notdir $(wildcard $(bench-srcdir)/*.cc))

.PHONY: benchmarks
define build-bench
benchmarks:: $(1)
$(1): CPPFLAGS+=-I $(gbench-top)/include
$(1): LDLIBS+=libbench.a
$(1): libbench.a
$(eval $(call build-ccexe,$(1),$(bench-srcdir)/,,$(1).cc))
endef

$(foreach src,$(notdir $(wildcard $(bench-srcdir)/*.cc)),\
$(eval $(call build-bench,$(basename $(src)))))

### Asm examples

example-srcdir:=$(top)example
example-src:=$(notdir $(wildcard $(example-srcdir)/*.cc))

.PHONY: examples
define build-asm-example
$(call dump-cc-asm,$(example-srcdir)/,,$(1).cc)
examples:: $(1).asm
$(1).asm: $(1).s; c++filt < $$< | grep -v '\.[a-z]\+' > $$@
realclean:: ; $(RM) $(1).asm
endef

$(foreach src,$(notdir $(wildcard $(example-srcdir)/*.cc)),\
$(eval $(call build-asm-example,$(basename $(src)))))

