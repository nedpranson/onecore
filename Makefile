FMT_DIRS := include src test/src
FMT_FILES := $(shell find $(FMT_DIRS) -type f \( -iname "*.c" -o -iname "*.h" \))

FMT := clang-format -i --style=file
HASH := xxhsum

.PHONY: fmt
fmt:
	@for f in $(FMT_FILES); do \
		old=$$($(HASH) "$$f" | awk '{print $$1}'); \
		$(FMT) "$$f"; \
		new=$$($(HASH) "$$f" | awk '{print $$1}'); \
		if [ "$$old" != "$$new" ]; then \
			echo "$$f"; \
		fi \
	done

.PHONY: default
default:
	@zig build

.PHONY: test
test:
	@zig build test

.PHONY: clean
clean:
	@rm -rf zig-out
