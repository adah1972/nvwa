#! /bin/sh

# Intermediate Doxyfile
DOXYFILE_TMP=nvwa.dox

# Whether/how to generate LaTeX documents
if [ "$1" = "latex" ]; then
  GENERATE_LATEX=YES
elif [ "$1" = "ps" ]; then
  GENERATE_LATEX=YES
  PDF_HYPERLINKS=NO
  USE_PDFLATEX=NO
elif [ "$1" = "pdf" ]; then
  GENERATE_LATEX=YES
  PDF_HYPERLINKS=YES
  USE_PDFLATEX=YES
elif [ "$1" = "pdf2" ]; then
  GENERATE_LATEX=YES
  PDF_HYPERLINKS=YES
  USE_PDFLATEX=NO
fi

# Determine the Doxygen engine
if [ "$DOXYGEN" = "" ]; then
  DOXYGEN=doxygen
fi

# Determine Doxygen options
if [ "$PDF_HYPERLINKS" = "" ]; then
  PDF_HYPERLINKS=NO
fi
if [ "$USE_PDFLATEX" = "" ]; then
  USE_PDFLATEX=NO
fi
if [ "$GENERATE_LATEX" = "" ]; then
  if [ "$PDF_HYPERLINKS" = "YES" ]; then
    GENERATE_LATEX=YES
  elif [ "$USE_PDFLATEX" = "YES" ]; then
    GENERATE_LATEX=YES
  else
    GENERATE_LATEX=NO
  fi
fi

# Set the options in the intermediate Doxyfile
cp -p Doxyfile $DOXYFILE_TMP
if [ "$GENERATE_LATEX" = "YES" ]; then
  sedfile 's/\(GENERATE_LATEX *=\).*/\1 YES/' $DOXYFILE_TMP
  if [ "$PDF_HYPERLINKS" = "YES" ]; then
    sedfile 's/\(PDF_HYPERLINKS *=\).*/\1 YES/' $DOXYFILE_TMP
  else
    sedfile 's/\(PDF_HYPERLINKS *=\).*/\1 NO/'  $DOXYFILE_TMP
  fi
  if [ "$USE_PDFLATEX" = "YES" ]; then
    sedfile 's/\(USE_PDFLATEX *=\).*/\1 YES/' $DOXYFILE_TMP
  else
    sedfile 's/\(USE_PDFLATEX *=\).*/\1 NO/'  $DOXYFILE_TMP
  fi
else
  sedfile 's/\(GENERATE_LATEX *=\).*/\1 NO/'  $DOXYFILE_TMP
fi

# Work around an expression that will confuse Doxygen
mv -i ../nvwa/static_mem_pool.h ..
sed 's/(_Gid < 0)/true/' ../static_mem_pool.h >../nvwa/static_mem_pool.h
$DOXYGEN $DOXYFILE_TMP
mv -f ../static_mem_pool.h ../nvwa/

# Remove the intermediate Doxyfile
rm $DOXYFILE_TMP

# Override the default style sheet of Doxygen 1.3.9.1 (font too big!)
#cp -p doxygen.css ../doc/html/

# Make LaTeX documents
if [ "$GENERATE_LATEX" = "YES" ]; then
  cd ../doc/latex

  # Remove the URIs in EPS files
  for file in *.eps
  do
    ed -s <<!EOF "$file"
      g/\[ \/Rect/.,.+4d
      w
      q
!EOF
  done

  # The LaTeX output automatically changes the operator "->" to
  # "\rightarrow", which does not look right.
  grepsedfile 'operator \$\\rightarrow\$ *' 'operator->' *.tex
  grepsedfile '\$\\rightarrow\$ *' '->' *.tex

  if [ "$PDF_HYPERLINKS" = "NO" -a "$USE_PDFLATEX" = "NO" ]; then
    make clean ps
    ps2pdf -sPAPERSIZE=a4 refman.ps refman.pdf
  else
    if [ "$PDF_HYPERLINKS" = "YES" ]; then
      # Work around a bug in Doxygen 1.5.1 when PDF_HYPERLINKS=YES.
      # It is fixed in Doxygen 1.5.3, so the following line will be
      # commented out or removed in the future.
      grepsedfile '\(subsubsection\[[^]]*\)\[\]' '\1[\\mbox{]}' *.tex
    fi

    # This is tested to work with MiKTeX 2.5, Doxygen 1.5.1 (Cygwin),
    # and Graphviz 2.8 (Windows).  Newer versions of Graphviz do NOT
    # work for me.
    #
    # USE_PDFLATEX=NO (option "pdf2") may not work the first time it is
    # run.  To work around this issue, run the script with the option
    # "pdf" first.
    rm -f refman.pdf
    make

    # Doxygen 1.5.1 has problems with "make refman.pdf" when
    # PDF_HYPERLINKS=YES and USE_PDFLATEX=NO, and must use the bare
    # "make", as shown above.  It is fixed in Doxygen 1.5.3, and the
    # following line is now necessary.
    [ -f refman.pdf ] || make refman.pdf
  fi
fi
