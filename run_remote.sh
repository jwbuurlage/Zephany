#/usr/bin/bash
rsync -r bin parallella:~/zephany
ssh parallella "source ~/.bashrc; ~/zephany/$1"
