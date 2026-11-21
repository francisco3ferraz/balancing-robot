package main

import (
	"bufio"
	"context"
	"errors"
	"flag"
	"fmt"
	"os"
	"os/signal"
	"time"
)

func main() {
	port := flag.String("port", "", "serial port to read telemetry from")
	baud := flag.Int("baud", 921600, "serial port baud rate")
	replay := flag.String("replay", "", "CSV file to replay telemetry from")
	logPath := flag.String("log", "", "log file for live mode (default logs/<date>_<time>.csv)")
	flag.Parse()

	if (*port == "") == (*replay == "") {
		fmt.Fprintln(os.Stderr, "specify exactly one of -port or -replay")
		flag.Usage()
		os.Exit(2)
	}
	live := *replay == ""

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt)
	defer stop()

	src, err := openSource(*port, *baud, *replay)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	defer src.Close()

	// A read from the serial port blocks; closing it on Ctrl+C unblocks the loop.
	go func() {
		<-ctx.Done()
		src.Close()
	}()

	// Replays are not logged: the log would just be a copy of the input.
	var logFile *os.File
	var log *bufio.Writer
	if live {
		logFile, err = createLog(*logPath)
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			os.Exit(1)
		}
		log = bufio.NewWriter(logFile)
		fmt.Fprintln(log, csvHeader)
		fmt.Fprintln(os.Stderr, "logging to", logFile.Name())
	}

	scanner := bufio.NewScanner(src)
	nextSampleAt := time.Now()
	samples, bad := 0, 0

	for scanner.Scan() {
		line := scanner.Text()
		sample, err := parseLine(line)
		if err != nil {
			bad++
			if bad <= 5 {
				fmt.Fprintln(os.Stderr, "skipping:", err)
			}
			continue
		}
		samples++

		if live {
			fmt.Fprintln(log, line)
		} else {
			// Play back at recorded speed. Advancing a deadline (like the
			// firmware loop) keeps small sleep errors from adding up.
			nextSampleAt = nextSampleAt.Add(time.Duration(sample.DtMicros) * time.Microsecond)
			if !waitUntil(ctx, nextSampleAt) {
				break
			}
		}

		fmt.Printf("%+v\n", sample)
	}

	// After Ctrl+C the closed port makes Scan fail; that's expected.
	if err := scanner.Err(); err != nil && ctx.Err() == nil {
		fmt.Fprintln(os.Stderr, "read error:", err)
	}
	fmt.Fprintf(os.Stderr, "%d samples, %d bad lines\n", samples, bad)

	if live {
		// Flush reports any write error since the start, so this is where lost data shows up.
		if err := errors.Join(log.Flush(), logFile.Close()); err != nil {
			fmt.Fprintf(os.Stderr, "log %s may be incomplete: %v\n", logFile.Name(), err)
			os.Exit(1)
		}
		fmt.Fprintln(os.Stderr, "saved", logFile.Name())
	}
}
