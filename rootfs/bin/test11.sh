#!sbush
echo testing cd:
pwd
cd /
pwd
cd roots/bin
pwd
cd ../..
pwd
cd ..
pwd
cd .
pwd
cd /rootfs/bin/
pwd
cd ../etc/
pwd
echo testing ls:
ls /rootfs/bin
ls
ls rootfs/
ls /rootfs/bin/abc.txt
ls /
echo testing cat:
cat /rootfs/bin/abc.txt
cat
cat roots/bin/
cat roots/bin/abc.txt
echo "testing env vars:"
export NEWVAR=/pqr/trd/
env
export PATH=$PATH:/abc/def/
echo new PATH
echo $PATH
echo tesing PS1
export PS1=newpromp#
