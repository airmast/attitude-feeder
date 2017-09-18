import qbs

CppApplication {
    name: "attfeeder"

    Depends { name: "Qt"; submodules: ["core", "network", "serialport"]}
    Depends { name: "attfeeder_version" }

    cpp.includePaths: [
        "../mavlink"
    ]

    cpp.cxxFlags: ["-std=c++11"]

    cpp.defines: {
        var defines = [];
        if (qbs.buildVariant == "debug") {
            defines.push("DEBUG");
        }
        return defines;
    }

    files: [
        "core.cpp", "core.h",
        "main.cpp",
        "mavlink_interface.cpp", "mavlink_interface.h"
    ]

    Group {
        fileTagsFilter: product.type
        qbs.install: true
    }
}
