const std = @import("std");
const builtin = @import("builtin");

pub const FontBackend = enum {
    FreeTypeFontConfig,
    CoreText,
    DirectWrite,
    FreeTypeCoreText,
    FreeTypeDirectWrite,
    CoreTextFontConfig,
    DirectWriteFontConfig,

    fn default(target: std.Target) FontBackend {
        return switch (target.os.tag) {
            .windows => .DirectWrite,
            .macos => .CoreText,
            else => .FreeTypeFontConfig
        };
    }

    fn hasFreeType(s: FontBackend) bool {
        return switch (s) {
            .FreeTypeFontConfig,
            .FreeTypeCoreText,
            .FreeTypeDirectWrite => true,
            else => false
        };
    }

    fn hasFontConfig(s: FontBackend) bool {
        return switch (s) {
            .FreeTypeFontConfig,
            .CoreTextFontConfig,
            .DirectWriteFontConfig => true,
            else => false
        };
    }

    fn hasDirectWrite(s: FontBackend) bool {
        return switch (s) {
            .DirectWrite,
            .FreeTypeDirectWrite,
            .DirectWriteFontConfig => true,
            else => false
        };
    }

    fn hasCoreText(s: FontBackend) bool {
        return switch (s) {
            .CoreText,
            .FreeTypeCoreText,
            .CoreTextFontConfig=> true,
            else => false
        };
    }
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const font_backend = b.option(
        FontBackend,
        "font-backend",
        "The font backend to use for discovery, parsing and rasterization.",
    ) orelse FontBackend.default(target.result);

    const sfreetype = b.systemIntegrationOption("freetype", .{ .default = FontBackend.default(target.result).hasFreeType() });
    //const sfontconfig = b.systemIntegrationOption("fontconfig", .{ .default = FontBackend.default().hasFontConfig() });

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
        .FreeTypeFontConfig => {
            lib_tests.root_module.addCMacro("ONECORE_FREETYPE_LOADER_IMPLEMENTATION", "");
            lib_tests.root_module.addCMacro("ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION", "");
        },
        .CoreText => {
            lib_tests.root_module.addCMacro("ONECORE_CORETEXT_LOADER_IMPLEMENTATION", "");
            lib_tests.root_module.addCMacro("ONECORE_CORETEXT_FINDER_IMPLEMENTATION", "");
        },
        .DirectWrite => {
            lib_tests.root_module.addCMacro("ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION", "");
            lib_tests.root_module.addCMacro("ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION", "");
        },
        .FreeTypeCoreText => {
            lib_tests.root_module.addCMacro("ONECORE_FREETYPE_LOADER_IMPLEMENTATION", "");
            lib_tests.root_module.addCMacro("ONECORE_CORETEXT_FINDER_IMPLEMENTATION", "");
        },
        .FreeTypeDirectWrite => {
            lib_tests.root_module.addCMacro("ONECORE_FREETYPE_LOADER_IMPLEMENTATION", "");
            lib_tests.root_module.addCMacro("ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION", "");
        },
        else => {},
    }

    if (font_backend.hasFreeType()) {
        if (sfreetype) {
            lib_tests.root_module.linkSystemLibrary("freetype2", .{});
        } else if (b.lazyDependency("freetype", .{
            .target = target,
            .optimize = optimize,
        })) |freetype| {
            lib_tests.root_module.linkLibrary(freetype.artifact("freetype"));
        }
    }

    if (font_backend.hasFontConfig()) {
        lib_tests.root_module.linkSystemLibrary("fontconfig", .{});
    }

    if (font_backend.hasDirectWrite()) {
        lib_tests.root_module.linkSystemLibrary("dwrite", .{});
    }

    if (font_backend.hasCoreText()) {
        addAppleSDK(b, lib_tests.root_module);

        lib_tests.root_module.linkFramework("CoreFoundation", .{});
        lib_tests.root_module.linkFramework("CoreGraphics", .{});
        lib_tests.root_module.linkFramework("CoreText", .{});
    }

    lib_tests.root_module.addIncludePath(unity.path("src"));
    lib_tests.root_module.addCSourceFile(.{ .file = unity.path("src/unity.c") });

    lib_tests.root_module.addIncludePath(b.path("test/src"));
    lib_tests.root_module.addIncludePath(b.path("../"));

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

    switch (builtin.os.tag) {
        .windows => example.root_module.linkSystemLibrary("dwrite", .{}),
        .macos => {
            addAppleSDK(b, example.root_module);

            example.root_module.linkFramework("CoreFoundation", .{});
            example.root_module.linkFramework("CoreGraphics", .{});
            example.root_module.linkFramework("CoreText", .{});
        },
        else => {
            example.root_module.linkSystemLibrary("freetype2", .{});
        },
    }

    example.root_module.addIncludePath(b.path("examples"));
    example.root_module.addIncludePath(b.path("../"));

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
