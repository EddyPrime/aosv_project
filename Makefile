obj-m += tsm.o
tsm-objs := /kmodule/tsm.o /kmodule/group_dev.o /kmodule/group_dev_manager.o

CURRENT_PATH = $(shell pwd)
LINUX_KERNEL = $(shell uname -r)
#LINUX_KERNEL = 5.8.0-55-generic
LINUX_KERNEL_PATH = /lib/modules/$(LINUX_KERNEL)/build/
TESTS_DIR = $(CURRENT_PATH)/test
TESTS_FILE = $(CURRENT_PATH)/tests.txt
LIB_PATH = $(CURRENT_PATH)/lib

all:
	[ -d $(TESTS_DIR) ] || mkdir test
	gcc -O2 $(LIB_PATH)/doubleopen.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/doubleopen.out
	gcc -O2 $(LIB_PATH)/install.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/install.out
	gcc -O2 $(LIB_PATH)/exceed_messages.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/exceed_messages.out
	gcc -O2 $(LIB_PATH)/mp_multigroup.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/mp_multigroup.out
	gcc -O2 $(LIB_PATH)/mp_readwrite.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/mp_readwrite.out
	gcc -O2 $(LIB_PATH)/mp_sleep.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/mp_sleep.out
	gcc -O2 $(LIB_PATH)/mt_ordinary_chaotic.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/mt_ordinary_chaotic.out -lpthread
	gcc -O2 $(LIB_PATH)/mt_ordinary.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/mt_ordinary.out -lpthread
	gcc -O2 $(LIB_PATH)/mt_readwrite.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/mt_readwrite.out -lpthread
	gcc -O2 $(LIB_PATH)/multigroup.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/multigroup.out
	gcc -O2 $(LIB_PATH)/readwrite_delay.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/readwrite_delay.out
	gcc -O2 $(LIB_PATH)/readwrite.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/readwrite.out
	gcc -O2 $(LIB_PATH)/revoke.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/revoke.out
	gcc -O2 $(LIB_PATH)/sleep.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/sleep.out
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules
	
allDebug:
	[ -d $(TESTS_DIR) ] || mkdir test
	gcc -DDEBUG=1 -ggdb3 -Og $(LIB_PATH)/doubleopen.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/doubleopen.out
	gcc -DDEBUG=1 -ggdb3 -Og $(LIB_PATH)/install.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/install.out
	gcc -DDEBUG=1 -ggdb3 -Og $(LIB_PATH)/exceed_messages.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/exceed_messages.out
	gcc -DDEBUG=1 -ggdb3 -Og $(LIB_PATH)/mp_multigroup.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/mp_multigroup.out
	gcc -DDEBUG=1 -ggdb3 -Og $(LIB_PATH)/mp_readwrite.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/mp_readwrite.out
	gcc -DDEBUG=1 -ggdb3 -Og $(LIB_PATH)/mp_sleep.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/mp_sleep.out
	gcc -DDEBUG=1 -ggdb3 -Og $(LIB_PATH)/mt_ordinary_chaotic.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/mt_ordinary_chaotic.out -lpthread
	gcc -DDEBUG=1 -ggdb3 -Og $(LIB_PATH)/mt_ordinary.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/mt_ordinary.out -lpthread
	gcc -DDEBUG=1 -ggdb3 -Og $(LIB_PATH)/mt_readwrite.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/mt_readwrite.out -lpthread
	gcc -DDEBUG=1 -ggdb3 -Og $(LIB_PATH)/multigroup.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/multigroup.out
	gcc -DDEBUG=1 -ggdb3 -Og $(LIB_PATH)/readwrite_delay.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/readwrite_delay.out
	gcc -DDEBUG=1 -ggdb3 -Og $(LIB_PATH)/readwrite.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/readwrite.out
	gcc -DDEBUG=1 -ggdb3 -Og $(LIB_PATH)/revoke.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/revoke.out
	gcc -DDEBUG=1 -ggdb3 -Og $(LIB_PATH)/sleep.c $(LIB_PATH)/test.c $(LIB_PATH)/tsm_lib.c -o $(TESTS_DIR)/sleep.out
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) ccflags-y="-DDEBUG" modules

clean:
	[ ! -d $(TESTS_DIR) ] || [ -z "$$(ls -A $(TESTS_DIR))" ] || rm $(TESTS_DIR)/*
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean


.SILENT: listTests
listTests:
	[ -f $(TESTS_FILE) ] && cat $(TESTS_FILE)
