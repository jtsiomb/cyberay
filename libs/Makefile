.PHONY: all
all: treestore imago

.PHONY: clean
clean: clean-treestore clean-imago

.PHONY: treestore
treestore:
	$(MAKE) -C treestore

.PHONY: clean-treestore
clean-treestore:
	$(MAKE) -C treestore clean

.PHONY: imago
imago:
	$(MAKE) -C imago

.PHONY: clean-imago
clean-imago:
	$(MAKE) -C imago clean
