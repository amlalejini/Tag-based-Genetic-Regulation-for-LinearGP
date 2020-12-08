#!/bin/sh

set -ev

rm -rf ./supplemental/
rm -rf ./supplemental_files/
Rscript -e "bookdown::render_book('index.Rmd', 'bookdown::gitbook')"
Rscript -e "bookdown::render_book('index.Rmd', 'bookdown::pdf_book')"
mv _book supplemental
mv _bookdown_files supplemental_files

