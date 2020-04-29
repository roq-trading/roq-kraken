noinst_LTLIBRARIES = libroq-kraken-json.la
libroq_kraken_json_la_LDFLAGS = \
	-static \
	$(LIBCONFIG_LIBS) \
	$(ROQ_LIBS)
libroq_kraken_json_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(LIBCONFIG_CFLAGS) \
	$(ROQ_CFLAGS)
AUTOGEN_CPP = \
	book.cpp \
	book_item.cpp \
	heartbeat.cpp \
	open_positions.cpp \
	order.cpp \
	own_trades_item.cpp \
	ping.cpp \
	pong.cpp \
	spread.cpp \
	subscription_status.cpp \
	system_status.cpp \
	time_.cpp \
	trade.cpp \
	trade_item.cpp
AUTOGEN_H = \
	book.h \
	book_item.h \
	heartbeat.h \
	open_positions.h \
	order.h \
	own_trades_item.h \
	ping.h \
	pong.h \
	spread.h \
	subscription_status.h \
	system_status.h \
	time_.h \
	trade.h \
	trade_item.h
CLEANFILES = $(AUTOGEN_CPP) $(AUTOGEN_H)
BUILT_SOURCES = $(CLEANFILES) .gitignore
libroq_kraken_json_la_SOURCES = \
	$(AUTOGEN_CPP) \
	utils.cpp

.gitignore: Makefile.am
	echo $(AUTOGEN_CPP) $(AUTOGEN_H) | tr " " "\n" | sort > $@

.json.h:
	@abs_top_builddir@/autogen/autogen.py \
		--namespace=roq/kraken/json \
		--output-type json \
		--file-type h \
		$< > $@

.json.cpp:
	@abs_top_builddir@/autogen/autogen.py \
		--namespace=roq/kraken/json \
		--output-type json \
		--file-type cpp \
		$< > $@
