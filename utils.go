package pkst

import (
	"fmt"
	"net"
)

func LoopbackIPv4() (net.IP, error) {
	ia, e := net.InterfaceAddrs()
	if e != nil {
		return nil, e
	}
	for _, a := range ia {
		if ip, ok := a.(*net.IPNet); ok {
			if ip.IP.IsLoopback() && ip.IP.To4() != nil {
				return ip.IP, nil
			}
		}
	}
	return nil, fmt.Errorf("unable to find a private ipv4 address")
}
