for FILE in ./tests/*.lua
do
    echo "Running test ${FILE}"
    ./build/bin/Debug/luacppc $FILE
done


# for file in files:
#     cmd = './build/bin/Debug/luacppc {}'.format(file)
#     cmd.replace('\\', '/')
#     os.system(cmd)
