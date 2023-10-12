#!/bin/bash
#########################################################################
# File Name: load.sh
# Author: LiHongjin
# mail: 872648180@qq.com
# Created Time: 2023年07月12日 星期三 22时59分01秒
#########################################################################
set -e

modName=globalmem

echo "======> building... <======"
echo ""
make
echo "======> build finish <======"


echo "======> insmod... <======"
sudo insmod ${modName}.ko
echo "======> insmod finish <======"
echo


echo "======> create device <======"
cat /proc/devices | grep $modName
echo

echo "======> create device node <======"
ls -al /dev | grep lhj_dev
echo


echo "======> loaded mode <======"
echo `lsmod | grep $modName`
echo


echo "======> test begin <======"
echo "======> test finish <======"
echo


echo "======> rmmod... <======"
sudo rmmod $modName
make clean
echo "======> rmmod finish <======"

set +e
