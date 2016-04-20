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

$(MODLIB): $(OBJECT_C) $(OBJECT_CXX)
	@echo ""
	@echo "------------ Building library: " $@
	$(AR) crv lib$@.a $^
	@echo ""

install: $(MODLIB)
	@echo ""
	@echo "-------------- Installing " lib$(MODLIB)  " to " $(INSTALL_LIBDIR)
	@if [ ! -d $(INSTALL_LIBDIR) ]; then \
		echo "create directory ... ";\
		mkdir -p $(INSTALL_LIBDIR) || exit -1;\
	fi
	-@cp lib$(MODLIB).a $(INSTALL_LIBDIR)/
	@echo ""

uninstall:
	@echo ""
	@echo "----------- Uninstall ------------------- "
	rm -rf $(INSTALL_LIBDIR)/lib$(MODLIB).a
	@echo ""

# not removing include
clean:
	@echo "Cleaning"
	@rm -rf $(CLEAN_TARGETS)
debug:
	@$(MAKE) clean
	@$(MAKE) -e BUILD_ENV=debug
# <====== COMPLING RULES ========>
$(OBJECT_C) : %.o : %.c
	@echo "-- COMPILING FILE: " $*.c
	$(CC) $(CFLAGS) $(INC_PATH) -c $< -o $@
	@echo ""

$(OBJECT_CXX) : %.o : %.cpp
	@echo "-- COMPILING FILE: " $*.cpp
	$(CXX) $(CXXFLAGS) $(CXX_INC_PATH) -c $< -o $@
	@echo ""
