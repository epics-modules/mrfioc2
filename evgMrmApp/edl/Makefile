UIC=evgsoftseq

#UICSRC=$(addsuffix .ui,$(UIC))
UICPY=$(patsubst %,ui_%.py,$(UIC))

all: ui

.PHONY: all ui clean

ui: $(UICPY)

ui_%.py: %.ui
	pyuic4 -o $@ $<

clean:
	rm -f $(UICPY)
