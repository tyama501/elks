# Makefile of /libc/time module

include $(TOPDIR)/libc/Makefile.inc

OBJS= gmtime.o localtime.o ctime.o tm_conv.o asctime.o asc_conv.o tzset.o mktime.o \
    strftime.o

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	$(RM) *.[aod]
