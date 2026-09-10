MAKE=/usr/bin/make
export_dir="./builds"
dest_data="./debian/se"
git_builder="debuild -i -I -us -uc"

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
	@gbp buildpackage --git-ignore-new --git-export-dir=$(export_dir) --git-upstream-tree=master --git-dist=unstable --git-builder=$(git_builder)
