module example

replace github.com/hhorai/gnbsim => ../gnbsim

replace github.com/hhorai/gnbsim/encoding/gtp => ../gnbsim/encoding/gtp

replace github.com/hhorai/gnbsim/encoding/nas => ../gnbsim/encoding/nas

replace github.com/hhorai/gnbsim/encoding/ngap => ../gnbsim/encoding/ngap

require (
	github.com/hhorai/gnbsim/encoding/gtp v0.0.0-20210214125543-31c2f406a8fb
	github.com/hhorai/gnbsim/encoding/nas v0.0.0-20210214125543-31c2f406a8fb
	github.com/hhorai/gnbsim/encoding/ngap v0.0.0-20210214125543-31c2f406a8fb
	github.com/ishidawataru/sctp v0.0.0-20210204083202-2bdd86bb9a4d
	github.com/vishvananda/netlink v1.1.1-0.20200603190747-5400e006d43d
)

go 1.13
