pspl_add_extension(PSPL_IR "Platform-independent intermediate shader representation")
pspl_add_extension_toolchain(PSPL_IR IRToolchain.c CalcChain.c)
pspl_add_extension_runtime(PSPL_IR IRRuntime.c CalcChain.c)