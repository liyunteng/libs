# $(MODLIB) must be defined by user befor include this file

all: headers exe
headers:
	@if [ -e $(INCDIR) ]; then \
		echo "";\
		echo "---- Moving header to include directory " ${INSTALL_INCDIR}; \
		cp $(INCDIR)/*.h* $(INSTALL_INCDIR); \
	fi;


exe: libs
libs: $(MODLIB)

$(MODLIB): $(OBJECT_C:.o=.lo) $(OBJECT_CXX:.o=.lo)
	@echo ""
	@echo "------------ Building library: " $@
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o \
	lib$@.la $? -rpath $(INSTALL_LIBDIR) $(LD_LIBS) $(LD_LIBS_PATH) $(LIBS) $(LIBS_PATH)
	@echo ""

install: $(MODLIB)
ifneq ($(MODLIB),)
	@echo ""
	@echo "-------------- Installing " lib$(MODLIB)  " to " $(INSTALL_LIBDIR)
	@if [ ! -d $(INSTALL_LIBDIR) ]; then \
		echo "create directory ... ";\
		mkdir -p $(INSTALL_LIBDIR) || exit -1;\
	fi
	$(LIBTOOL) --tag=CC --mode=install install -c lib$(MODLIB).la $(INSTALL_LIBDIR)/lib$(MODLIB).la
# $(LIBTOOL) --finish .libs
	@echo ""
endif

uninstall:
ifneq ($(MODLIB),)
	@echo ""
	@echo "----------- Uninstall ------------------- "
	rm -rf $(INSTALL_LIBDIR)/lib$(MODLIB).*
	@echo ""
endif

# not removing include
clean:
	@echo "Cleaning..."
	@rm -rf $(CLEAN_TARGETS)
debug:
	@$(MAKE) clean
	@$(MAKE) -e BUILD_ENV=debug
# <====== COMPLING RULES ========>
$(OBJECT_C:.o=.lo) : $(OBJECT_C)
	
$(OBJECT_C) : %.o: %.c
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(CFLAGS) $(INC_PATH) -c $<

$(OBJECT_CXX:.o=.lo) : $(OBJECT_CXX)
	
$(OBJECT_CXX) : %.o: %.cpp
	$(LIBTOOL) --tag=CXX --mode=compile $(CXX) $(CXXFLAGS) $(CXX_INC_PATH) -c $<
