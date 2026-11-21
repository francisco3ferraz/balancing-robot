package main

import (
	"bufio"
	"errors"
	"os"
	"testing"
)

func TestParseLine(t *testing.T) {
	tests := []struct {
		name    string
		line    string
		want    Sample
		wantErr bool
	}{
		// Valid lines, copied from testdata/filter_test.csv.
		{
			name: "at rest on the floor",
			line: "1048,16004,-3224,-4800,-34,84,-28,1.16,0.52,0.01,1.16,10001",
			want: Sample{
				AccelX: 1048, AccelY: 16004, AccelZ: -3224, Temp: -4800,
				GyroX: -34, GyroY: 84, GyroZ: -28,
				AccelAngle: 1.16, GyroRate: 0.52, GyroAngle: 0.01, Angle: 1.16,
				DtMicros: 10001,
			},
		},
		{
			name: "tilted forward, negative angles",
			line: "-3464,14700,-23408,-4800,4904,1685,-1623,-52.95,-74.87,-28.80,-36.74,9999",
			want: Sample{
				AccelX: -3464, AccelY: 14700, AccelZ: -23408, Temp: -4800,
				GyroX: 4904, GyroY: 1685, GyroZ: -1623,
				AccelAngle: -52.95, GyroRate: -74.87, GyroAngle: -28.80, Angle: -36.74,
				DtMicros: 9999,
			},
		},
		{
			name: "gyro saturated at int16 max",
			line: "316,13536,-19120,-4816,32767,-7375,-11660,-48.40,-500.26,-14.84,-11.62,10001",
			want: Sample{
				AccelX: 316, AccelY: 13536, AccelZ: -19120, Temp: -4816,
				GyroX: 32767, GyroY: -7375, GyroZ: -11660,
				AccelAngle: -48.40, GyroRate: -500.26, GyroAngle: -14.84, Angle: -11.62,
				DtMicros: 10001,
			},
		},
		{
			name: "gyro saturated at int16 min",
			line: "2728,4108,-3800,-4800,-32768,7023,7634,-3.19,500.27,-6.92,-16.87,10000",
			want: Sample{
				AccelX: 2728, AccelY: 4108, AccelZ: -3800, Temp: -4800,
				GyroX: -32768, GyroY: 7023, GyroZ: 7634,
				AccelAngle: -3.19, GyroRate: 500.27, GyroAngle: -6.92, Angle: -16.87,
				DtMicros: 10000,
			},
		},

		// Non-data lines the firmware or the ESP32 prints on every reset.
		{name: "empty line", line: "", wantErr: true},
		{name: "PWR_MGMT_1 read-back", line: "PWR_MGMT_1 = 0x01", wantErr: true},
		{name: "GYRO_CONFIG read-back", line: "GYRO_CONFIG = 0x08", wantErr: true},
		{name: "read error message", line: "read_regs failed", wantErr: true},
		{name: "boot ROM noise at the wrong baud", line: "\xe0\x00\xfc\x1c\x80 \xfe\x00\x00", wantErr: true},

		// Partial lines: opening the port mid-stream usually cuts the first line.
		{name: "start of line cut off (11 fields)", line: "16004,-3224,-4800,-34,84,-28,1.16,0.52,0.01,1.16,10001", wantErr: true},
		{name: "end of line cut off (11 fields)", line: "1048,16004,-3224,-4800,-34,84,-28,1.16,0.52,0.01,1.16", wantErr: true},
		{name: "two lines glued together", line: "1048,16004,-3224,-4800,-34,84,-28,1.16,0.52,0.01,1.16,100011048,16004", wantErr: true},
		{name: "last field empty", line: "1048,16004,-3224,-4800,-34,84,-28,1.16,0.52,0.01,1.16,", wantErr: true},

		// Twelve fields, but a value is corrupt. Must be an error, not a silent 0.
		{name: "letters in an int field", line: "10x8,16004,-3224,-4800,-34,84,-28,1.16,0.52,0.01,1.16,10001", wantErr: true},
		{name: "letters in a float field", line: "1048,16004,-3224,-4800,-34,84,-28,1.16,0.52,0.01,nan?,10001", wantErr: true},
		{name: "int16 overflow (40000)", line: "40000,16004,-3224,-4800,-34,84,-28,1.16,0.52,0.01,1.16,10001", wantErr: true},
		{name: "negative dt_us", line: "1048,16004,-3224,-4800,-34,84,-28,1.16,0.52,0.01,1.16,-5", wantErr: true},
		{name: "empty field in the middle", line: "1048,16004,,-4800,-34,84,-28,1.16,0.52,0.01,1.16,10001", wantErr: true},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, err := parseLine(tt.line)
			if tt.wantErr {
				if err == nil {
					t.Fatalf("parseLine(%q) = %+v, want an error", tt.line, got)
				}
				if !errors.Is(err, ErrInvalidLine) {
					t.Errorf("parseLine(%q) error = %v, want it to wrap ErrInvalidLine", tt.line, err)
				}
				return
			}
			if err != nil {
				t.Fatalf("parseLine(%q) unexpected error: %v", tt.line, err)
			}
			if got != tt.want {
				t.Errorf("parseLine(%q)\n got  %+v\n want %+v", tt.line, got, tt.want)
			}
		})
	}
}

// Every line of a real 150 s capture must parse, with a plausible loop period.
func TestParseLineRecording(t *testing.T) {
	f, err := os.Open("testdata/filter_test.csv")
	if err != nil {
		t.Fatal(err)
	}
	defer f.Close()

	sc := bufio.NewScanner(f)
	n := 0
	for sc.Scan() {
		n++
		s, err := parseLine(sc.Text())
		if err != nil {
			t.Fatalf("line %d: %v (%q)", n, err, sc.Text())
		}
		if s.DtMicros < 9900 || s.DtMicros > 10100 {
			t.Errorf("line %d: dt_us = %d, want about 10000", n, s.DtMicros)
		}
	}
	if err := sc.Err(); err != nil {
		t.Fatal(err)
	}
	if n < 10000 {
		t.Errorf("read %d lines, expected the full recording (~14873)", n)
	}
}
