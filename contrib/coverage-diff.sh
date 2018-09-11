#!/bin/sh

# Usage: 'contrib/coverage-diff.sh <version1> <version2>
# Outputs a list of new lines in version2 compared to version1 that are
# not covered by the test suite. Assumes you ran
# 'make coverage-test coverage-report' from root first, so .gcov files exist.

V1=$1
V2=$2

diff_lines () {
	while read line
	do
		if echo $line | grep -q -e "^@@ -([0-9]+)(,[0-9]+)? \\+([0-9]+)(,[0-9]+)? @@.*"
		then
			line_num=$(echo $line \
				| awk 'match($0, "@@ -([0-9]+)(,[0-9]+)? \\+([0-9]+)(,[0-9]+)? @@.*", m) { print m[3] }')
		else
			echo "$line_num:$line"
			if ! echo $line | grep -q -e "^-"
			then
				line_num=$(($line_num + 1))
			fi
		fi
	done
}

files=$(git diff --raw $V1 $V2 \
	| grep \.c$ \
	| awk 'NF>1{print $NF}')

for file in $files
do
	git diff $V1 $V2 -- $file \
		| diff_lines \
		| grep ":+" \
		| sed 's/:/ /g' \
		| awk '{print $1}' \
		| sort \
		>new_lines.txt

	hash_file=$(echo $file | sed "s/\//\#/")
	cat "$hash_file.gcov" \
		| grep \#\#\#\#\# \
		| sed 's/    #####: //g' \
		| sed 's/\:/ /g' \
		| awk '{print $1}' \
		| sort \
		>uncovered_lines.txt

	comm -12 uncovered_lines.txt new_lines.txt \
		| sed -e 's/$/\)/' \
		| sed -e 's/^/\t/' \
		>uncovered_new_lines.txt

	grep -q '[^[:space:]]' < uncovered_new_lines.txt && \
		echo $file && \
		git blame -c $file \
			| grep -f uncovered_new_lines.txt

	rm -f new_lines.txt uncovered_lines.txt uncovered_new_lines.txt
done

