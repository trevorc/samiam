if [ $# = 0 ]; then
    echo "need sam input file as first arg."
    echo "usage: $0 [pyfile] samfile"
    echo
    echo "try one of the following:"
    for i in tests/*.sam; do
	echo $0 $i
    done
    exit 1
fi

if [ $# = 2 ]; then
    pyfile=$1
    samfile=$2
else
    pyfile=tests/load.py
    samfile=$1
fi

. env.sh

python_args="$pyfile $samfile"
