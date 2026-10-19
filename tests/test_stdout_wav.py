"""CLI regression: python3 tests/test_stdout_wav.py /path/to/faad."""

import pathlib
import struct
import subprocess
import sys
import tempfile
import unittest

FAAD = str(pathlib.Path(sys.argv.pop(1)).resolve())
FIXTURES = pathlib.Path(__file__).parent / "fixtures"
UNKNOWN_SIZE = 0xFFFFFFFF


def run_decoder(*args, **kwargs):
    result = subprocess.run(*args, **kwargs)
    for diagnostic in (b"runtime error:", b"ERROR: AddressSanitizer"):
        if diagnostic in (result.stderr or b""):
            raise AssertionError(result.stderr.decode(errors="replace"))
    return result


def wav_parts(data):
    assert data[:4] == b"RIFF" and data[8:12] == b"WAVE"
    offset = 12
    while data[offset:offset + 4] != b"data":
        size = struct.unpack_from("<I", data, offset + 4)[0]
        offset += 8 + size + (size % 2)
    return (struct.unpack_from("<I", data, 4)[0],
            struct.unpack_from("<I", data, offset + 4)[0],
            data[offset + 8:])


class StdoutWavTests(unittest.TestCase):
    def test_stdout_and_named_output(self):
        for fixture in ("mono.aac", "surround.aac"):
            for sample_format in (1, 2, 3, 4):
                with self.subTest(fixture=fixture, sample_format=sample_format):
                    args = [FAAD, "-q", "-b", str(sample_format)]
                    source = str(FIXTURES / fixture)
                    with tempfile.TemporaryDirectory() as directory:
                        output = pathlib.Path(directory) / "named.wav"
                        run_decoder(args + ["-o", str(output), source],
                                       check=True, capture_output=True)
                        named = output.read_bytes()
                        streamed = run_decoder(args + ["-w", source],
                                                  check=True, capture_output=True).stdout
                        redirected = pathlib.Path(directory) / "redirected.wav"
                        with redirected.open("wb") as target:
                            run_decoder(args + ["-w", source], stdout=target,
                                           stderr=subprocess.PIPE, check=True)
                        self.assertEqual(streamed, redirected.read_bytes())
                        riff_size, data_size, pcm = wav_parts(named)
                        self.assertEqual(riff_size, len(named) - 8)
                        self.assertEqual(data_size, len(pcm))
                        self.assertGreater(len(pcm), 0)
                        stream_riff, stream_size, stream_pcm = wav_parts(streamed)
                        self.assertEqual(stream_riff, UNKNOWN_SIZE)
                        self.assertEqual(stream_size, UNKNOWN_SIZE)
                        self.assertEqual(stream_pcm, pcm)
                        # Format metadata, including extensible GUIDs, is unchanged.
                        header_end = len(named) - len(pcm) - 4
                        self.assertEqual(streamed[8:header_end], named[8:header_end])
                        raw = run_decoder(args + ["-f", "2", "-w", source],
                                             check=True, capture_output=True).stdout
                        self.assertEqual(raw, pcm)


if __name__ == "__main__":
    unittest.main()
