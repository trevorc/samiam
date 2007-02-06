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

sam_so_err() {
    echo "couldn't find sam.so."
    echo "please run ./compile.sh"
    exit 1
}

SAM_SO=`find build -name sam.so 2>/dev/null` || sam_so_err

export LD_LIBRARY_PATH=../../build/libsam
export PYTHONPATH=`dirname $SAM_SO`

python_args="$pyfile $samfile"
