sh build.sh

for FILE in ./tests/*.lua
do
    echo "Running test ${FILE}"
    ./build/bin/Debug/luacppc $FILE
done

