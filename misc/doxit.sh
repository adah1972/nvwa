#! /bin/sh

# Recommended tool versions and notes:
#
#   * Graphviz 2.40
#
#     This new version works well for me
#
#   * Doxygen 1.5.1/1.5.2 for LaTeX output
#
#     1.5.3+ have wrong output in some diagrams
#
#   * Doxygen 1.5.8/1.6.2/1.8.13 for HTML output
#
#     1.5.9 has broken link on "More..."
#     1.6.0/1 left-aligns the project name
#     1.6.3 has issues with included-by graphs
#
#     Please notice that only Doxygen 1.8 has good support for C++11
#

case $(sed --version 2>&1) in
  *GNU*) sed_i () { sed -E -i "$@"; };;
  *) sed_i () { sed -E -i '' "$@"; };;
esac

function grepsedfile {
  if [ "$#" -le 2 ]; then
    return 0
  fi
  EXP=$1
  REP=$2
  shift 2
  FILES=`grep -E -l "$EXP" "$@"`
  if [ "$?" -ne 0 ]; then
    return $?
  fi
  if [ -z "$FILES" ]; then
    return 0
  fi
  echo "$FILES"
  sed_i "s/$EXP/$REP/g" $FILES
  return $?
}

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

OLD_DOXYGEN_VER=NO
$DOXYGEN --version | grep '^1\.\(5\|6\)\.' > /dev/null && OLD_DOXYGEN_VER=YES

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
  sed_i 's/(GENERATE_LATEX *=).*/\1 YES/' $DOXYFILE_TMP
  if [ "$PDF_HYPERLINKS" = "YES" ]; then
    sed_i 's/(PDF_HYPERLINKS *=).*/\1 YES/' $DOXYFILE_TMP
  else
    sed_i 's/(PDF_HYPERLINKS *=).*/\1 NO/'  $DOXYFILE_TMP
  fi
  if [ "$USE_PDFLATEX" = "YES" ]; then
    sed_i 's/(USE_PDFLATEX *=).*/\1 YES/' $DOXYFILE_TMP
  else
    sed_i 's/(USE_PDFLATEX *=).*/\1 NO/'  $DOXYFILE_TMP
  fi
else
  sed_i 's/(GENERATE_LATEX *=).*/\1 NO/'  $DOXYFILE_TMP
fi
if [ "$OLD_DOXYGEN_VER" = "YES" ]; then
  sed_i 's/ (.*nvwa\/functional\.h)/#\1/' $DOXYFILE_TMP
fi

# Work around an expression that will confuse Doxygen
if [ "$OLD_DOXYGEN_VER" = "YES" ]; then
  mv -i ../nvwa/static_mem_pool.h ..
  sed 's/(_Gid < 0)/true/' ../static_mem_pool.h >../nvwa/static_mem_pool.h
fi
$DOXYGEN $DOXYFILE_TMP
if [ "$OLD_DOXYGEN_VER" = "YES" ]; then
  mv -f ../static_mem_pool.h ../nvwa/
fi

# Remove the intermediate Doxyfile
rm $DOXYFILE_TMP

# Remove the space between -> and * in some Doxygen versions
cd ../doc/html
echo "Postprocessing HTML files"
grepsedfile 'operator-&gt; \*|operator-> \*' 'operator-\&gt;*' *.html
cd ../../misc

# Make LaTeX documents
if [ "$GENERATE_LATEX" = "YES" ]; then
  cd ../doc/latex
  echo "Postprocessing LaTeX files"

  # Remove the URIs in EPS files
  for file in *.eps
  do
    echo "$file"
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

  # Note for non-PDFLaTeX output: the package cm-super should be
  # installed, which would make the PDF file contain only Type 1 and
  # TrueType fonts.
  if [ "$PDF_HYPERLINKS" = "NO" -a "$USE_PDFLATEX" = "NO" ]; then
    make clean ps
    ps2pdf -sPAPERSIZE=a4 refman.ps refman.pdf
  else
    if [ "$PDF_HYPERLINKS" = "YES" ]; then
      # Work around a bug in Doxygen 1.5.1 when PDF_HYPERLINKS=YES.
      # It is fixed in Doxygen 1.5.3, so the following line will be
      # commented out or removed in the future.
      grepsedfile '(subsubsection\[[^]]*)\[\]' '\1[\\mbox{]}' *.tex
    fi

    # USE_PDFLATEX=NO (option "pdf2") may not work the first time it is
    # run.  To work around this issue, run the script with the option
    # "pdf" first. -- This problem does not occur with more recent LaTeX
    # distributions like MiKTeX 2.7 or TeXLive 2008.
    rm -f refman.pdf
    make

    # Doxygen 1.5.1 has problems with "make refman.pdf" when
    # PDF_HYPERLINKS=YES and USE_PDFLATEX=NO, and must use the bare
    # "make", as shown above.  It is fixed in Doxygen 1.5.3, and the
    # following line is now necessary.
    [ -f refman.pdf ] || make refman.pdf
  fi
fi
