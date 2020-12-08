#!/bin/sh

set -ev

rm -rf ./_book/
rm -rf ./_bookdown_files/
Rscript -e "bookdown::render_book('index.Rmd', 'bookdown::gitbook')"
Rscript -e "bookdown::render_book('index.Rmd', 'bookdown::pdf_book')"

