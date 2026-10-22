MAKE=/usr/bin/make
export_dir="./builds"
dest_data="./debian/se"
git_builder="debuild -i -I -us -uc"
# The upstream tarball is built from the current branch, so `make deb`
# works from any branch, not only master.
git_branch=$(shell git rev-parse --abbrev-ref HEAD)

.PHONY: all documentation bin test clean deb

all: clean documentation bin

documentation:
	@$(MAKE) -C doc all

bin:
	@$(MAKE) -C src all

test: bin
	@sh tests/run.sh ./se

clean:
	@$(MAKE) -C src clean
	@$(MAKE) -C doc clean
	@rm -rf $(export_dir)

deb: clean documentation bin
	@gbp buildpackage --git-ignore-new --git-export-dir=$(export_dir) --git-upstream-tree=BRANCH --git-upstream-branch=$(git_branch) --git-debian-branch=$(git_branch) --git-dist=unstable --git-builder=$(git_builder)
