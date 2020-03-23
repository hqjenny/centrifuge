export RDIR="$(realpath $( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )/../../)"
echo "Set \$RDIR to $RDIR"
export PATH=$PATH:$RDIR/tools/centrifuge/deploy

