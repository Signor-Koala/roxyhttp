# RoxyHTTP

_A simple HTTP server written in C, and extensible through Lua_

---

## About
A (very) simple HTTP server written in C designed to simple to use and deploy in POSIX systems.

By default it simply serves files in a provided directory. This functionality can be overridden by 
functions that directly process the request and produce a response, which is done in plain Lua.

This project is currently mostly incomplete and has many rough edges.

## Prerequisites
- cmake >= 3.30
- \<add others here\>

## Setup

    git clone https://github.com/Signor-Koala/roxyhttp.git

    cd roxyhttp

    cmake -B build

    cmake --build build

The executable produced resides in `build/roxyhttp`

Currently only tested with Arch Linux

## Usage
TODO!
