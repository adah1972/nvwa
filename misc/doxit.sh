#! /bin/sh

# Work around an expression that will confuse doxygen
mv ../nvwa/static_mem_pool.h ..
sed 's/(_Gid < 0)/true/' ../static_mem_pool.h >../nvwa/static_mem_pool.h
doxygen
mv -f ../static_mem_pool.h ../nvwa/

# Override the default style sheet of doxygen 1.3.9.1 (font too big!)
cp -p doxygen.css ../doc/html/

# Make refman.ps if needed
if [ "$1" = "ps" ]; then
  cd ../doc/latex
  make ps
fi
