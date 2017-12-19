FILE=doc

make:
	latex $(FILE).tex
	bibtex $(FILE)
	latex $(FILE).tex
	latex $(FILE).tex
	dvips -t a4 $(FILE).dvi	
	ps2pdf $(FILE).ps
	make clean

silent:
	@latex $(FILE).tex       > /dev/null
	@bibtex $(FILE)          > /dev/null
	@latex $(FILE).tex       > /dev/null
	@latex $(FILE).tex       > /dev/null
	@dvips -t a4 $(FILE).dvi > /dev/null
	@ps2pdf $(FILE).ps       > /dev/null
	@make clean              > /dev/null

clean:
	rm -f *.aux *.log *.dvi *.ps *.bbl *.blg *.toc *.out

clean-all: clean
	rm -f *.pdf
