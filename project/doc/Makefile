FILE=doc

make:
	latex $(FILE).tex
	bibtex $(FILE)
	latex $(FILE).tex
	latex $(FILE).tex
	dvips -t a4 $(FILE).dvi	
	ps2pdf $(FILE).ps
	make clean

clean:
	rm -f *.aux *.log *.dvi *.ps *.bbl *.blg *.toc *.out

clean-all: clean
	rm -f *.pdf
