"""
Script: FilterColumns.py

Given a csv file, filter named columns from file.
"""


import argparse, os, copy, errno, csv, re, sys
import pandas as pd

def main():
    # Setup the commandline argument parser
    parser = argparse.ArgumentParser(description="Filter columns script.")
    parser.add_argument("-f", "--filter", type=str, nargs="+", help="What columns should we filter out of the file?")
    parser.add_argument("--file", type=str, help="What .csv file should we filter?")
    parser.add_argument("--out", type=str, help="Name of filtered file?", default="filtered.csv")
    # Extract arguments from commandline
    args = parser.parse_args()
    columns = args.filter
    in_file = args.file
    out_file = args.out

    data = pd.read_csv(in_file)
    data = data.drop(columns=columns)
    data.to_csv(out_file, index=False)

if __name__ == "__main__":
    main()