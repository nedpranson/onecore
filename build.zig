const std = @import("std");
const builtin = @import("builtin");

const FontBackend = enum {
    FreeType,
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
            else => .FreeType,
        };
    }
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

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
        .FreeType => blk: {
            if (b.systemIntegrationOption("freetype", .{ .default = target.result.os.tag == .linux })) {
                onecore_module.linkSystemLibrary("freetype2", .{});
            } else {
                const freetype = b.lazyDependency("freetype", .{
                    .target = target,
                    .optimize = optimize,
                }) orelse break :blk;

                onecore_module.linkLibrary(freetype.artifact("freetype"));
            }
        },
        .CoreText => {
            onecore_module.linkFramework("CoreFoundation", .{});
            onecore_module.linkFramework("CoreGraphics", .{});
            onecore_module.linkFramework("CoreText", .{});
            addAppleSDK(b, onecore_module);
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
                .FreeType => "-DONECORE_FREETYPE",
                .CoreText => "-DONECORE_CORETEXT",
                .DirectWrite => "-DONECORE_DWRITE",
            },
        },
    });

    const onecore = b.addLibrary(.{
        .name = "onecore",
        .linkage = .static,
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
    addAppleSDK(b, lib_tests.root_module);

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

fn addAppleSDK(b: *std.Build, m: *std.Build.Module) void {
    if (builtin.os.tag.isDarwin()) return;

    const apple_sdk = b.lazyDependency("apple_sdk", .{}) orelse return;

    m.addSystemFrameworkPath(apple_sdk.path("System/Library/Frameworks"));
    m.addSystemIncludePath(apple_sdk.path("usr/include"));
    m.addLibraryPath(apple_sdk.path("usr/lib"));
}
