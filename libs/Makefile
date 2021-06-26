.PHONY: all
all: treestore imago drawtext

.PHONY: clean
clean: clean-treestore clean-imago clean-drawtext

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

.PHONY: drawtext
drawtext:
	$(MAKE) -C drawtext

.PHONY: clean-drawtext
clean-drawtext:
	$(MAKE) -C drawtext clean
