package(default_visibility = ["//visibility:public"])

genrule(
    name = "faad3_config",
    outs = ["config.h"],
    cmd = """cat > $@ <<'EOF'
#define PACKAGE "faac"
#define PACKAGE_VERSION "3.0.0"
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define WORDS_BIGENDIAN 1
#else
#define WORDS_BIGENDIAN 0
#endif
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
