The source code in this directory comes from https://github.com/google/reftable
and can be updated by doing:

   rm -rf reftable-repo && \
   git clone https://github.com/google/reftable reftable-repo && \
   cp reftable-repo/c/*.[ch] reftable/ && \
   cp reftable-repo/LICENSE reftable/
   git --git-dir reftable-repo/.git show --no-patch origin/master \
      > reftable/VERSION

Bugfixes should be accompanied by a test and applied to upstream project at
https://github.com/google/reftable.
