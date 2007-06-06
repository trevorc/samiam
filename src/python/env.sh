sam_so_err() {
    echo "couldn't find sam.so."
    echo "please run ./compile.sh"
    exit 1
}

basedir=../python
SAM_SO=`find $basedir/build/ -name sam.so 2>/dev/null` || sam_so_err

export LD_LIBRARY_PATH=$basedir/../../build/libsam
export PYTHONPATH=$basedir/`dirname $SAM_SO`
