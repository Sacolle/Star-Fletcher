PRINTOUT=1 python3 ./scripts/cross-deriv-gen.py > src/cross-deriv.gen.c   

make test ARGS="--filter=cross_derivative/cross_derivative_otwelveo_yz" > order.txt

# Extract the segment between "Macro:" and "Computed operation:"
awk '/Computed operation Macro:/{flag=1; next} /Computed operation:/{flag=0} flag' order.txt > macro.txt

# Extract the segment between "Computed operation:" and "make[1]"
awk '/Computed operation:/{flag=1; next} /make\[1\]: Leaving/{flag=0} flag' order.txt > func.txt

echo "Files 'macro.txt' and 'func.txt' created."

# Optional: Run a side-by-side diff immediately
diff macro.txt func.txt