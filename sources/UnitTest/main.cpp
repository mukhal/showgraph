/**
 * File: UnitTest/main.cpp - Implementation and entry point for unit testing of ShowGraph
 * Copyright (C) 2009  Boris Shurygin
 */
#include "utest_impl.h"

int main(int argc, char **argv)
{
    //Test graph package
    if ( !UTestGraph())
        return -1;

    //Test graph package
    if ( !UTestGUI(argc, argv))
        return -1;

}