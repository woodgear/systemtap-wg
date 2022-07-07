#!/bin/bash

function stap-build() {
    ./configure  --prefix=$PWD/target
    make all
    make install
}