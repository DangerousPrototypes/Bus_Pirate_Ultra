##
## This file is part of the libopencm3 project.
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

GIT_VERSION := $(shell git describe --dirty --always --tags)

CFLAGS +=-DFWVER=\"$(GIT_VERSION)\"
CFLAGS +=-DPRINTF_DISABLE_SUPPORT_FLOAT
CFLAGS +=-DPRINTF_DISABLE_SUPPORT_LONG_LONG
CFLAGS +=-DPRINTF_DISABLE_SUPPORT_PTRDIFF_T
CFLAGS +=-I../printf

OBJS += debug.o cdcacm.o delay.o fpga.o fs.o
OBJS += ../printf/printf.o

BINARY = buspirate

include ../Makefile.include



