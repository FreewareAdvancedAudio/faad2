# Generated AAC test inputs

These short synthetic inputs contain no third-party recording. Generated with
FFmpeg's native AAC encoder:

```sh
ffmpeg -f lavfi -i sine=frequency=440:sample_rate=44100:duration=0.05 -c:a aac -b:a 32k -f adts mono.aac
ffmpeg -f lavfi -i anullsrc=r=48000:cl=5.1 -t 0.05 -c:a aac -b:a 96k -f adts surround.aac
```

The mono input exercises the ordinary WAV header; the six-channel input
exercises WAVE_FORMAT_EXTENSIBLE. FFmpeg is not needed to run the tests.
Run `python3 tests/test_stdout_wav.py build/faad` from the repository root.
