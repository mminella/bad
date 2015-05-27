ifndef GO
GO=go
endif

PACKAGES=bad/settings bad/sim bad/sort bad/utils imp1

.PHONY: clean fmt vet lint help

all: imp1

imp1: src/*/*.go
	@GOPATH=$(CURDIR) $(GO) install imp1

clean:
	@GOPATH=$(CURDIR) $(GO) clean
	@rm -Rf bin
	@rm -Rf pkg

fmt:
	@GOPATH=$(CURDIR) $(GO) fmt ${PACKAGES}

vet:
	@GOPATH=$(CURDIR) $(GO) vet ${PACKAGES}

lint:
	@command -v golint >/dev/null 2>&1 \
		|| { echo >&2 "The 'golint' tool is required, please install"; exit 1;  }
	@for p in ${PACKAGES}; do \
		GOPATH=$(CURDIR) golint $$p; \
	done

help:
	@echo "Build Targets"
	@echo "   all        - Build simulator"
	@echo "   clean      - Remove built sources"
	@echo "   fmt        - Format the source code using 'go fmt'"
	@echo "   vet        - Analyze the source code for potential errors"
	@echo "   lint       - Analyze the source code for style mistakes"
	@echo ""

