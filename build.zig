const std = @import("std");
const builtin = @import("builtin");
const libonecore = @import("build/libonecore.zig");
const libunity = @import("build/libunity.zig");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // const font_backend = b.option(
    //     libonecore.FontBackend,
    //     "font-backend",
    //     "The font backend to use for parsing and rasterization.",
    // );

    // const onecore = libonecore.buildLibrary(b, .{
    //     .target = target,
    //     .optimize = optimize,
    //     .font_backend = font_backend,
    // });
    // b.installArtifact(onecore);

    const unity = libunity.buildLibrary(b, .{
        .target = target,
        .optimize = optimize,
    });

    const lib_tests = b.addExecutable(.{
        .name = "test",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    // lib_tests.root_module.linkLibrary(onecore);
    lib_tests.root_module.linkLibrary(unity);
    lib_tests.root_module.linkSystemLibrary("freetype2", .{});

    lib_tests.root_module.addIncludePath(b.path("test/src"));
    lib_tests.root_module.addIncludePath(b.path("include"));

    lib_tests.root_module.addCSourceFiles(.{
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

    const example = b.addExecutable(.{
        .name = "example",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    example.root_module.link_libc = true;
    // example.root_module.linkLibrary(onecore);

    example.root_module.addIncludePath(b.path("examples"));
    example.root_module.addCSourceFile(.{ .file = b.path("examples/render_to_image.c") });

    const install_example = b.addInstallArtifact(example, .{});

    const example_step = b.step("example", "Build example");
    example_step.dependOn(&install_example.step);
}

// fn addAppleSDK(b: *std.Build, m: *std.Build.Module) void {
//     if (builtin.os.tag.isDarwin()) return;
//
//     const apple_sdk = b.lazyDependency("apple_sdk", .{}) orelse return;
//
//     m.addSystemFrameworkPath(apple_sdk.path("System/Library/Frameworks"));
//     m.addSystemIncludePath(apple_sdk.path("usr/include"));
//     m.addLibraryPath(apple_sdk.path("usr/lib"));
// }
