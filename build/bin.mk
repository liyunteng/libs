# include it at the end of a program makefile.
# $(PROGBIN) must be defined by user befor include
# this file
all: headers exe
headers:
	@if [ -e $(INCDIR) ]; then \
		echo "";\
		echo "---- Moving header to include directory " ${INSTALL_INCDIR}; \
		cp $(INCDIR)/*.h* $(INSTALL_INCDIR); \
		echo ""; \
	fi;

libs :

exe : $(PROGBIN)
$(PROGBIN) : $(OBJECT_C:.o=.lo) $(OBJECT_CXX:.o=.lo)
	@echo ""
	@echo "---------- Building excutable : " $@
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $? $(LD_LIBS) $(LD_LIBS_PATH) $(LIBS) $(LIBS_PATH)
	@echo ""

install : $(PROGBIN)
	@echo ""
	@echo "-------------- Installing " $(PROGBIN)  " to " $(INSTALL_BINDIR)
	@if [ ! -d $(INSTALL_BINDIR) ]; then \
		echo "create directory ... ";\
		mkdir -p $(INSTALL_BINDIR) || exit -1;\
	fi
	@echo ""
	$(LIBTOOL) --tag=CC --mode=install install -c $(PROGBIN) $(INSTALL_BINDIR)/$(PROGBIN)
	@echo ""

uninstall:
	@echo ""
	@echo "------------ Uninstall " $(PROGBIN)
	@rm -rf $(INSTALL_BINDIR)/$(PROGBIN)
	@echo ""

#not removing include
clean :
	rm -rf $(CLEAN_TARGETS) $(PROGBIN)

# <====== COMPLING RULES ========>
$(OBJECT_C:.o=.lo) : $(OBJECT_C)
$(OBJECT_C) : %.o: %.c
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(CFLAGS) $(INC_PATH) -c $<

$(OBJECT_CXX:.o=.lo) : $(OBJECT_CXX)
$(OBJECT_CXX) : %.o: %.cpp
	$(LIBTOOL) --tag=CC --mode=compile $(CXX) $(CXXFLAGS) $(CXX_INC_PATH) -c $<
