# $(DIRS), $(MODLIB) must have been defined by user
all:  headers exe
headers:
	@if [ -e $(INCDIR) ]; then \
		echo ""; \
		echo "---- Moving header to include directory " ${INSTALL_INCDIR}; \
		cp $(INCDIR)/*.h* $(INSTALL_INCDIR); \
	fi;
	@for dirx in $(DIRS); do \
		echo "";\
		echo "------------------- WORKING DIR ===> $$dirx"; \
		echo ""; \
		$(MAKE) -C $$dirx headers || exit 1; \
	done;

exe: libs
	@for dirx in $(DIRS); do \
	echo ""; \
	echo "------------------ WORKING DIR ===> $$dirx"; \
	echo "";\
	$(MAKE) -C $$dirx exe || exit 1;\
	done;

libs: $(MODLIB)
	@for dirx in $(DIRS); do \
	echo ""; \
	echo "------------------ WORKING DIR ===> $$dirx"; \
	echo "";\
	$(MAKE) -C $$dirx libs || exit 1;\
	done;

$(MODLIB): $(OBJECT_C:.o=.lo) $(OBJECT_CXX:.o=.lo)
ifneq ($(MODLIB),)
	@echo ""
	@echo "----- Building library : " $@
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) -o lib$@.la $? \
	-rpath $(INSTALL_LIBDIR) $(LD_LIBS) $(LD_LIBS_PATH) $(LIBS) $(LIBS_PATH)
	@echo ""
endif

install : $(MODLIB)
ifneq ($(MODLIB),)
	@echo ""
	@echo "-------------- Installing " lib$(MODLIB)  " to " $(INSTALL_LIBDIR)
	@if [ ! -d $(INSTALL_LIBDIR) ]; then \
		echo "create directory ... ";\
		mkdir -p $(INSTALL_LIBDIR) || exit -1;\
	fi
	$(LIBTOOL) --tag=CC --mode=install install -c lib$(MODLIB).la $(INSTALL_LIBDIR)/lib$(MODLIB).la
endif
	@echo ""
	@echo "----------------- INSTALLING ----------------"
	@for dirx in $(DIRS); do \
		echo "";\
		echo "------------------ WORKING DIR ===> $$dirx"; \
		echo "";\
		$(MAKE) -C $$dirx install || exit 1;\
	done;
	@echo "----------------- DONE -----------------------";
	@echo ""
# $(LIBTOOL) --finish $(INSTALL_LIBDIR)

uninstall :
ifneq ($(MODLIB),)
	@echo ""
	@echo "---------------- Uninstall -----------------"
	rm -rf $(INSTALL_LIBDIR)/lib$(MODLIB).*
endif
	@for dirx in $(DIRS); do \
		echo ""; \
		echo "------------------ WORKING DIR ===> $$dirx"; \
		echo ""; \
		$(MAKE) -C $$dirx uninstall; \
	done;
	@echo "------------------ DONE ---------------------"
	@echo ""

clean :
	@echo ""
	@echo "---------------- CLEANING -----------------"
	@rm -rf $(CLEAN_TARGETS) $(INSTALL_LIBDIR)/lib$(MODLIB).*
	@for dirx in $(DIRS); do \
		echo ""; \
		echo "------------------ WORKING DIR ===> $$dirx"; \
		echo ""; \
		$(MAKE) -C $$dirx clean; \
	done;
	@echo "----------------- DONE -----------------------";
	@echo ""
debug:
	@$(MAKE) clean
	@$(MAKE) -e BUILD_ENV=debug
	@for dirx in $(DIRS); do \
		$(MAKE) -C $$dirx debug; \
	done;
# <====== COMPLING RULES ========>
$(OBJECT_C:.o=.lo) : $(OBJECT_C)
	
$(OBJECT_C) : %.o: %.c
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(CFLAGS) $(INC_PATH) -c $<

$(OBJECT_CXX:.o=.lo) : $(OBJECT_CXX)
	
$(OBJECT_CXX) : %.o: %.cpp
	$(LIBTOOL) --tag=CXX --mode=compile $(CXX) $(CFLAGS) $(INC_PATH) -c $<
