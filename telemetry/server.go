package main

import (
	"context"
	"encoding/json"
	"fmt"
	"net"
	"net/http"
	"sync"
)

// hub hands each sample to every connected browser.
type hub struct {
	mu      sync.Mutex
	clients map[chan Sample]bool
}

func newHub() *hub {
	return &hub{clients: map[chan Sample]bool{}}
}

func (h *hub) subscribe() chan Sample {
	ch := make(chan Sample, 100) // about 1 s of samples
	h.mu.Lock()
	h.clients[ch] = true
	h.mu.Unlock()
	return ch
}

func (h *hub) unsubscribe(ch chan Sample) {
	h.mu.Lock()
	delete(h.clients, ch)
	h.mu.Unlock()
}

// publish never blocks. If a browser falls behind and its buffer is full,
// it misses this sample; the read loop and the log never wait for it.
func (h *hub) publish(s Sample) {
	h.mu.Lock()
	defer h.mu.Unlock()
	for ch := range h.clients {
		select {
		case ch <- s:
		default:
		}
	}
}

// serveEvents streams samples as Server-Sent Events, one JSON object per message.
func (h *hub) serveEvents(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/event-stream")
	w.Header().Set("Cache-Control", "no-cache")
	rc := http.NewResponseController(w)

	ch := h.subscribe()
	defer h.unsubscribe(ch)

	for {
		select {
		case <-r.Context().Done():
			return
		case s := <-ch:
			data, err := json.Marshal(s)
			if err != nil {
				return
			}
			if _, err := fmt.Fprintf(w, "data: %s\n\n", data); err != nil {
				return
			}
			if err := rc.Flush(); err != nil {
				return
			}
		}
	}
}

// startServer listens on addr and serves in the background. Every request's
// context is derived from ctx, so cancelling ctx ends the open SSE streams,
// which would otherwise run forever and block Shutdown.
func startServer(ctx context.Context, addr string, h *hub) (*http.Server, error) {
	mux := http.NewServeMux()
	mux.HandleFunc("GET /events", h.serveEvents)

	srv := &http.Server{
		Handler:     mux,
		BaseContext: func(net.Listener) context.Context { return ctx },
	}

	// Listen here rather than in the goroutine, so "address in use" is
	// reported at startup instead of being lost.
	ln, err := net.Listen("tcp", addr)
	if err != nil {
		return nil, err
	}
	go srv.Serve(ln)
	return srv, nil
}
