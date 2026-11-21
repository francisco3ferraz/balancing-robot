package main

import (
	"bufio"
	"context"
	"errors"
	"flag"
	"fmt"
	"io"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"time"

	"go.bug.st/serial"
)

var ErrInvalidLine = errors.New("invalid line")

// ax,ay,az,temp,gx,gy,gz,accel_angle,gyro_rate,gyro_angle,angle,dt_us
type Sample struct {
	AccelX     int16   `json:"ax"` // raw counts
	AccelY     int16   `json:"ay"`
	AccelZ     int16   `json:"az"`
	Temp       int16   `json:"temp"` // raw; °C = Temp/340 + 36.53
	GyroX      int16   `json:"gx"`   // raw counts
	GyroY      int16   `json:"gy"`
	GyroZ      int16   `json:"gz"`
	AccelAngle float32 `json:"accel_angle"` // degrees
	GyroRate   float32 `json:"gyro_rate"`   // degrees per second
	GyroAngle  float32 `json:"gyro_angle"`  // degrees
	Angle      float32 `json:"angle"`       // degrees, complementary filter
	DtMicros   uint32  `json:"dt_us"`       // microseconds since the previous sample
}

func main() {
	port := flag.String("port", "", "serial port to read telemetry from")
	baud := flag.Int("baud", 921600, "serial port baud rate")
	replay := flag.String("replay", "", "CSV file to replay telemetry from")
	flag.Parse()

	if (*port == "") == (*replay == "") {
		fmt.Fprintln(os.Stderr, "specify exactly one of -port or -replay")
		flag.Usage()
		os.Exit(2)
	}

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt)
	defer stop()

	src, err := openSource(*port, *baud, *replay)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	defer src.Close()

	go func() {
		<-ctx.Done()
		src.Close()
	}()

	var lines lineScanner = bufio.NewScanner(src)
	if *replay != "" {
		lines = newPacedScanner(ctx, src)
	}

	samples, bad := 0, 0
	for lines.Scan() {
		sample, err := parseLine(lines.Text())
		if err != nil {
			bad++
			if bad <= maxBadLinesShown {
				fmt.Fprintf(os.Stderr, "skipping: %v\n", err)
			}
			continue
		}
		samples++
		fmt.Printf("%+v\n", sample)
	}

	if err := lines.Err(); err != nil && ctx.Err() == nil {
		fmt.Fprintf(os.Stderr, "read error: %v\n", err)
	}
	fmt.Fprintf(os.Stderr, "%d samples, %d bad lines\n", samples, bad)
}

// maxBadLinesShown limits how many skipped lines are printed in full.
const maxBadLinesShown = 5

func openSource(port string, baud int, replay string) (io.ReadCloser, error) {
	if replay != "" {
		f, err := os.Open(replay)
		if err != nil {
			return nil, fmt.Errorf("open replay file: %w", err)
		}
		return f, nil
	}
	p, err := serial.Open(port, &serial.Mode{BaudRate: baud})
	if err != nil {
		return nil, fmt.Errorf("open serial port %s: %w", port, err)
	}
	return p, nil
}

type lineScanner interface {
	Scan() bool
	Text() string
	Err() error
}

type pacedScanner struct {
	*bufio.Scanner
	ctx context.Context
	due time.Time
}

func newPacedScanner(ctx context.Context, r io.Reader) *pacedScanner {
	return &pacedScanner{Scanner: bufio.NewScanner(r), ctx: ctx, due: time.Now()}
}

func (p *pacedScanner) Scan() bool {
	if !p.Scanner.Scan() {
		return false
	}
	p.due = p.due.Add(lineInterval(p.Text()))
	t := time.NewTimer(time.Until(p.due))
	defer t.Stop()
	select {
	case <-p.ctx.Done():
		return false
	case <-t.C:
		return true
	}
}

// lineInterval returns the line's dt_us (its last column), or 10 ms for
// lines without a usable one.
func lineInterval(line string) time.Duration {
	last := line[strings.LastIndexByte(line, ',')+1:]
	us, err := strconv.ParseUint(last, 10, 32)
	if err != nil {
		return 10 * time.Millisecond
	}
	return time.Duration(us) * time.Microsecond
}

const numFields = 12

// parseLine parses one telemetry line. Every error wraps ErrInvalidLine.
func parseLine(line string) (Sample, error) {
	fields := strings.Split(line, ",")
	if len(fields) != numFields {
		return Sample{}, fmt.Errorf("%w: got %d fields, want %d", ErrInvalidLine, len(fields), numFields)
	}

	// Go evaluates the calls in a composite literal left to right,
	// so the fields are consumed in CSV column order.
	p := fieldParser{fields: fields}
	s := Sample{
		AccelX:     p.int16(),
		AccelY:     p.int16(),
		AccelZ:     p.int16(),
		Temp:       p.int16(),
		GyroX:      p.int16(),
		GyroY:      p.int16(),
		GyroZ:      p.int16(),
		AccelAngle: p.float32(),
		GyroRate:   p.float32(),
		GyroAngle:  p.float32(),
		Angle:      p.float32(),
		DtMicros:   p.uint32(),
	}
	if p.err != nil {
		return Sample{}, p.err
	}
	return s, nil
}

// fieldParser reads fields in order and remembers the first error
type fieldParser struct {
	fields []string
	i      int // index of the next field to read
	err    error
}

// next returns the next field, or ok=false if an earlier field failed.
func (p *fieldParser) next() (field string, ok bool) {
	if p.err != nil {
		return "", false
	}
	field = p.fields[p.i]
	p.i++
	return field, true
}

// fail records err for the field just read, wrapped with ErrInvalidLine.
func (p *fieldParser) fail(err error) {
	p.err = fmt.Errorf("%w: field %d: %w", ErrInvalidLine, p.i-1, err)
}

func (p *fieldParser) int16() int16 {
	f, ok := p.next()
	if !ok {
		return 0
	}
	v, err := strconv.ParseInt(f, 10, 16)
	if err != nil {
		p.fail(err)
	}
	return int16(v)
}

func (p *fieldParser) float32() float32 {
	f, ok := p.next()
	if !ok {
		return 0
	}
	v, err := strconv.ParseFloat(f, 32)
	if err != nil {
		p.fail(err)
	}
	return float32(v)
}

func (p *fieldParser) uint32() uint32 {
	f, ok := p.next()
	if !ok {
		return 0
	}
	v, err := strconv.ParseUint(f, 10, 32)
	if err != nil {
		p.fail(err)
	}
	return uint32(v)
}
