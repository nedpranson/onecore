const std = @import("std");
const builtin = @import("builtin");

pub const FontBackend = enum {
    FreeType,
    CoreText,
    DirectWrite,

    pub fn default(target: std.Target) FontBackend {
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

    const font_backend = b.option(
        FontBackend,
        "font-backend",
        "The font backend to use for parsing and rasterization.",
    ) orelse FontBackend.default(target.result);

    const unity = b.dependency("unity", .{});

    const lib_tests = b.addExecutable(.{
        .name = "test",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    lib_tests.root_module.link_libc = true;

    switch (font_backend) {
        .DirectWrite => lib_tests.root_module.linkSystemLibrary("dwrite", .{}),
        .FreeType => {
            lib_tests.root_module.linkSystemLibrary("freetype2", .{});
            lib_tests.root_module.linkSystemLibrary("fontconfig", .{});
        },
        .CoreText => {
            addAppleSDK(b, lib_tests.root_module);

            lib_tests.root_module.linkFramework("CoreFoundation", .{});
            lib_tests.root_module.linkFramework("CoreGraphics", .{});
            lib_tests.root_module.linkFramework("CoreText", .{});
        },
    }

    lib_tests.root_module.addIncludePath(unity.path("src"));
    lib_tests.root_module.addCSourceFile(.{ .file = unity.path("src/unity.c") });

    lib_tests.root_module.addIncludePath(b.path("test/src"));
    lib_tests.root_module.addIncludePath(b.path(""));

    lib_tests.root_module.addCSourceFile(.{
        .file = b.path("test/src/main.c"),
        .flags = &.{
            "-Wall",
            "-Wextra",
            "-Werror",
            "-std=c99",
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

    switch (font_backend) {
        .FreeType => {
            example.root_module.linkSystemLibrary("freetype2", .{});
        },
        .DirectWrite => example.root_module.linkSystemLibrary("dwrite", .{}),
        .CoreText => {
            addAppleSDK(b, example.root_module);

            example.root_module.linkFramework("CoreFoundation", .{});
            example.root_module.linkFramework("CoreGraphics", .{});
            example.root_module.linkFramework("CoreText", .{});
        },
    }

    example.root_module.addIncludePath(b.path("examples"));
    example.root_module.addIncludePath(b.path(""));

    example.root_module.addCSourceFile(.{ .file = b.path("examples/render_to_image.c") });

    const install_example = b.addInstallArtifact(example, .{});

    const example_step = b.step("example", "Build example");
    example_step.dependOn(&install_example.step);
}

fn addAppleSDK(b: *std.Build, m: *std.Build.Module) void {
    if (builtin.os.tag.isDarwin()) return;

    const apple_sdk = b.lazyDependency("apple_sdk", .{}) orelse return;

    m.addSystemFrameworkPath(apple_sdk.path("System/Library/Frameworks"));
    m.addSystemIncludePath(apple_sdk.path("usr/include"));
    m.addLibraryPath(apple_sdk.path("usr/lib"));
}
