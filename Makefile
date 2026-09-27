MZRE ?= mzretools
MZRETOOLDIR := $(MZRE)/tools
DOSBOX ?= dosbox
DOSBUILD := DOSBOX=$(DOSBOX) $(MZRETOOLDIR)/dosbuild.sh
ASM ?= uasm
# UASM: no copyright banner, 8086 instructions, MASM compatibility
ASMFLAGS := -q -0 -Zm
MZDIFF := $(MZRE)/build/mzdiff
LINK_TOOLCHAIN ?= msc510
# ms link: create mapfile, verbose
LINKFLAGS := /M /I

BUILDDIR := build
MAPDIR := map

.PHONY: all egame verify clean
all: egame

$(BUILDDIR):
	mkdir -p $@

#
# egame.exe skeleton reconstruction from the IDA listing.
# lst2asm converts lst/EGAME.EXE.lst to a flat asm source, applying the
# per-site encoding fixes and assumes from conf/egame.json; lst/egame.inc
# carries the SREGS structure the listing references but never defines.
#
EGAME_EXE := $(BUILDDIR)/egame.exe

ifneq ($(wildcard lst/EGAME.EXE.lst),)
# IDA listing present — regenerate the skeleton from it
EGAME_ASM := $(BUILDDIR)/egame.asm
$(EGAME_ASM): lst/EGAME.EXE.lst conf/egame.json lst/egame.inc | $(BUILDDIR)
	python3 $(MZRETOOLDIR)/lst2asm.py lst/EGAME.EXE.lst $@ conf/egame.json
else
# no listing in the repo — assemble the checked-in skeleton
EGAME_ASM := src/egame.asm
endif

$(BUILDDIR)/egame.obj: $(EGAME_ASM)
	$(ASM) $(ASMFLAGS) -Fo$@ $<

egame: $(EGAME_EXE)
$(EGAME_EXE): $(BUILDDIR)/egame.obj
	@$(DOSBUILD) link $(LINK_TOOLCHAIN) -i $< -o $@ -f "$(LINKFLAGS)"

# Whole-load-image byte comparison against the original EGAME.EXE.
verify: $(EGAME_EXE)
	python3 tools/compare_exe.py EGAME.EXE $(EGAME_EXE)

#
# Driver overlays (jump-table drivers, no IDA lst needed):
#   drvmap.py seeds mzmap with the OvlHeader jump table; drv2asm.py emits a
#   byte-exact db-skeleton asm. MGRAPHIC target uses the English F19 binary
#   (the RU one differs ~21%); ASOUND is identical in both.
#
F19EN ?= /home/xor/games/f19/F19

# When an Ada Script listing (lst/<d>_ada.lst, from ~/vextest/ada.py) exists,
# asmfix.py emits real mnemonics where they reassemble byte-exact and db
# bytes elsewhere -- a readable skeleton instead of a pure byte blob.
ifneq ($(wildcard lst/mgraphic_ada.lst),)
$(BUILDDIR)/mgraphic.exe $(BUILDDIR)/mgraphic.obj $(BUILDDIR)/mgraphic.asm &: tools/asmfix.py tools/drv2asm.py map/mgraphic_en.map lst/mgraphic_ada.lst | $(BUILDDIR)
	python3 tools/asmfix.py $(F19EN)/MGRAPHIC.EXE map/mgraphic_en.map \
	    $(BUILDDIR)/mgraphic.asm --lst lst/mgraphic_ada.lst --build $(BUILDDIR)
else
$(BUILDDIR)/mgraphic.asm: tools/drv2asm.py map/mgraphic_en.map | $(BUILDDIR)
	python3 tools/drv2asm.py $(F19EN)/MGRAPHIC.EXE map/mgraphic_en.map $@
$(BUILDDIR)/mgraphic.exe: $(BUILDDIR)/mgraphic.obj
	@$(DOSBUILD) link $(LINK_TOOLCHAIN) -i $< -o $@ -f "$(LINKFLAGS)"
endif
ifneq ($(wildcard lst/asound_ada.lst),)
$(BUILDDIR)/asound.exe $(BUILDDIR)/asound.obj $(BUILDDIR)/asound.asm &: tools/asmfix.py tools/drv2asm.py map/asound.map lst/asound_ada.lst | $(BUILDDIR)
	python3 tools/asmfix.py ASOUND.EXE map/asound.map \
	    $(BUILDDIR)/asound.asm --lst lst/asound_ada.lst --build $(BUILDDIR)
else
$(BUILDDIR)/asound.asm: tools/drv2asm.py map/asound.map | $(BUILDDIR)
	python3 tools/drv2asm.py ASOUND.EXE map/asound.map $@
$(BUILDDIR)/asound.exe: $(BUILDDIR)/asound.obj
	@$(DOSBUILD) link $(LINK_TOOLCHAIN) -i $< -o $@ -f "$(LINKFLAGS)"
endif
$(BUILDDIR)/%.obj: $(BUILDDIR)/%.asm
	$(ASM) $(ASMFLAGS) -Fo$@ $<

.PHONY: drivers verify-drivers
drivers: $(BUILDDIR)/mgraphic.exe $(BUILDDIR)/asound.exe
verify-drivers: drivers
	python3 tools/compare_exe.py $(F19EN)/MGRAPHIC.EXE $(BUILDDIR)/mgraphic.exe
	python3 tools/compare_exe.py ASOUND.EXE $(BUILDDIR)/asound.exe

# C declarations generated from the listing (prototypes + data layout)
hdr: $(BUILDDIR)/EGAME.EXE.h
$(BUILDDIR)/EGAME.EXE.h: lst/EGAME.EXE.lst conf/egame.json lst/egame.inc | $(BUILDDIR)
	python3 $(MZRETOOLDIR)/lst2ch.py lst/EGAME.EXE.lst $(BUILDDIR) conf/egame.json

#
# Per-routine C porting harness:
#   make port SRC=src/egui.c ROUTINES="loadColorPalette projectMapPoint" [CLFLAGS="/AS /Os"]
# Compiles SRC with MSC 5.1 under kvikdos, links a test exe, and mzdiffs each
# named routine's bytes against the original (map/egame.map extents).
# ghidra:  make seed SRC=src/egui.c ROUTINES=drawViewportLine  dumps the
#          headless-decompiled C for each routine as a starting point.
#
.PHONY: port seed
port:
	python3 tools/portcheck.py $(SRC) $(ROUTINES) $(CLFLAGS)
seed:
	python3 tools/ghidra_src.py $(ROUTINES)

clean:
	rm -f $(BUILDDIR)/egame.asm $(BUILDDIR)/egame.obj $(BUILDDIR)/egame.exe \
		$(BUILDDIR)/egame.map $(BUILDDIR)/egame.rsp $(BUILDDIR)/egame.err
