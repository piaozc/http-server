package ipc

import (
	"context"
	"log"
	"net"

	"github.com/piaozc/http-server/business/internal/service"
)

type Server struct {
	addr     string
	services *service.Services
}

func NewServer(addr string, services *service.Services) *Server {
	return &Server{
		addr:     addr,
		services: services,
	}
}

func (s *Server) ListenAndServe(ctx context.Context) error {
	listener, err := net.Listen("tcp", s.addr)
	if err != nil {
		return err
	}
	defer listener.Close()

	go func() {
		<-ctx.Done()
		_ = listener.Close()
	}()

	log.Printf("ipc server listening on %s", s.addr)
	for {
		conn, err := listener.Accept()
		if err != nil {
			if ctx.Err() != nil {
				return nil
			}
			return err
		}
		go s.handleConn(conn)
	}
}

func (s *Server) handleConn(conn net.Conn) {
	defer conn.Close()
	// The frame protocol will be implemented after the API contract is finalized.
	_ = s.services
}
