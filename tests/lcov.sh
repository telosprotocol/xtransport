set -e

pids=$(ps -ef | grep SimpleHTTPServer | grep -v grep | awk -F' ' '{print $2}')
echo "pids: $pids"
if [ -n "$pids" ]; then
    kill -9 $pids
fi

parent_path=$(pwd)

cd cbuild

bin/Linux/xtransport_test
abs_path=`pwd`"/src/xtopcom/xtransport/CMakeFiles/xtransport.dir/src/"

lcov -d $abs_path -t 'test' -o 'test.info' -b . -c
echo "=========================1"

lcov --remove test.info \
    '/usr/*' \
    $parent_path'/src/xtopcom/xbase/*' \
    $parent_path'/src/xtopcom/xdepends/*' \
    $parent_path'/src/xtopcom/xpbase/*' \
    $parent_path'/src/xtopcom/xkad/*' \
    $parent_path'/src/xtopcom/xtransport/proto/*' \
    -o xtransport.info
echo "=========================2"
genhtml -o result xtransport.info
echo "=========================3"
cd result

echo ''
echo ''
echo ''

echo "visit this link to view code recover: http://0.0.0.0:8000"
echo ''
echo ''
python -m SimpleHTTPServer > /dev/null 2>&1 &

echo ''
