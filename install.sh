if [[ -n "$ELECTRON" ]]
then
  cmake-js rebuild -r electron -v 1.2.5 -a ia64
else
  cmake-js compile
fi

