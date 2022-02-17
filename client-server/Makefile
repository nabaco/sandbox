SUBDIRS=ncsa/client ncsa/server ncsa/common
CLEAN_SUBDIRS=$(addprefix clean-,$(SUBDIRS))
.PHONY: subdirs $(SUBDIRS) clean $(CLEAN_SUBDIRS)

export CPPFLAGS+=-Wall -Wextra -Werror
subdirs:$(SUBDIRS)

ncsa/client /ncsa/server:ncsa/common

$(SUBDIRS):
	$(MAKE) -C $@

clean:$(CLEAN_SUBDIRS)

$(CLEAN_SUBDIRS):
	$(MAKE) -C $(@:clean-%=%) clean
