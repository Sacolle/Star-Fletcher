cmp=~/fletcher-base/original/sources.txt
file=./sources.txt

make run > $file

awk -F'[:[:space:]]+' '
NR==FNR { 
    a[FNR,1]=$2; a[FNR,2]=$4 
    next 
} 
{ 
    diff1 = $2 - a[FNR,1]
    diff2 = $4 - a[FNR,2]
    printf "%d: %.9f + %.9f\n", $1, diff1, diff2
}' <(grep -E "^[0-9]+:" $file) <(grep -E "^[0-9]+:" $cmp)