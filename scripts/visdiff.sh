source $1
f2=$((n2 / 2))

# sfgraph < TTImax.rsf symbol="g" symbolsz=7 | sfpen

sfadd $1 $2 scale=1,-1 | sfwindow f2=$f2 n2=1 | sfgrey gainpanel=all wantframenum=no polarity=y | sfpen