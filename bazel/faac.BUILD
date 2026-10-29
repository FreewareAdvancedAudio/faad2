package(default_visibility = ["//visibility:public"])

genrule(
    name = "faad3_config",
    outs = ["config.h"],
    cmd = """cat > $@ <<'EOF'
#define MAX_CHANNELS 8
EOF
""",
)

cc_library(
    name = "faad3",
    srcs = glob(["libfaad/*.c", "common/*.c"]) + [":faad3_config"],
    hdrs = glob(["include/*.h", "libfaad/*.h", "libfaad/*.inc", "common/*.h", "common/*.inc"]),
    copts = ["-Wall", "-Wextra", "-include", "$(location :faad3_config)"],
    defines = ["COMMON_PREFIX=faad_"],
    includes = ["include", "libfaad", "common"],
    linkopts = ["-lm"],
)

cc_library(
    name = "faar",
    srcs = [
        "libfaar/dft_coeffs.c",
        "libfaar/dft_fir.c",
        "libfaar/dft_prefilter.c",
        "libfaar/fft.c",
        "libfaar/polyresample.c",
    ],
    hdrs = glob(["include/faar.h", "libfaar/*.h", "libfaar/*.inc", "common/*.h"]),
    copts = ["-Wall", "-Wextra"],
    local_defines = ["FAAR_WIDE_HIGH"],
    includes = ["include", "libfaar"],
    linkopts = ["-lm"],
)
