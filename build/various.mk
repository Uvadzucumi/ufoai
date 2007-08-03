# sync sourceforget.net svn to local svn dir
LOCAL_SVN_DIR=/var/lib/svn
rsync:
	export RSYNC_PROXY=rsync-svn.sourceforge.net:80; rsync -a rsync-svn-u::svn/ufoai/* $(LOCAL_SVN_DIR)

update-maps
	rsync -avz rsync://mattn.ninex.info/ufo base/maps

# generate doxygen docs
doxygen-docs:
	doxygen src/docs/doxyall

# debian packages
# build this first - install ufoai-tools and then build debdata
debbinary:
	debuild binary-arch

debdata:
	debuild binary-indep

deb:
	debuild binary

pdf-manual:
	$(MAKE) -C src/docs/tex
