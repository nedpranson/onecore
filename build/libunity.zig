const std = @import("std");

pub const Options = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
};

pub fn buildLibrary(b: *std.Build, options: Options) *std.Build.Step.Compile {
    const target = options.target;
    const optimize = options.optimize;

    // todo: when available make it lazy dep
    const unity = b.dependency("unity", .{});

    const lib = b.addLibrary(.{
        .name = "unity",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    lib.root_module.link_libc = true;
    lib.root_module.addIncludePath(unity.path("src"));
    lib.root_module.addCSourceFile(.{ .file = unity.path("src/unity.c") });

    lib.installHeadersDirectory(unity.path("src"), "", .{});

    return lib;
}
