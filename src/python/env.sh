sam_so_err() {
    echo "couldn't find sam.so."
    echo "please run ./compile.sh"
    exit 1
}

SAM_SO=`find build -name sam.so 2>/dev/null` || sam_so_err

export LD_LIBRARY_PATH=../../build/libsam
export PYTHONPATH=`dirname $SAM_SO`
