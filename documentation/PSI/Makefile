
all: pdf

pdf: tutorial.pdf evr_manual.pdf

%.pdf: %.tex
	pdflatex -synctex=1 -interaction=nonstopmode "$<"
	bibtex $*.aux
	pdflatex -synctex=1 -interaction=nonstopmode "$<"
	pdflatex -synctex=1 -interaction=nonstopmode "$<"

%.pdf: %.svg
	inkscape -C -z --file="$<" --export-pdf="$@"

evr_manual.pdf: img/pulserGeneric.pdf
evr_manual.pdf: img/output.pdf
evr_manual.pdf: img/TTL.pdf
evr_manual.pdf: img/CML.pdf
evr_manual.pdf: img/bunchTrain.pdf
evr_manual.pdf: img/univ.pdf
evr_manual.pdf: img/templates.pdf

tutorial.pdf: img/pulserSignal.pdf
tutorial.pdf: img/pulser.pdf
tutorial.pdf: img/prescaler.pdf
tutorial.pdf: img/gen_evt_clk.pdf
tutorial.pdf: img/dbus.pdf

clean:
	rm -f tutorial.pdf evr_manual.pdf
	rm -f img/*.pdf
	rm -f *.aux *.bbl *.blg *.log *.out *.synctex.gz *.toc
