import qbs
import qbs.File
import qbs.FileInfo
import qbs.TextFile

Product {
    name: "attfeeder_version"
    type: "hpp"

    Group {
        name: "Input"
        files: [
            "app_version.h.in"
        ]
        fileTags: "hpp_in"
    }

    Rule {
        id: app_version
        alwaysRun: true
        inputs: ["hpp_in"]
        Artifact {
            filePath: "app_version.h"
            fileTags: ["hpp"]
        }

        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "generating app_version.h";
            cmd.highlight = "codegen";
            cmd.onWindows = (product.moduleProperty("qbs", "targetOS").contains("windows"));
            cmd.sourceCode = function() {
                var file;

                file = new TextFile(input.filePath);
                var content = file.readAll();
                file.close();
                // replace quoted quotes
                content = content.replace(/\\\"/g, '"');
                // replace Windows line endings
                if (onWindows) {
                    content = content.replace(/\r\n/g, "\n");
                }
                // replace the magic qmake incantations
                content = content.replace(/(\n#define APP_VERSION_MAJOR) .+\n/, "$1 " + project.versionMajor + "\n");
                content = content.replace(/(\n#define APP_VERSION_MINOR) .+\n/, "$1 " + project.versionMinor + "\n");
                content = content.replace(/(\n#define APP_VERSION_RELEASE) .+\n/, "$1 " + project.versionRelease + "\n");

                content = content.replace(/(\n#define APP_NAME) .+\n/, "$1 \"" + project.appName + "\"\n");

                var version = project.versionMajor + "." + project.versionMinor + "." + project.versionRelease;
                content = content.replace(/(\n#define APP_VERSION) .+\n/, "$1 \"" + version + "\"\n");

                // Bump build
                var buildFile = FileInfo.path(output.filePath) + "/.build";
                var build = 0;
                if (File.exists(buildFile)) {
                    file = TextFile(buildFile, TextFile.ReadOnly);
                    var versionString = file.readLine();
                    var buildString = file.readLine();
                    file.close();
                    if (versionString != version) {
                        build = 0;
                    } else {
                        build = parseInt(buildString) + 1;
                    }
                }
                file = TextFile(buildFile, TextFile.WriteOnly);
                file.writeLine(version);
                file.writeLine(build);
                file.close();

                content = content.replace(/(\n#define APP_VERSION_BUILD) .+\n/, "$1 " + build + "\n");

                var versionLong = version + "." + build;
                content = content.replace(/(\n#define APP_VERSION_LONG) .+\n/, "$1 \"" + versionLong + "\"\n");

                var now = new Date();
                content = content.replace(/(\n#define APP_YEAR) .+\n/, "$1 " + now.getFullYear() + "\n");
                content = content.replace(/(\n#define APP_DATE) .+\n/, "$1 \"" + now.toLocaleDateString("ru-RU") + "\"\n");

                file = new TextFile(output.filePath, TextFile.WriteOnly);
                file.truncate();
                file.write(content);
                file.close();
            }
            return cmd;
        }
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: product.buildDirectory
    }
}
