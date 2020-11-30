#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.
#

CC=gcc
CFLAGS=-Wall -Wextra -Werror -std=c99 \
	-Og -g -march=native -mtune=native \
	`pkg-config --cflags icu-uc icu-io` \
	-D_POSIX_C_SOURCE=20201201L
PROGNAME=main
LIBS=`pkg-config --libs icu-uc icu-io`
OBJS=$(PROGNAME).o

.PHONY: all clean clean-all

all: $(PROGNAME)

$(PROGNAME): $(OBJS)
	$(CC) -o '$@' $^ $(LIBS)

$(PROGNAME).o: $(PROGNAME).c
	$(CC) -x c $(CFLAGS) -c -o '$@' '$<'

clean:
	-@rm $(OBJS)
clean-all: clean
	-@rm ./$(PROGNAME)
