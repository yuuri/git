#!/bin/sh

test_description='git log --graph of skewed merges'

. ./test-lib.sh

test_expect_success 'setup left-skewed merge' '
	git checkout --orphan _a && test_commit A &&
	git branch _b &&
	git branch _c &&
	git branch _d &&
	git branch _e &&
	git checkout _b && test_commit B &&
	git checkout _c && test_commit C &&
	git checkout _d && test_commit D &&
	git checkout _e && git merge --no-ff _d -m E &&
	git checkout _a && git merge --no-ff _b _c _e -m F
'

cat > expect <<\EOF
*---.   F
|\ \ \
| | | * E
| |_|/|
|/| | |
| | | * D
| |_|/
|/| |
| | * C
| |/
|/|
| * B
|/
* A
EOF

test_expect_success 'log --graph with left-skewed merge' '
	git log --graph --pretty=tformat:%s | sed "s/ *$//" > actual &&
	test_cmp expect actual
'

test_done
