CC:=gcc
CXX:=g++
LIBS:=-lreadline -lhistory -lcgraph -lgvc
FLAGS_release:=-O2 -flto
FLAGS_debug:=-Og -ggdb3 -DTEST_BUILD
COMMONFLAGS:=-Werror -Wall
CFLAGS:=-std=gnu17 $(COMMONFLAGS) -Iinclude
CXXFLAGS:=-std=gnu++17 $(COMMONFLAGS) -Icxxinclude
OUTDIR:=build

# Recursive wildcard function, stackoverflow.com/questions/2483182
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Collect the source files
HEADERS:=$(call rwildcard,include,*.h)
CSRCS:=$(call rwildcard,src,*.c)
CXXSRCS:=$(call rwildcard,src,*.cc)
MAINCSRCS:=$(call rwildcard,mains,*.c)
MAINCXXSRCS:=$(call rwildcard,mains,*.cc)
OBJFILES_SRC:=$(patsubst %.c,%.o,$(CSRCS)) $(patsubst %.cc,cxx/%.o,$(CXXSRCS))
OBJFILES_SRC_debug:=$(foreach o,$(OBJFILES_SRC),$(OUTDIR)/debug/$(o))
OBJFILES_SRC_release:=$(foreach o,$(OBJFILES_SRC),$(OUTDIR)/release/$(o))
OBJFILES_MAINS:=$(patsubst %.c,%.o,$(MAINCSRCS)) $(patsubst %.cc,cxx/%.o,$(MAINCXXSRCS))
OBJFILES_MAINS_debug:=$(foreach o,$(OBJFILES_MAINS),$(OUTDIR)/debug/$(o))
OBJFILES_MAINS_release:=$(foreach o,$(OBJFILES_MAINS),$(OUTDIR)/release/$(o))
BINS:=$(patsubst mains/%.c,bin/%,$(MAINCSRCS)) \
	$(patsubst mains/%.cc,cxx/bin/%,$(MAINCXXSRCS))
BINS_debug:=$(foreach f,$(BINS),$(OUTDIR)/debug/$(f))
BINS_release:=$(foreach f,$(BINS),$(OUTDIR)/release/$(f))
CINCLUDES_debug:=$(patsubst include/%,$(OUTDIR)/debug/cxx/cincludes/%,$(HEADERS))
CINCLUDES_release:=$(patsubst include/%,$(OUTDIR)/release/cxx/cincludes/%,$(HEADERS))
OUTPUTS_debug:=$(OBJFILES_SRC_debug) $(OBJFILES_MAINS_debug) $(BINS_debug) \
	$(CINCLUDES_debug)
OUTPUTS_release:=$(OBJFILES_SRC_release) $(OBJFILES_MAINS_release) \
	$(BINS_release) $(CINCLUDES_release)
OUTPUTS:=$(OUTPUTS_debug) $(OUTPUTS_release)
DIRS:=$(sort $(dir $(OUTPUTS)))

_create_dirs := $(foreach d,$(DIRS),$(shell [[ -d $(d) ]] || mkdir -p $(d)))

binpath = $(filter %/$(1),$(BINS))

ifeq ($(strip $(CXXSRCS) $(MAINCXXSRCS)),)
# C only
LD:=$(CC)
LDFLAGS:=$(CFLAGS)
else
LD:=$(CXX)
LDFLAGS:=$(CXXFLAGS)
endif

ifeq ($(V),)
cmd = @printf '  %-8s %s\n' $(cmd_$(1)_name) $(if $(2),$(2),"$@") ; $(call cmd_$(1),$(2))
else
ifeq ($(V),1)
cmd = $(call cmd_$(1),$(2))
else
cmd = @$(call cmd_$(1),$(2))
endif
endif

get_target = $(word 2,$(subst /, ,$@))
target_flags = $(FLAGS_$(get_target))

DEPFLAGS = -MMD -MP -MF $@.d

cmd_c_to_o_name = CC
cmd_c_to_o = $(CC) $(CFLAGS) $(target_flags) $(DEPFLAGS) -c $< -o $@

cmd_cc_to_o_name = CXX
cmd_cc_to_o = $(CXX) $(CXXFLAGS) $(target_flags) -I$(OUTDIR)/$(get_target)/cxx/cincludes $(DEPFLAGS) -c $< -o $@

cmd_cincludes_name = GEN
cmd_cincludes = { echo 'extern "C" {'; cat $<; echo '};'; } >$@

cmd_o_to_elf_name = LD
cmd_o_to_elf = $(LD) $(LDFLAGS) $(target_flags) $^ $(LIBS) -o $@

cmd_clean_name = CLEAN
cmd_clean = rm -rf $(1)

cmd_gdb_name = GDB
cmd_gdb = $(MAKE) $(1) && gdb $(1)

cmd_run_name = RUN
cmd_run = $(MAKE) $(1) && ./$(1)

.SECONDARY:
.PHONY: all
all: $(BINS_debug) $(BINS_release)

-include $(call rwildcard,$(OUTDIR),*.d)

$(OUTDIR)/debug/bin/%: $(OUTDIR)/debug/mains/%.o $(OBJFILES_SRC_debug)
	$(call cmd,o_to_elf)
$(OUTDIR)/release/bin/%: $(OUTDIR)/release/mains/%.o $(OBJFILES_SRC_release)
	$(call cmd,o_to_elf)

$(OUTDIR)/debug/cxx/bin/%: $(OUTDIR)/debug/cxx/mains/%.o $(OBJFILES_SRC_debug)
	$(call cmd,o_to_elf)
$(OUTDIR)/release/cxx/bin/%: $(OUTDIR)/release/cxx/mains/%.o $(OBJFILES_SRC_release)
	$(call cmd,o_to_elf)

$(OUTDIR)/debug/%.o: %.c
	$(call cmd,c_to_o)
$(OUTDIR)/release/%.o: %.c
	$(call cmd,c_to_o)

$(OUTDIR)/debug/cxx/%.o: %.cc $(CINCLUDES_debug)
	$(call cmd,cc_to_o)
$(OUTDIR)/release/cxx/%.o: %.cc $(CINCLUDES_release)
	$(call cmd,cc_to_o)

$(OUTDIR)/debug/cxx/cincludes/%.h: include/%.h
	$(call cmd,cincludes)
$(OUTDIR)/release/cxx/cincludes/%.h: include/%.h
	$(call cmd,cincludes)

.PHONY: run-%
run-%:
	$(call cmd,run,$(OUTDIR)/release/$(call binpath,$(patsubst run-%,%,$@)))

.PHONY: debug-%
debug-%:
	$(call cmd,gdb,$(OUTDIR)/debug/$(call binpath,$(patsubst debug-%,%,$@)))

.PHONY: run-tests
run-tests:
	$(call cmd,run,$(OUTDIR)/debug/$(call binpath,run_tests))

.PHONY: clean
clean:
	$(call cmd,clean,$(OUTDIR))
