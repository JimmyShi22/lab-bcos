# "Copyright [2018] <fisco-bcos>"
# @ function : one-click install shell script(appliable for centos and ubuntu)
# @ Require  : yum or apt, git are ready
# @ attention: if dependecies are downloaded failed, 
#              please fetch packages into "deps/src" from https://github.com/bcosorg/lib manually
#              and execute this shell script later
# @ author   : yujiechen  
# @ file     : build.sh
# @ date     : 2018

#!/bin/bash

current_dir=`pwd`"/../.."
Ubuntu_Platform=0
Centos_Platform=1
clear_cache()
{ 
    cd ${current_dir}
    execute_cmd "rm -rf deps/src/*stamp"
}

LOG_ERROR()
{
    content=${1}
    echo -e "\033[31m"${content}"\033[0m"
}

LOG_INFO()
{
    content=${1}
    echo -e "\033[32m"${content}"\033[0m"
}

execute_cmd()
{
    command="${1}"
    #LOG_INFO "RUN: ${command}"
    eval ${command}
    ret=$?
    if [ $ret -ne 0 ];then
        LOG_ERROR "FAILED execution of command: ${command}"
        clear_cache
        exit 1
    else
        LOG_INFO "SUCCESS execution of command: ${command}"
    fi
}

# get platform: now support debain/ubuntu, fedora/centos, oracle
get_platform()
{
    uname -v > /dev/null 2>&1 || { echo >&2 "ERROR - Require 'uname' to identify the platform."; exit 1; }
    case $(uname -s) in
    Darwin)
        LOG_ERROR "Not Support MAC OS Yet!"
        exit 1;;
    FreeBSD)
        LOG_ERROR "Not Support FreeBSD Yet!"
        exit 1;;
    Linux)
        if [ -f "/etc/arch-release" ]; then
            LOG_ERROR "Not Support arch-linux Yet!"
        elif [ -f "/etc/os-release" ];then
            DISTRO_NAME=$(. /etc/os-release; echo $NAME)
            case $DISTRO_NAME in
            Debian*|Ubuntu)
                LOG_INFO "Debian*|Ubuntu Platform"
                return ${Ubuntu_Platform};; #ubuntu type
            Fedora|CentOS*)
                LOG_INFO "Fedora|CentOS* Platform"
                return ${Centos_Platform};; #centos type
            Oracle*)
                LOG_INFO "Oracle Platform"
                return ${Centos_Platform};; #oracle type
            esac
        else
            LOG_ERROR "Unsupported Platform"
        fi
    esac
}


install_ubuntu_package()
{
for i in $@ ;
do 
    LOG_INFO "install ${i}";
    execute_cmd "sudo apt-get -y install ${i}";
done
}

install_centos_package()
{
for i in $@ ;
do
    LOG_INFO "install ${i}";
    execute_cmd "sudo yum -y install ${i}";
done
}

#install ubuntu package
install_ubuntu_deps()
{
install_ubuntu_package "cmake" "openssl" "libssl-dev" "libcurl4-openssl-dev" "libgmp-dev" "libleveldb-dev"
}

# install centos package
install_centos_deps()
{
install_centos_package "cmake3" "gcc-c++" "openssl" "openssl-devel" "leveldb-devel" "curl-devel" "gmp-devel"
}

install_all_deps()
{
    get_platform
        platform=`echo $?`
    if [ ${platform} -eq ${Ubuntu_Platform} ];then
        install_ubuntu_deps
    elif [ ${platform} -eq ${Centos_Platform} ];then
        install_centos_deps
    else
        LOG_ERROR "Unsupported Platform"
        exit 1
    fi
}


build_ubuntu_source()
{
# build source
execute_cmd "mkdir -p build && cd build/"
execute_cmd "cmake -DTESTS=ON .. "
execute_cmd "make"
}

build_centos_source()
{
# build source
execute_cmd "mkdir -p build && cd build/"
execute_cmd "cmake3 -DTESTS=ON .. "
execute_cmd "make && cd ${current_dir}"
}

build_source()
{
cd ${current_dir}
get_platform
platform=`echo $?`
if [ ${platform} -eq ${Ubuntu_Platform} ];then
    build_ubuntu_source
elif [ ${platform} -eq ${Centos_Platform} ];then
     build_centos_source
else
    LOG_ERROR "Unsupported Platform, Exit"
    exit 1
fi
}

install_all()
{
	install_all_deps
	build_source
}

cd ${current_dir}
execute_cmd "git submodule update --init"
install_all
