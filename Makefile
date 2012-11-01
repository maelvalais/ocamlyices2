# -*- indent-tabs-mode: true, mode: Makefile -*-

PACKAGE_NAME=ocamlyices2
PACKAGE_VERSION=0.0.1

OCAMLFIND=ocamlfind
OCAMLC=ocamlc
OCAMLOPT=ocamlopt
OCAMLMKLIB=ocamlmklib
OF_INSTALL=$(OCAMLFIND) install
OF_REMOVE=$(OCAMLFIND) remove
CFLAGS=-Wall -O2

C_STUBS= src/ocamlyices2_all.c
C_OBJS= build/ocamlyices2_all.o

ML = yices2.ml
ML_BYTE = $(ML:%.ml=build/%.cmo)
ML_NATIVE = $(ML:%.ml=build/%.cmx)

#	ocamlyices2_contexts.c \
#	ocamlyices2_models.c \
#	ocamlyices2_types.c \
#	ocamlyices2_terms.c \
#	ocamlyices2_utils.c \
#	ocamlyices2_misc.c

all: build

build: build/$(PACKAGE_NAME).cma build/$(PACKAGE_NAME).cmxa

# Generic compilation rules ####################################################

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(OCAMLC) $(CFLAGS:%=-ccopt %) -ccopt -o -ccopt $@ $< 
build/%.cmi: src/%.mli
	@mkdir -p $(dir $@)
	$(OCAMLC) -I build -c -o $@ $<
build/%.cmo: src/%.ml
	@mkdir -p $(dir $@)
	$(OCAMLC) -I build -c -o $@ $<
build/%.cmx: src/%.ml 
	@mkdir -p $(dir $@)
	$(OCAMLOPT) -I build -c -o $@ $<

# Dependencies #################################################################

build/yices2.cmo build/yices2.cmx: build/yices2.cmi

build/ocamlyices2_all.o: src/ocamlyices2_contexts.c src/ocamlyices2_terms.c src/ocamlyices2_models.c src/ocamlyices2_types.c src/ocamlyices2_misc.c
build/ocamlyices2_all.o build/ocamlyices2_contexts.o build/ocamlyices2_terms.o build/ocamlyices2_models.o build/ocamlyices2_types.o build/ocamlyices2_misc.o: src/ocamlyices2.h
build/ocamlyices2_all.o build/ocamlyices2_terms.o: src/ocamlyices2_terms_macros.h


# Library compilation ##########################################################

build/$(PACKAGE_NAME).cma: $(C_OBJS) $(ML_BYTE)
	$(OCAMLMKLIB) -o $(@:.cma=) $^ -lyices
build/$(PACKAGE_NAME).cmxa: $(C_OBJS) $(ML_NATIVE)
	$(OCAMLMKLIB) -o $(@:.cmxa=) $^ -lyices

# (Un)Install ##################################################################
install: build
	$(OF_INSTALL) $(PACKAGE_NAME) \
		build/$(PACKAGE_NAME).cma build/$(PACKAGE_NAME).cmxa \
		build/lib$(PACKAGE_NAME).a build/$(PACKAGE_NAME).a \
		build/yices2.cmi META -dll build/dll$(PACKAGE_NAME).so
uninstall:
	$(OF_REMOVE) $(PACKAGE_NAME)

# Clean up #####################################################################
clean:
	rm -rf build *.o *.[aos] *.cm[aoxi] *.cmxa *.so a.out .depend

.PHONY: all build install uninstall clean test

test: 
	@cd tests; for testfile in *.ml; do\
		if ocaml -I ../build/ ../build/ocamlyices2.cma $$testfile ; then\
			echo "test '$${testfile%.ml}' passed";\
		else\
			echo "test '$${testfile%.ml}' failed";\
		fi;\
	done

# vim:noet:
