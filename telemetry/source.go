package main

import (
	"context"
	"fmt"
	"io"
	"os"
	"time"

	"go.bug.st/serial"
)

func openSource(port string, baud int, replay string) (io.ReadCloser, error) {
	if replay != "" {
		return os.Open(replay)
	}

	p, err := serial.Open(port, &serial.Mode{BaudRate: baud})
	if err != nil {
		return nil, fmt.Errorf("open %s: %w", port, err)
	}
	// The OS keeps unread bytes from the last time the port was open.
	// Drop them so the session doesn't start with stale samples.
	if err := p.ResetInputBuffer(); err != nil {
		p.Close()
		return nil, err
	}
	return p, nil
}

// waitUntil sleeps until t, or returns false early if ctx is cancelled.
func waitUntil(ctx context.Context, t time.Time) bool {
	timer := time.NewTimer(time.Until(t))
	defer timer.Stop()
	select {
	case <-ctx.Done():
		return false
	case <-timer.C:
		return true
	}
}
