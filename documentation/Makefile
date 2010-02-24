ASCIIDOC=asciidoc
ASCIIOPTS=-d article -a toc -a numbered

DBLATEX=dblatex
INKSCAPE=inkscape

DOCSRC += evr-usage.txt
evr_usage_txt_PNG += images/mrf-overview.png
evr_usage_txt_PNG += images/mrf-frames.png
evr_usage_txt_PNG += images/mrf-evr-blocks.png

images_mrf_overview_png_PNGOPTS += -d 100
images_mrf_frames_png_PNGOPTS += -d 100
images_mrf_evr_blocks_png_PNGOPTS += -d 60

USE_XHTML=YES
XHTML_YES=-b xhtml11
XHTML_NO=-b html4
XHTML=$(XHTML_$(USE_XHTML))

# $1 is the doc filename (ie evr-usage.txt)
define docrules

$(1)_ROOT = $(1:.txt=)

$(1)_VAR = $$(subst /,_,$$(subst -,_,$$(subst .,_,$(1))))

PNG += $$($$($(1)_VAR)_PNG)

$$($(1)_VAR)_ASCIIOPTS += $$(XHTML) $$(ASCIIOPTS)

html: $$($(1)_ROOT).html

$$($(1)_ROOT).html: $(1) $$($$($(1)_VAR)_PNG) $$($$($(1)_VAR)_DEP)
	$$(ASCIIDOC) $$($$($(1)_VAR)_ASCIIOPTS) $$<

$$($(1)_VAR)_DBOPTS += -b docbook $$(ASCIIOPTS)

$$($(1)_ROOT).xml: $(1) $$($$($(1)_VAR)_PNG) $$($$($(1)_VAR)_DEP)
	$$(ASCIIDOC) $$($$($(1)_VAR)_DBOPTS) $$<

pdf: $$($(1)_ROOT).pdf

$$($(1)_ROOT).pdf: $$($(1)_ROOT).xml
	$$(DBLATEX) $$<

info: info-$$($(1)_VAR)

info-$$($(1)_VAR):
	@echo "Target=$(1)"
	@echo "  $$($(1)_VAR)_PNG=$$($$($(1)_VAR)_PNG)"
	@echo "  $$($(1)_VAR)_DEP=$$($$($(1)_VAR)_DEP)"
	@echo "  $$($(1)_VAR)_ASCIIOPTS=$$($$($(1)_VAR)_ASCIIOPTS)"
	@echo "  $$($(1)_VAR)_DBOPTS=$$($$($(1)_VAR)_DBOPTS)"

clean::
	rm -f $$($(1)_ROOT).html $$($(1)_ROOT).xml $$($(1)_ROOT).pdf

endef # docrules

define pngrules

$(1)_ROOT = $(1:.png=)

$(1)_VAR = $$(subst /,_,$$(subst -,_,$$(subst .,_,$(1))))

$$($(1)_VAR)_PNGOPTS += $$(PNGOPTS)

png: $(1)

$(1): $$($(1)_ROOT).svg
	$$(INKSCAPE) -z $$< $$($$($(1)_VAR)_PNGOPTS) -e $$@

info: info-$$($(1)_VAR)

info-$$($(1)_VAR):
	@echo "Target $(1)"
	@echo "  $$($(1)_VAR)_PNGOPTS=$$($$($(1)_VAR)_PNGOPTS)"

clean::
	rm -f $(1)

endef # pngrules

all: html

doc: html pdf

$(foreach d,$(DOCSRC),$(eval $(call docrules,$(d))))

$(foreach p,$(PNG),$(eval $(call pngrules,$(p))))

help:
	@echo "Targets:"
	@echo "          all html pdf clean"
	@echo "          info"

clean::
	rm -f *.aux *.out *.log
