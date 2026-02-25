const std = @import("std");

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

pub const Options = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    font_backend: ?FontBackend = null,
};

pub fn buildLibrary(b: *std.Build, options: Options) *std.Build.Step.Compile {
    const target = options.target;
    const optimize = options.optimize;
    const font_backend = options.font_backend orelse FontBackend.default(options.target.result);

    const lib_module = b.createModule(.{
        .target = target,
        .optimize = optimize,
    });

    lib_module.link_libc = true;
    switch (font_backend) {
        .DirectWrite => lib_module.linkSystemLibrary("dwrite", .{}),
        .CoreText => {
            lib_module.linkFramework("CoreFoundation", .{});
            lib_module.linkFramework("CoreGraphics", .{});
            lib_module.linkFramework("CoreText", .{});
        },
        .FreeType => blk: {
            if (b.systemIntegrationOption("freetype", .{ .default = target.result.os.tag == .linux })) {
                lib_module.linkSystemLibrary("freetype2", .{});
                break :blk;
            }

            const libfreetype = b.lazyDependency("freetype", .{
                .target = target,
                .optimize = optimize,
            }) orelse break :blk;

            lib_module.linkLibrary(libfreetype.artifact("freetype"));
        },
    }

    lib_module.addIncludePath(b.path("include"));
    lib_module.addIncludePath(b.path("src"));
    
    lib_module.addCSourceFiles(.{
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

    const lib = b.addLibrary(.{
        .name = "onecore",
        .root_module = lib_module,
    });

    lib.installHeader(b.path("include/onecore.h"), "onecore.h");

    return lib;
}
