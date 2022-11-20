all: build

build: update
	go build

update:
	go get -u

clean:
	rm -f reg_dereg_test go.sum
	git checkout go.mod
