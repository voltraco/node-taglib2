if [ -n "$ELECTRON" ]
then
  CC=clang CXX=clang++ cmake-js rebuild -r electron -v 1.2.5 -a ia64
else
  cmake-js compile
fi

