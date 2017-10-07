LATEX	= latex -shell-escape
BIBTEX	= bibtex
DVIPS	= dvips
DVIPDF  = dvipdft
XDVI	= xdvi -gamma 4
GH		= gv

EXAMPLES = $(wildcard *.c)
SRC	:= $(shell egrep -l '^[^%]*\\begin\{document\}' *.tex)
TRG	= $(SRC:%.tex=%.dvi)
PSF	= $(SRC:%.tex=%.ps)
PDF	= $(SRC:%.tex=%.pdf)

all: conc

pdf: $(PDF)
	
ps: $(PSF)
	
$(TRG): %.dvi: %.tex $(EXAMPLES)
	#one way of including source code is to use pygments
	pygmentize -f latex -o __${EXAMPLES}.tex ${EXAMPLES}
	#requires that you \include{pygments.tex} in your preamble
				
	$(LATEX) $<
	$(BIBTEX) $(<:%.tex=%)
	$(LATEX) $<
	$(LATEX) $<
	#remove the pygmentized output to avoid cluttering up the directory
	#rm __${SRC}.tex
										
										
$(PSF):%.ps: %.dvi
	$(DVIPS) -R -Poutline -t letter $< -o $@
											
$(PDF): %.pdf: %.ps
	ps2pdf $<
												
show: $(TRG)
	@for i in $(TRG) ; do $(XDVI) $$i & done
													
showps: $(PSF)
	@for i in $(PSF) ; do $(GH) $$i & done

conc: produce_consume.o
	gcc produce_consume.o -o run_conc -pthread

produce_consume.o:
	gcc -c produce_consume.c -g
														
clean:
	rm -f *.pdf *.ps *.dvi *.out *.log *.aux *.bbl *.blg *.pyg *.o run_conc
															
.PHONY: all show clean ps pdf showps
