import qbs

StaticLibrary {
    name: "mavlink"

    Depends { name: "cpp" }

    cpp.cxxFlags: {
        if (qbs.toolchain.contains("gcc"))
            return ["-std=c++11", "-pthread"];
        return [];
    }

    Group {
        prefix: "message_definitions/v1.0/"
        files: [
            "common.xml",
        ]
        fileTags: "xml"
    }

    Rule {
        id: mavgen
        multiplex: true
        inputs: ["xml"]
        outputArtifacts: [
            { filePath: product.buildDirectory + "/include/ugcs/vsm/auto_mavlink_enums.h", fileTags: "hpp" },
            { filePath: product.buildDirectory + "/include/ugcs/vsm/auto_mavlink_messages.h", fileTags: "hpp" },
            { filePath: product.buildDirectory + "/auto_mavlink_messages.cpp", fileTags: "cpp" },
        ]
        outputFileTags: ["cpp", "hpp"]
        prepare: {
            var args = [];
            args.push('-B');
            args.push('mavlink/generator/mavgen.py');
            args.push('--wire-protocol 1.0');
            args.push('--lang C');
            var xmlFiles = "";
            for (var i in inputs.xml) {
                args.push(inputs.xml[i].filePath);
                //xmlFiles += inputs.xml[i].fileName + " ";
            }
            //args.push('--schema=../../../cpp-sdk/resources/mavlink/mavschema.xsd');
            args.push("-o " + product.buildDirectory);
            var cmd = new Command("python", args);
            cmd.workingDirectory = project.path;
            cmd.description = "mavgen " + xmlFiles;
            cmd.highlight = "codegen";
            return cmd;
        }
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: product.buildDirectory + "/include"
    }
}
