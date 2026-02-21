const std = @import("std");
const builtin = @import("builtin");

const FontBackend = enum {
    Freetype,
    CoreText,
    DirectWrite,

    fn default(target: std.Target) FontBackend {
        return switch (target.os.tag) {
            .windows => .DirectWrite,
            // .driverkit, does not have ct
            .ios,
            .macos,
            .tvos,
            .visionos,
            .watchos => .CoreText,
            else => .Freetype,
        };
    }
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const apple_sdk = b.dependency("apple_sdk", .{
        .target = target,
        .optimize = optimize,
    });

    // would be nice to lazy load this
    const unity = b.dependency("unity", .{
        .target = target,
        .optimize = optimize,
    });

    const font_backend = b.option(
        FontBackend,
        "font-backend",
        "The font backend to use for parsing and rasterization.",
    ) orelse FontBackend.default(target.result);

    const onecore_module = b.createModule(.{
        .target = target,
        .optimize = optimize,
    });

    onecore_module.link_libc = true;
    switch (font_backend) {
        .DirectWrite => onecore_module.linkSystemLibrary("dwrite", .{}),
        .Freetype => onecore_module.linkSystemLibrary("freetype2", .{}),
        .CoreText => {
            onecore_module.addSystemFrameworkPath(apple_sdk.path("System/Library/Frameworks"));
            onecore_module.addSystemIncludePath(apple_sdk.path("usr/include"));
            onecore_module.addLibraryPath(apple_sdk.path("usr/lib"));

            onecore_module.linkFramework("CoreFoundation", .{});
            onecore_module.linkFramework("CoreGraphics", .{});
            onecore_module.linkFramework("CoreText", .{});
        },
    }

    onecore_module.addIncludePath(b.path("include"));
    onecore_module.addIncludePath(b.path("src"));

    onecore_module.addCSourceFiles(.{
        .root = b.path("src"),
        .files = &.{
            "shared.c",
            "freetype.c",
            "dwrite.c",
            "coretext.c",
        },
        .flags = &.{
            "-Wall",
            "-Wextra",
            "-Werror",
            switch (font_backend) {
                .Freetype => "-DONECORE_FREETYPE",
                .CoreText => "-DONECORE_CORETEXT",
                .DirectWrite => "-DONECORE_DWRITE",
            },
        },
    });

    const onecore = b.addLibrary(.{
        .name = "onecore",
        .linkage = .dynamic, // todo: need to make it static!!!
        .root_module = onecore_module,
    });

    b.installArtifact(onecore);

    const lib_tests = b.addExecutable(.{
        .name = "test",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    lib_tests.linkLibC();

    lib_tests.linkLibrary(onecore);

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
