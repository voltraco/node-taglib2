if [[ -n "$ELECTRON" ]]
then
  cmake-js rebuild -r electron -v 0.37.5 -a ia64
else
  cmake-js compile
fi

