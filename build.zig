const std = @import("std");
const apple_sdk = @import("apple_sdk");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const unity = b.dependency("unity", .{
        .target = target,
        .optimize = optimize,
    });

    const lib_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
    });

    const lib = b.addLibrary(.{
        .name = "onecore",
        .root_module = lib_mod,
    });

    lib.linkLibC();

    switch (target.result.os.tag) {
        .windows => lib.linkSystemLibrary("dwrite"),
        else => |tag| {
            if (tag.isDarwin()) {
                // we need apple-sdk shit
                lib.linkFramework("CoreText");
            } else {
                lib.linkSystemLibrary("freetype2");
            }
        },
    }

    lib.addIncludePath(b.path("include"));
    lib.addIncludePath(b.path("src"));

    lib.addCSourceFiles(.{
        .root = b.path("src"),
        .files = &.{
            "freetype/library.c",
            "freetype/face.c",
            "dwrite/library.c",
            "dwrite/face.c",
            "coretext/library.c",
        },
        .flags = &.{
            "-Wall",
            "-Wextra",
            "-Werror",
        },
    });

    b.installArtifact(lib);

    const lib_tests = b.addExecutable(.{
        .name = "test",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    lib_tests.linkLibC();
    lib_tests.linkLibrary(lib);

    lib_tests.addIncludePath(b.path("include"));

    lib_tests.addIncludePath(unity.path("src"));
    lib_tests.addCSourceFile(.{ .file = unity.path("src/unity.c") });

    lib_tests.addIncludePath(b.path("test/src"));
    lib_tests.addCSourceFiles(.{
        .root = b.path("test/src"),
        .files = &.{
            "main.c",
        },
        .flags = &.{
            "-Wall",
            "-Wextra",
            "-Werror",
        },
    });

    const run_lib_tests = b.addRunArtifact(lib_tests);

    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&run_lib_tests.step);
}
