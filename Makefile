TARGET   := guardian
SRCDIR   := src
BUILDDIR := build

CC       := clang
CFLAGS   := -Wall -Wextra

SRCEXT  := c
SOURCES := $(shell find $(SRCDIR) -type f -name "*.$(SRCEXT)")
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
DEPS    := $(OBJECTS:.o=.deps)

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -MD -MF $(@:.o=.deps) -c -o $@ $<

.PHONY: clean

-include $(DEPS)

clean:
	$(RM) -r $(BUILDDIR) $(TARGET)
