(C) 2011-2012 Azuru/Ephasic
Contact us at Development@Azuru.net or ant@ephasic.org

Arbitrary Navn Tool
============================================================
------------------------------------------------------------

## Table of Contents
1.   Credits
2.   Dependencies
3.   How to install
4.   Contact

------------------------------------------------------------

## 1. CREDITS
Based on the original code of CIA.vc by Micah Dowty
Based on the original code of Anope by The Anope Team.
This IRC bot was Created by Justasic and developed by Justasic on Azuru.

## 2. DEPENDENCIES
Building:
  Linux/Unix
  CMake
  GCC 4.5 or higher (anything below and it causes C++11 errors)
  CPPCMS 1.0.1 or higher (included)
  Discount 2.1.3 or higher (included, download: http://www.pell.portland.or.us/~orc/Code/discount/)
Segmentation fault traces:
  for backtrace dumps: C++ include '<execinfo.h>'
  for attempts to restore program before crash, include '<setjmp.h>'

## 3. HOW TO INSTALL
1. Extract the source with unzip on linux.
2. Make sure you have updated Gpp and G++ to latest versions or have build-essentials installed.
3. run ./Config
4. Run make
5. Run make install
6. To start the bot(s) run ./ant

How to start in developer mode
1. Follow above instructions.
2. Start the bot with ./ant -d

How to start in Protocol Debug mode
1. Follow above instructions.
2. Start the bot with ./ant -n -p

## 4. CONTACT/SUPPORT
IRC: irc.Azuru.net (ports: 6660-6669; SSL: 5000) or irc.alphachat.net (SSL: 6697)
Website: http://www.Azuru.net/ or http://www.ephasic.org/

Git: https://github.com/Ephasic/ANT
