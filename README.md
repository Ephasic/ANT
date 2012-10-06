Arbitrary Navn Tool -- (C) 2011-2012 Azuru
============================================================
------------------------------------------------------------

## Table of Contents
1.   Developers
2.   Credits
3.   Dependencies
4.   How to install
5.   Contact

------------------------------------------------------------

## 1. DEVELOPERS
+ [Justin "Justasic" Crawford](https://github.com/Justasic)
+ [Aaron "Dark_Aaron" Blakely](https://github.com/ablakely)
+ [Matthew "mattwb65" Barksdale](https://github.com/mattwb65)

## 2. CREDITS
In addition to the above developers we'd also like to acknowledge the
following
projects for design concepts, code, and/or inspiration.

+ CIA.vc
+ [Anope IRC Services](http://anope.org)
+ [InspIRCd](http://inspircd.org)

## 3. DEPENDENCIES
Building:
  - Linux/Unix
  - CMake
  - GCC 4.5 or higher (anything below and it causes C++11 errors)
  - CPPCMS 1.0.1 or higher (included)
  - Discount 2.1.3 or higher (included, download:
http://www.pell.portland.or.us/~orc/Code/discount/)

Segmentation fault traces:
  - for backtrace dumps: C++ include '\<execinfo.h\>'
  - for attempts to restore program before crash, include '\<setjmp.h\>'

## 4. HOW TO INSTALL
- Extract the source with unzip on linux.
- Make sure you have updated Gpp and G++ to latest versions or have
build-essentials installed.
- Run `./Config`
- Run `make`
- Run `make install`
- To start the bot(s) run `./ant`

How to start in developer mode
- Follow above instructions.
- Start the bot with `./ant -d`

How to start in Protocol Debug mode
- Follow above instructions.
- Start the bot with `./ant -n -p`

## 5. CONTACT
Our IRC: irc.Azuru.net #ANT (Ports: 6660-6669 / SSL: 5000)  or irc.AlphaChat.net #ephasic (6667 / SSL: 6697)
Git: https://github.com/Ephasic/ANT
