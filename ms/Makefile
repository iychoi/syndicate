include ../buildconf.mk
include ../MS.mk

MS_FILES		:=
MS_FILES_IN	:= 
MS_YAML_IN	:= 

# include YAML files 
MS_YAML		:= $(MS_FILES) $(wildcard *.yaml)

# include base files 
MS_FILES		:= $(MS_FILES) $(wildcard *.py)
MS_FILES		:= $(MS_FILES) $(wildcard common/*.py)
MS_FILES		:= $(MS_FILES) $(wildcard protobufs/*.py)
MS_FILES		:= $(MS_FILES) $(wildcard storage/*.py)
MS_FILES		:= $(MS_FILES) $(wildcard storage/backends/*.py)
MS_FILES		:= $(MS_FILES) $(wildcard MS/*.py)
MS_FILES		:= $(MS_FILES) $(wildcard MS/methods/*.py)

# google protobuf files 
MS_FILES		:= $(MS_FILES) $(wildcard google/*.py)
MS_FILES		:= $(MS_FILES) $(wildcard google/protobuf/*.py)
MS_FILES		:= $(MS_FILES) $(wildcard google/protobuf/internal/*.py)
MS_FILES		:= $(MS_FILES) $(wildcard google/protobuf/compiler/*.py)

# protobuf-generated files 
MS_PROTOBUF_FILES	:= $(notdir $(wildcard $(BUILD_PROTOBUFS_PYTHON)/*.py))

# python templates 
MS_FILES_IN := $(MS_FILES_IN) $(wildcard common/*.pyin)

# YAML templates 
MS_YAML_IN	:= $(MS_YAML_IN) $(wildcard *.yamlin)

# python targets
MS_FILES_BUILD			:= $(patsubst %.py,$(BUILD_MS)/%.py,$(MS_FILES))

# python protobuf targets
MS_FILES_BUILD_PROTOBUF	:= $(patsubst %.py,$(BUILD_MS)/protobufs/%.py,$(MS_PROTOBUF_FILES))

# YAML template targets
MS_YAML_BUILD_OUT		:= $(patsubst %.yamlin,$(BUILD_MS)/%.yaml,$(MS_YAML_IN))

# python template targets
MS_FILES_BUILD_OUT	:= $(patsubst %.pyin,$(BUILD_MS)/%.py,$(MS_FILES_IN))

all: $(MS_FILES_BUILD) $(MS_YAML_BUILD_OUT) $(MS_FILES_BUILD_OUT) $(MS_FILES_BUILD_PROTOBUF)

$(BUILD_MS)/%.py: %.py
	@mkdir -p $(shell dirname "$@")
	@cat "$<" > "$@"

$(BUILD_MS)/common/admin_info.py: common/admin_info.pyin $(MS_APP_ADMIN_PUBLIC_KEY) $(MS_APP_PUBLIC_KEY) $(MS_APP_PRIVATE_KEY)
	@mkdir -p $(shell dirname "$@")
	@cat "$<" | \
		sed -e 's~@MS_APP_NAME@~$(MS_APP_NAME)~g;' | \
		sed -e 's~@MS_APP_ADMIN_EMAIL@~$(MS_APP_ADMIN_EMAIL)~g;' | \
		sed -e 's~@MS_APP_ADMIN_REGISTER_PASSWD@~$(MS_APP_ADMIN_REGISTER_PASSWD)~g;' | \
		sed -e 's~@MS_DEVEL@~$(MS_DEVEL)~g;' | \
		sed -e 's~@MS_APP_ADMIN_PUBLIC_KEY@~$(shell cat $(MS_APP_ADMIN_PUBLIC_KEY) | tr "\n" "@" | sed 's/@/\\n/g')~g;' | \
		sed -e 's~@MS_APP_PRIVATE_KEY@~$(shell cat $(MS_APP_PRIVATE_KEY) | tr "\n" "@" | sed 's/@/\\n/g')~g;' | \
		sed -e 's~@MS_APP_PUBLIC_KEY@~$(shell cat $(MS_APP_PUBLIC_KEY) | tr "\n" "@" | sed 's/@/\\n/g')~g;' > "$@"

$(BUILD_MS)/app.yaml: app.yamlin
	@mkdir -p $(shell dirname "$@")
	@cat "$<" | \
		sed -e 's~@MS_APP_NAME@~$(MS_APP_NAME)~g;' > "$@"


# fix up protobufs to work on GAE 
$(BUILD_MS)/protobufs/%.py: $(BUILD_PROTOBUFS_PYTHON)/%.py google_protobuf_fixer.pyin
	@mkdir -p $(shell dirname "$@")
	@echo '# Automatically generated by Makefile.  DO NOT EDIT!' > "$@"
	@cat google_protobuf_fixer.pyin >> "$@"
	@cat "$<" >> "$@"

.PHONY: clean
clean:
	@rm -rf $(BUILD_MS)

# debugging
print-%: ; @echo $*=$($*)

