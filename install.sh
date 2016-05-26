if [[ -n "$ELECTRON" ]]
then
  cmake-js rebuild -r electron -v 1.1.3 -a ia64
else
  cmake-js compile
fi

