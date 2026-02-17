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
                lib.addSystemFrameworkPath(.{ .cwd_relative = "/home/nedas/Work/macos-sdk/Frameworks" });
                lib.addSystemIncludePath(.{ .cwd_relative = "/home/nedas/Work/macos-sdk/include" });
                lib.addLibraryPath(.{ .cwd_relative = "/home/nedas/Work/macos-sdk/lib" });

                lib.linkFramework("CoreFoundation");
                lib.linkFramework("CoreGraphics");
                lib.linkFramework("CoreText");
            } else {

                const ft = b.dependency("freetype", .{
                    .target = target,
                    .optimize = optimize,
                });
                

                //lib.linkSystemLibrary("freetype2");
                lib.linkLibrary(ft.artifact("freetype"));
            }
        },
    }

    lib.addIncludePath(b.path("include"));
    lib.addIncludePath(b.path("src"));

    lib.addCSourceFiles(.{
        .root = b.path("src"),
        .files = &.{
            "freetype.c",
            "dwrite.c",
            "coretext.c",
        },
        .flags = &.{
            "-Wall",
            "-Wextra",
            "-Werror",
        },
    });

    b.installArtifact(lib);

    const example = b.addExecutable(.{
        .name = "render_to_image",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    example.linkLibC();
    example.linkLibrary(lib);
    example.addIncludePath(b.path("include"));

    example.addCSourceFiles(.{
        .root = b.path("examples"),
        .files = &.{
            "render_to_image.c",
        },
        .flags = &.{
            "-Wall",
            "-Wextra",
            "-Werror",
        },
    });

    if (target.result.os.tag.isDarwin()) {
        example.addSystemFrameworkPath(.{ .cwd_relative = "/home/nedas/Work/macos-sdk/Frameworks" });
        example.addSystemIncludePath(.{ .cwd_relative = "/home/nedas/Work/macos-sdk/include" });
        example.addLibraryPath(.{ .cwd_relative = "/home/nedas/Work/macos-sdk/lib" });
    }

    b.installArtifact(example);

    const lib_tests = b.addExecutable(.{
        .name = "test",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    lib_tests.addFrameworkPath(b.path("../macos-sdk/Frameworks"));

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

    if (target.result.os.tag.isDarwin()) {
        lib_tests.addSystemFrameworkPath(.{ .cwd_relative = "/home/nedas/Work/macos-sdk/Frameworks" });
        lib_tests.addSystemIncludePath(.{ .cwd_relative = "/home/nedas/Work/macos-sdk/include" });
        lib_tests.addLibraryPath(.{ .cwd_relative = "/home/nedas/Work/macos-sdk/lib" });
    }

    const run_lib_tests = b.addRunArtifact(lib_tests);

    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&run_lib_tests.step);
}
