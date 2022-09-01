#!/bin/bash

function stap-build() (
    cd $STAP_BASE
    local s=$(time_now)
    ./configure  --prefix=$PWD/target
    local ec=$(time_now)
    make all
    local em=$(time_now)
    sudo make install || true # install 到target中
    local ei=$(time_now)
    echo "time: config " $(time_format_time_diff $s $ec)
    echo "time: make all " $(time_format_time_diff $ec $em)
    echo "time: install " $(time_format_time_diff $em $ei)
    echo "time: all " $(time_format_time_diff $s $ei)
    sudo rm /usr/bin/wg-stap || true
    sudo rm /usr/bin/stap || true
    sudo ln -s $PWD/target/bin/stap /usr/bin/wg-stap
    sudo ln -s $PWD/target/bin/stap /usr/bin/stap
)

function find-hostpid(){
  local cid="$1"
  local pidns=$(sudo ls -l /proc/$(docker inspect $cid  | jq -r '.[0].State.Pid')/ns/pid|rg -o '\[(.*)\]' -r '$1')
  local targetpid="$2"
  local pids=$(sudo ps -a -o pidns,pid,cmd |grep "$pidns")
  while IFS= read -r line; do
        # echo "-$line-"
        local pidns=$(echo $line| awk '{print($1)}')
        local pid=$(echo $line| awk '{print($2)}')
        if [ -f "/proc/$pid/status" ] ;then
                local pidsts=$(cat /proc/$pid/status)
                local nspid=$(echo "$pidsts"|grep NSpid)
                local pid_in_docker=$(echo "$nspid"|awk '{print $3}')
                local pid_in_host=$(echo "$nspid"|awk '{print $2}')
                # echo $pid_in_docker $targetpid
                if [[ "$pid_in_docker" == "$targetpid" ]]; then
                        echo "$pid_in_host"
                        return
                fi
        fi
  done <<< "$pids"
}

function stap-docker() {
  local cid=$1
  local pid_in_docker=$2
  local stappath=$3
  local pid_in_host=$(find-hostpid "$cid" "$pid_in_docker")
  local base=$(docker inspect $cid  | jq -r '.[0].GraphDriver.Data.MergedDir')
  echo $pid_in_host
  echo $base
  echo $stappath

  local cfg="-v -DDEBUG_TASK_FINDER=9 -DDEBUG_UPROBES=9 -DDEBUG_TASK_FINDER_VMA=9"
  #sudo wg-stap  $cfg -x $pid_in_host  --sysroot="$base" -e "probe process(\"/stap/docker-test\").function(\"a\") {printf(\" a %s  %s \n\",\$\$parms,user_string(\$s))}"
  local cmd="sudo wg-stap $cfg -x $pid_in_host  --sysroot=$base  $stappath"
  echo $cmd
  eval "$cmd"
}

